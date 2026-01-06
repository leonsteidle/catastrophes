/*
  butterfly~ (Thom butterfly catastrophe) — equilibria of:
    V'(x) = x^5 + a x^3 + b x^2 + c x + d = 0

  Stability:
    V''(x) = 5x^4 + 3a x^2 + 2b x + c    (stable if > 0)

  Conventions (catastrophes~):
   - Signal inlets, but if an inlet is unconnected we use stored parameter.
   - Sample-and-hold per vector: compute once using first sample of each inlet.
   - width scales b,c,d (a is not scaled): b_eff=b*width, c_eff=c*width, d_eff=d*width
   - Output selection x_out: prefer stable real root closest to x_prev; else closest root.
   - If no real roots found: mode = hold|zero|nan.

  Inlets (signal/float messages):
    0 a, 1 b, 2 c, 3 d, 4 width

  Outlets (signal):
    0..4 r0..r4   (sorted real roots; unused = NAN)
    5    state    (chosen root index 0..4, or -1)
    6    nroots   (0..5)
    7    x_out

  Messages:
    a/b/c/d/width <f>, eps <f>, width_eps <f>, mode <sym>, reset, info
    float (no selector) sets a
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <math.h>
#include <float.h>
#include <complex.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    BF_MODE_HOLD = 0,
    BF_MODE_ZERO = 1,
    BF_MODE_NAN  = 2
} t_bf_mode;

typedef struct _butterfly {
    t_pxobject x_obj;

    // stored params (when inlet unconnected)
    double a, b, c, d, width;

    // numerics
    double eps;       // root / residual tolerance
    double width_eps; // clamp for |width|

    // selection memory
    double x_prev;

    // mode when nroots==0
    long mode;

    // connection flags
    short a_connected, b_connected, c_connected, d_connected, w_connected;
} t_butterfly;

static t_class *s_butterfly_class = NULL;

/* ---------------- utilities ---------------- */

static int bf_isfinite(double x)
{
    return (x == x) && (x <= DBL_MAX) && (x >= -DBL_MAX);
}

static double bf_clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void sort_doubles(double *arr, int n)
{
    for (int i = 0; i < n - 1; ++i)
        for (int j = i + 1; j < n; ++j)
            if (arr[j] < arr[i]) { double t = arr[i]; arr[i] = arr[j]; arr[j] = t; }
}

static int uniq_merge(double *arr, int n, double tol)
{
    if (n <= 1) return n;
    int w = 1;
    double prev = arr[0];
    for (int i = 1; i < n; ++i) {
        if (fabs(arr[i] - prev) > tol) {
            arr[w++] = arr[i];
            prev = arr[i];
        }
    }
    return w;
}

/* monic quintic: p(x)=x^5 + c4 x^4 + c3 x^3 + c2 x^2 + c1 x + c0 */
static inline double poly5_real(double x, double c4, double c3, double c2, double c1, double c0)
{
    return (((((x + c4) * x + c3) * x + c2) * x + c1) * x + c0);
}

static inline double poly5_real_deriv(double x, double c4, double c3, double c2, double c1)
{
    double x2 = x * x;
    double x3 = x2 * x;
    double x4 = x2 * x2;
    return 5.0 * x4 + 4.0 * c4 * x3 + 3.0 * c3 * x2 + 2.0 * c2 * x + c1;
}

static inline double complex poly5_c_monic(double complex z,
                                           double c4, double c3, double c2, double c1, double c0)
{
    double complex p = z + c4;
    p = p * z + c3;
    p = p * z + c2;
    p = p * z + c1;
    p = p * z + c0;
    return p;
}

/* V''(x) = 5x^4 + 3a x^2 + 2b x + c  (here b,c are effective) */
static inline double vpp(double x, double a, double b_eff, double c_eff)
{
    double x2 = x * x;
    double x4 = x2 * x2;
    return 5.0 * x4 + 3.0 * a * x2 + 2.0 * b_eff * x + c_eff;
}

/* Newton polish on real line */
static double newton_refine(double x, double c4, double c3, double c2, double c1, double c0)
{
    for (int it = 0; it < 10; ++it) {
        double fx  = poly5_real(x, c4, c3, c2, c1, c0);
        double dfx = poly5_real_deriv(x, c4, c3, c2, c1);
        if (fabs(dfx) < 1e-14) break;
        double step = fx / dfx;
        x -= step;
        if (fabs(step) < 1e-12) break;
    }
    return x;
}

/* -------- fallback scan+bisection (guaranteed, slower) -------- */

static double bisect_root(double a, double b, double c4, double c3, double c2, double c1, double c0, double eps)
{
    double fa = poly5_real(a, c4, c3, c2, c1, c0);
    double fb = poly5_real(b, c4, c3, c2, c1, c0);

    for (int it = 0; it < 90; ++it) {
        double m = 0.5 * (a + b);
        double fm = poly5_real(m, c4, c3, c2, c1, c0);

        if (fabs(fm) < eps) return m;

        if (fa * fm <= 0.0) { b = m; fb = fm; }
        else                { a = m; fa = fm; }
    }
    return 0.5 * (a + b);
}

static int solve_quintic_real_scan(double c4, double c3, double c2, double c1, double c0,
                                   double *out_real, double eps)
{
    double maxc = fabs(c4);
    if (fabs(c3) > maxc) maxc = fabs(c3);
    if (fabs(c2) > maxc) maxc = fabs(c2);
    if (fabs(c1) > maxc) maxc = fabs(c1);
    if (fabs(c0) > maxc) maxc = fabs(c0);

    double R = 1.0 + maxc;
    if (R < 1.0) R = 1.0;

    const int N = 4096;              // still only used when DK fails
    const double near0 = 1e-5;

    int nr = 0;
    double x0 = -R;
    double f0 = poly5_real(x0, c4, c3, c2, c1, c0);

    for (int i = 1; i <= N; ++i) {
        double x1 = -R + (2.0 * R) * ((double)i / (double)N);
        double f1 = poly5_real(x1, c4, c3, c2, c1, c0);

        if (fabs(f0) < near0 && nr < 5) out_real[nr++] = x0;

        if ((f0 * f1) < 0.0 && nr < 5) {
            double r = bisect_root(x0, x1, c4, c3, c2, c1, c0, eps);
            r = newton_refine(r, c4, c3, c2, c1, c0);
            out_real[nr++] = r;
        } else if (fabs(f1) < near0 && nr < 5) {
            double r = newton_refine(x1, c4, c3, c2, c1, c0);
            out_real[nr++] = r;
        }

        x0 = x1;
        f0 = f1;
    }

    if (nr == 0) return 0;

    sort_doubles(out_real, nr);
    nr = uniq_merge(out_real, nr, 1e-7);
    if (nr > 5) nr = 5;
    return nr;
}

/* -------- Durand–Kerner (complex), then extract real roots -------- */

static int solve_quintic_real_DK(double c4, double c3, double c2, double c1, double c0,
                                 double *out_real, double eps)
{
    const int n = 5;
    double complex z[5];

    double maxc = fabs(c4);
    if (fabs(c3) > maxc) maxc = fabs(c3);
    if (fabs(c2) > maxc) maxc = fabs(c2);
    if (fabs(c1) > maxc) maxc = fabs(c1);
    if (fabs(c0) > maxc) maxc = fabs(c0);

    double R = 1.0 + maxc;
    if (R < 1.0) R = 1.0;

    for (int k = 0; k < n; ++k) {
        double theta = 2.0 * M_PI * (double)k / (double)n;
        double jitter = 1.0 + 0.03 * (double)(k + 1);
        z[k] = (R * jitter) * (cos(theta) + I * sin(theta));
    }

    const int max_iter = 120;
    const double tol_step = 1e-12;

    for (int it = 0; it < max_iter; ++it) {
        double max_step = 0.0;

        for (int k = 0; k < n; ++k) {
            double complex denom = 1.0 + 0.0*I;
            for (int j = 0; j < n; ++j) {
                if (j == k) continue;
                denom *= (z[k] - z[j]);
            }

            if (cabs(denom) < 1e-18) {
                z[k] += (1e-6 + 1e-6*I);
                continue;
            }

            double complex p = poly5_c_monic(z[k], c4, c3, c2, c1, c0);
            double complex step = p / denom;
            z[k] -= step;

            double sm = cabs(step);
            if (sm > max_step) max_step = sm;
        }

        if (max_step < tol_step) break;
    }

    double reals[5];
    int nr = 0;

    // extraction criteria
    const double imag_tol_base = 1e-6;
    const double prescale = 1e-8;

    for (int k = 0; k < n; ++k) {
        double rr = creal(z[k]);
        double im = cimag(z[k]);

        double complex pz = poly5_c_monic(z[k], c4, c3, c2, c1, c0);
        double abs_pz = cabs(pz);

        double scale = pow(1.0 + cabs(z[k]), 5.0);
        double p_tol = prescale * scale;

        double imag_tol = imag_tol_base * (1.0 + fabs(rr));

        if (fabs(im) < imag_tol && abs_pz < p_tol) {
            double r = newton_refine(rr, c4, c3, c2, c1, c0);
            if (fabs(poly5_real(r, c4, c3, c2, c1, c0)) < (eps * 100.0)) {
                reals[nr++] = r;
            }
        }
    }

    if (nr == 0) return solve_quintic_real_scan(c4, c3, c2, c1, c0, out_real, eps);

    sort_doubles(reals, nr);
    nr = uniq_merge(reals, nr, 1e-7);
    if (nr > 5) nr = 5;

    for (int i = 0; i < nr; ++i) out_real[i] = reals[i];
    return nr;
}

/* ---------------- Max methods ---------------- */

static void butterfly_assist(t_butterfly *x, void *b, long m, long a, char *s)
{
    (void)x; (void)b;
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0: snprintf(s, 256, "signal/float: a"); break;
            case 1: snprintf(s, 256, "signal/float: b"); break;
            case 2: snprintf(s, 256, "signal/float: c"); break;
            case 3: snprintf(s, 256, "signal/float: d"); break;
            case 4: snprintf(s, 256, "signal/float: width"); break;
        }
    } else {
        switch (a) {
            case 0: snprintf(s, 256, "signal: r0"); break;
            case 1: snprintf(s, 256, "signal: r1"); break;
            case 2: snprintf(s, 256, "signal: r2"); break;
            case 3: snprintf(s, 256, "signal: r3"); break;
            case 4: snprintf(s, 256, "signal: r4"); break;
            case 5: snprintf(s, 256, "signal: state (chosen root index 0..4, -1 if none)"); break;
            case 6: snprintf(s, 256, "signal: nroots (0..5)"); break;
            case 7: snprintf(s, 256, "signal: x_out"); break;
        }
    }
}

static void butterfly_float(t_butterfly *x, double f) { x->a = f; }
static void butterfly_set_a(t_butterfly *x, double v) { x->a = v; }
static void butterfly_set_b(t_butterfly *x, double v) { x->b = v; }
static void butterfly_set_c(t_butterfly *x, double v) { x->c = v; }
static void butterfly_set_d(t_butterfly *x, double v) { x->d = v; }
static void butterfly_set_w(t_butterfly *x, double v) { x->width = v; }

static void butterfly_reset(t_butterfly *x) { x->x_prev = 0.0; }

static void butterfly_eps(t_butterfly *x, double e)
{
    if (bf_isfinite(e) && e > 0.0) x->eps = e;
}

static void butterfly_width_eps(t_butterfly *x, double e)
{
    if (bf_isfinite(e) && e > 0.0) x->width_eps = e;
}

static void butterfly_mode(t_butterfly *x, t_symbol *s)
{
    if (s == gensym("hold")) x->mode = BF_MODE_HOLD;
    else if (s == gensym("zero")) x->mode = BF_MODE_ZERO;
    else if (s == gensym("nan")) x->mode = BF_MODE_NAN;
    else object_warn((t_object*)x, "mode: use hold | zero | nan");
}

static void butterfly_info(t_butterfly *x)
{
    post("butterfly~: a=%g b=%g c=%g d=%g width=%g eps=%g width_eps=%g mode=%ld x_prev=%g",
         x->a, x->b, x->c, x->d, x->width, x->eps, x->width_eps, x->mode, x->x_prev);
    post("           connected: a=%d b=%d c=%d d=%d width=%d",
         (int)x->a_connected, (int)x->b_connected, (int)x->c_connected,
         (int)x->d_connected, (int)x->w_connected);
}

static void butterfly_perform64(t_butterfly *x, t_object *dsp64,
                                double **ins, long numins,
                                double **outs, long numouts,
                                long sampleframes, long flags, void *userparam)
{
    (void)dsp64; (void)numins; (void)numouts; (void)flags; (void)userparam;

    const double *in_a = ins[0];
    const double *in_b = ins[1];
    const double *in_c = ins[2];
    const double *in_d = ins[3];
    const double *in_w = ins[4];

    double *out_r0 = outs[0];
    double *out_r1 = outs[1];
    double *out_r2 = outs[2];
    double *out_r3 = outs[3];
    double *out_r4 = outs[4];
    double *out_state  = outs[5];
    double *out_nroots = outs[6];
    double *out_xout   = outs[7];

    // sample-and-hold: 1st sample of each inlet
    double a = x->a_connected ? in_a[0] : x->a;
    double b = x->b_connected ? in_b[0] : x->b;
    double c = x->c_connected ? in_c[0] : x->c;
    double d = x->d_connected ? in_d[0] : x->d;
    double w = x->w_connected ? in_w[0] : x->width;

    const double eps = (x->eps > 0.0 ? x->eps : 1e-12);
    const double weps = (x->width_eps > 0.0 ? x->width_eps : 1e-12);

    if (!bf_isfinite(a) || !bf_isfinite(b) || !bf_isfinite(c) || !bf_isfinite(d) || !bf_isfinite(w)) {
        for (long i = 0; i < sampleframes; ++i) {
            out_r0[i]=out_r1[i]=out_r2[i]=out_r3[i]=out_r4[i]=NAN;
            out_state[i]=NAN; out_nroots[i]=NAN; out_xout[i]=NAN;
        }
        return;
    }

    if (fabs(w) < weps) w = (w < 0.0 ? -weps : weps);

    const double b_eff = b * w;
    const double c_eff = c * w;
    const double d_eff = d * w;

    // quintic: x^5 + 0*x^4 + a*x^3 + b_eff*x^2 + c_eff*x + d_eff
    const double c4 = 0.0;
    const double c3 = a;
    const double c2 = b_eff;
    const double c1 = c_eff;
    const double c0 = d_eff;

    double roots[5] = {NAN, NAN, NAN, NAN, NAN};
    int nreal = solve_quintic_real_DK(c4, c3, c2, c1, c0, roots, eps);

    // choose x_out
    double xout = NAN;
    double state = -1.0;

    if (nreal <= 0) {
        if (x->mode == BF_MODE_HOLD) xout = x->x_prev;
        else if (x->mode == BF_MODE_ZERO) { xout = 0.0; x->x_prev = 0.0; }
        else xout = NAN;
        state = -1.0;
    } else {
        const double prev = x->x_prev;

        int best_any = 0;
        double best_any_d = fabs(roots[0] - prev);

        int found_stable = 0;
        int best_stable = 0;
        double best_stable_d = DBL_MAX;

        for (int k = 0; k < nreal; ++k) {
            double r = roots[k];
            double d0 = fabs(r - prev);

            if (d0 < best_any_d) { best_any_d = d0; best_any = k; }

            if (vpp(r, a, b_eff, c_eff) > eps) {
                if (d0 < best_stable_d) {
                    best_stable_d = d0;
                    best_stable = k;
                    found_stable = 1;
                }
            }
        }

        int chosen = found_stable ? best_stable : best_any;
        xout = roots[chosen];
        state = (double)chosen;
        x->x_prev = xout;
    }

    // prepare constant outputs
    double r0 = (nreal > 0) ? roots[0] : NAN;
    double r1 = (nreal > 1) ? roots[1] : NAN;
    double r2 = (nreal > 2) ? roots[2] : NAN;
    double r3 = (nreal > 3) ? roots[3] : NAN;
    double r4 = (nreal > 4) ? roots[4] : NAN;

    for (long i = 0; i < sampleframes; ++i) {
        out_r0[i] = r0;
        out_r1[i] = r1;
        out_r2[i] = r2;
        out_r3[i] = r3;
        out_r4[i] = r4;
        out_state[i]  = state;
        out_nroots[i] = (double)nreal;
        out_xout[i]   = xout;
    }
}

static void butterfly_dsp64(t_butterfly *x, t_object *dsp64, short *count,
                            double samplerate, long maxvectorsize, long flags)
{
    (void)samplerate; (void)maxvectorsize; (void)flags;

    x->a_connected = count[0];
    x->b_connected = count[1];
    x->c_connected = count[2];
    x->d_connected = count[3];
    x->w_connected = count[4];

    object_method(dsp64, gensym("dsp_add64"), x, butterfly_perform64, 0, NULL);
}

static void butterfly_free(t_butterfly *x)
{
    dsp_free((t_pxobject*)x);
}

static void *butterfly_new(t_symbol *s, long argc, t_atom *argv)
{
    (void)s;
    t_butterfly *x = (t_butterfly*)object_alloc(s_butterfly_class);
    if (!x) return NULL;

    dsp_setup((t_pxobject*)x, 5);

    // outlets created right-to-left:
    outlet_new((t_object*)x, "signal"); // x_out (rightmost)
    outlet_new((t_object*)x, "signal"); // nroots
    outlet_new((t_object*)x, "signal"); // state
    outlet_new((t_object*)x, "signal"); // r4
    outlet_new((t_object*)x, "signal"); // r3
    outlet_new((t_object*)x, "signal"); // r2
    outlet_new((t_object*)x, "signal"); // r1
    outlet_new((t_object*)x, "signal"); // r0 (leftmost)

    // defaults (musically usable)
    x->a = -3.0;
    x->b = 0.0;
    x->c = 1.0;
    x->d = 0.0;
    x->width = 1.0;

    x->eps = 1e-12;
    x->width_eps = 1e-12;
    x->x_prev = 0.0;
    x->mode = BF_MODE_HOLD;

    // optional args: a b c d width
    if (argc > 0) x->a = atom_getfloat(argv+0);
    if (argc > 1) x->b = atom_getfloat(argv+1);
    if (argc > 2) x->c = atom_getfloat(argv+2);
    if (argc > 3) x->d = atom_getfloat(argv+3);
    if (argc > 4) x->width = atom_getfloat(argv+4);

    attr_args_process(x, argc, argv);
    return x;
}

void ext_main(void *r)
{
    t_class *c;

    c = class_new("butterfly~",
                  (method)butterfly_new,
                  (method)butterfly_free,
                  (long)sizeof(t_butterfly),
                  0L, A_GIMME, 0);

    class_addmethod(c, (method)butterfly_dsp64,  "dsp64",  A_CANT, 0);
    class_addmethod(c, (method)butterfly_assist, "assist", A_CANT, 0);

    class_addmethod(c, (method)butterfly_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_set_a, "a",     A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_set_b, "b",     A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_set_c, "c",     A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_set_d, "d",     A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_set_w, "width", A_FLOAT, 0);

    class_addmethod(c, (method)butterfly_eps,       "eps",       A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_width_eps, "width_eps", A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_mode,      "mode",      A_SYM, 0);

    class_addmethod(c, (method)butterfly_reset, "reset", 0);
    class_addmethod(c, (method)butterfly_info,  "info",  0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    s_butterfly_class = c;

    post("butterfly~ loaded %s %s", __DATE__, __TIME__);
}
