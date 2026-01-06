/*
 swallowtail~ (Thom swallowtail catastrophe) — audio-rate equilibria for:

    V(x) = x^5/5 + (a/3)x^3 + (b/2)x^2 + c x
    V'(x)= x^4 + a x^2 + b x + c = 0

 Design (padronizado catastrophes~):
  - Inlets são signal, mas se desconectados usam valores armazenados.
  - width escala b e c: b_eff=b*width, c_eff=c*width (a não escala).
  - Cálculo é por vetor (sample-and-hold no 1º sample do bloco).
  - Escolha do x_out: preferir raiz estável (V''>0) mais próxima de x_prev; fallback: raiz mais próxima.

 Inlets (signal/float via msgs):
  0: a
  1: b
  2: c
  3: width

 Outlets (signal):
  0: r0 (menor raiz real)
  1: r1
  2: r2
  3: r3 (maior raiz real)
  4: state  (índice da raiz escolhida: 0..3; -1 se nroots==0)
  5: nroots (0..4)
  6: x_out  (equilíbrio escolhido)

 Messages:
  a <f>, b <f>, c <f>, width <f>
  eps <f>        : tolerância (default 1e-12, clamp >0)
  width_eps <f>  : clamp mínimo |width| (default 1e-12)
  mode <sym>     : hold | zero | nan  (quando nroots==0; default hold)
  reset          : x_prev=0
  info

 float (sem seletor): seta "a" (padrão como fold~/cusp~)
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <math.h>
#include <float.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    SW_MODE_HOLD = 0,
    SW_MODE_ZERO = 1,
    SW_MODE_NAN  = 2
} t_sw_mode;

typedef struct _swallowtail {
    t_pxobject x_obj;

    // stored params (usados quando inlet desconectado)
    double a, b, c, width;

    // numerics + state
    double eps;
    double width_eps;
    double x_prev;
    long   mode;

    // signal connection flags
    short a_connected;
    short b_connected;
    short c_connected;
    short w_connected;
} t_swallowtail;

static t_class *s_swallowtail_class = NULL;

// ---------- utilities ----------

static int sw_isfinite(double x)
{
    return (x == x) && (x <= DBL_MAX) && (x >= -DBL_MAX);
}

static double sw_clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int cmp_dbl(const void *pa, const void *pb)
{
    const double a = *(const double *)pa;
    const double b = *(const double *)pb;
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

static int dedup_sorted(double *x, int n, double eps)
{
    if (n <= 1) return n;
    int w = 1;
    for (int i = 1; i < n; ++i) {
        if (fabs(x[i] - x[w-1]) > eps) x[w++] = x[i];
    }
    return w;
}

static double clamp_width(double w, double weps)
{
    if (!sw_isfinite(w)) return 1.0;
    if (fabs(w) < weps) return (w < 0.0 ? -weps : weps);
    return w;
}

// P(x) = x^4 + a x^2 + b x + c
static double poly4(double x, double a, double b, double c)
{
    // Horner: (((x^2 + a)*x + b)*x + c) after initial x*x:
    // Equivalent monic quartic: x^4 + a x^2 + b x + c
    return ((((x) * x + a) * x + b) * x + c);
}

// V''(x) = P'(x) = 4x^3 + 2ax + b
static double dpoly4(double x, double a, double b)
{
    return (4.0 * x * x * x + 2.0 * a * x + b);
}

// Cardano para cúbica deprimida: t^3 + p t + q = 0
// retorna nroots (1..3) e escreve out[] (ordenado quando n==3, ou n==2)
static int solve_cubic_depressed(double p, double q, double out[3], double eps)
{
    const double half_q  = 0.5 * q;
    const double third_p = p / 3.0;
    const double disc = half_q*half_q + third_p*third_p*third_p;

    if (disc > eps) {
        const double s = sqrt(disc);
        const double u = cbrt(-half_q + s);
        const double v = cbrt(-half_q - s);
        out[0] = u + v;
        return 1;
    }

    if (fabs(disc) <= eps) {
        const double u = cbrt(-half_q);
        out[0] = 2.0*u;
        out[1] = -u;
        if (fabs(out[0] - out[1]) <= eps) return 1;
        if (out[1] < out[0]) { double tmp = out[0]; out[0]=out[1]; out[1]=tmp; }
        return 2;
    }

    // disc < 0: 3 reais (requer p < 0 idealmente). Guardas anti-NaN:
    const double r2 = -third_p;
    if (r2 <= eps) { // fallback seguro
        out[0] = cbrt(-q);
        return 1;
    }

    const double r = sqrt(r2);
    const double denom = r*r*r;
    double arg = (-half_q) / denom;
    arg = sw_clamp(arg, -1.0, 1.0);

    const double phi = acos(arg);
    out[0] = 2.0 * r * cos(phi / 3.0);
    out[1] = 2.0 * r * cos((phi + 2.0*M_PI) / 3.0);
    out[2] = 2.0 * r * cos((phi + 4.0*M_PI) / 3.0);
    qsort(out, 3, sizeof(double), cmp_dbl);
    return 3;
}

// bisseção (assume f(x0)*f(x1) <= 0)
static double bisect_root(double acoef, double bcoef, double ccoef,
                          double x0, double x1, double f0, double f1,
                          double eps)
{
    double lo = x0, hi = x1;
    double flo = f0, fhi = f1;

    if (fabs(flo) <= eps) return lo;
    if (fabs(fhi) <= eps) return hi;

    for (int it = 0; it < 80; ++it) {
        const double mid = 0.5 * (lo + hi);
        const double fmid = poly4(mid, acoef, bcoef, ccoef);

        if (fabs(fmid) <= eps) return mid;

        if ((flo < 0.0 && fmid > 0.0) || (flo > 0.0 && fmid < 0.0)) {
            hi = mid; fhi = fmid;
        } else {
            lo = mid; flo = fmid;
        }
    }
    return 0.5 * (lo + hi);
}

// encontra raízes reais de x^4 + a x^2 + b x + c = 0
static int solve_quartic_real(double a, double b, double c, double roots[4], double eps)
{
    // Cauchy bound para polinômio mônico: R = 1 + max(|coef|)
    const double R = 1.0 + fmax(fmax(fabs(a), fabs(b)), fabs(c));

    // derivada: 4x^3 + 2ax + b = 0  => x^3 + (a/2)x + (b/4)=0
    double crit[3] = {0,0,0};
    int ncrit = solve_cubic_depressed(a/2.0, b/4.0, crit, eps);

    double pts[5];
    int npts = 0;
    pts[npts++] = -R;
    for (int i = 0; i < ncrit; ++i) pts[npts++] = crit[i];
    pts[npts++] = R;

    qsort(pts, npts, sizeof(double), cmp_dbl);
    npts = dedup_sorted(pts, npts, eps*10.0);

    int nroots = 0;

    // múltiplas em pontos críticos
    for (int i = 1; i < npts-1 && nroots < 4; ++i) {
        const double x = pts[i];
        const double fx = poly4(x, a, b, c);
        if (fabs(fx) <= eps) roots[nroots++] = x;
    }

    // mudanças de sinal em intervalos monótonos
    for (int i = 0; i < npts-1 && nroots < 4; ++i) {
        const double x0 = pts[i];
        const double x1 = pts[i+1];
        const double f0 = poly4(x0, a, b, c);
        const double f1 = poly4(x1, a, b, c);
        if (!sw_isfinite(f0) || !sw_isfinite(f1)) continue;

        if ((f0 < 0.0 && f1 > 0.0) || (f0 > 0.0 && f1 < 0.0)) {
            roots[nroots++] = bisect_root(a, b, c, x0, x1, f0, f1, eps);
        }
    }

    if (nroots == 0) return 0;

    qsort(roots, nroots, sizeof(double), cmp_dbl);
    nroots = dedup_sorted(roots, nroots, eps*10.0);
    return nroots;
}

// ---------- Max methods ----------

static void swallowtail_assist(t_swallowtail *x, void *b, long m, long a, char *dst)
{
    (void)x; (void)b;
    const long n = 256;
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0: snprintf(dst, n, "signal/float: a"); break;
            case 1: snprintf(dst, n, "signal/float: b"); break;
            case 2: snprintf(dst, n, "signal/float: c"); break;
            case 3: snprintf(dst, n, "signal/float: width (default 1 if unconnected)"); break;
        }
    } else {
        switch (a) {
            case 0: snprintf(dst, n, "signal: r0"); break;
            case 1: snprintf(dst, n, "signal: r1"); break;
            case 2: snprintf(dst, n, "signal: r2"); break;
            case 3: snprintf(dst, n, "signal: r3"); break;
            case 4: snprintf(dst, n, "signal: state (chosen root index 0..3, -1 if none)"); break;
            case 5: snprintf(dst, n, "signal: nroots (0..4)"); break;
            case 6: snprintf(dst, n, "signal: x_out (chosen equilibrium)"); break;
        }
    }
}

static void swallowtail_float(t_swallowtail *x, double f) { x->a = f; }
static void swallowtail_set_a(t_swallowtail *x, double v) { x->a = v; }
static void swallowtail_set_b(t_swallowtail *x, double v) { x->b = v; }
static void swallowtail_set_c(t_swallowtail *x, double v) { x->c = v; }
static void swallowtail_set_w(t_swallowtail *x, double v) { x->width = v; }

static void swallowtail_reset(t_swallowtail *x) { x->x_prev = 0.0; }

static void swallowtail_eps(t_swallowtail *x, double e)
{
    if (sw_isfinite(e) && e > 0.0) x->eps = e;
}

static void swallowtail_width_eps(t_swallowtail *x, double e)
{
    if (sw_isfinite(e) && e > 0.0) x->width_eps = e;
}

static void swallowtail_mode(t_swallowtail *x, t_symbol *s)
{
    if (s == gensym("hold")) x->mode = SW_MODE_HOLD;
    else if (s == gensym("zero")) x->mode = SW_MODE_ZERO;
    else if (s == gensym("nan")) x->mode = SW_MODE_NAN;
    else object_warn((t_object*)x, "mode: use hold | zero | nan");
}

static void swallowtail_info(t_swallowtail *x)
{
    post("swallowtail~: a=%g b=%g c=%g width=%g eps=%g width_eps=%g mode=%ld x_prev=%g",
         x->a, x->b, x->c, x->width, x->eps, x->width_eps, x->mode, x->x_prev);
    post("             connected: a=%d b=%d c=%d width=%d",
         (int)x->a_connected, (int)x->b_connected, (int)x->c_connected, (int)x->w_connected);
}

static void swallowtail_perform64(t_swallowtail *x, t_object *dsp64,
                                  double **ins, long numins, double **outs, long numouts,
                                  long sampleframes, long flags, void *userparam)
{
    (void)dsp64; (void)numins; (void)numouts; (void)flags; (void)userparam;

    const double *in_a = ins[0];
    const double *in_b = ins[1];
    const double *in_c = ins[2];
    const double *in_w = ins[3];

    double *out_r0 = outs[0];
    double *out_r1 = outs[1];
    double *out_r2 = outs[2];
    double *out_r3 = outs[3];
    double *out_state = outs[4];
    double *out_n  = outs[5];
    double *out_x  = outs[6];

    // block-rate (1º sample)
    const double a = x->a_connected ? in_a[0] : x->a;
    const double b_raw = x->b_connected ? in_b[0] : x->b;
    const double c_raw = x->c_connected ? in_c[0] : x->c;
    const double w_raw = x->w_connected ? in_w[0] : x->width;

    const double eps = (x->eps > 0.0 ? x->eps : 1e-12);
    const double weps = (x->width_eps > 0.0 ? x->width_eps : 1e-12);

    double w = clamp_width(w_raw, weps);
    double b = b_raw * w;
    double c = c_raw * w;

    double roots[4] = {NAN, NAN, NAN, NAN};
    int nroots = 0;

    if (sw_isfinite(a) && sw_isfinite(b) && sw_isfinite(c)) {
        nroots = solve_quartic_real(a, b, c, roots, eps);
    }

    double xout = NAN;
    double state = -1.0;

    if (nroots <= 0) {
        if (x->mode == SW_MODE_HOLD) xout = x->x_prev;
        else if (x->mode == SW_MODE_ZERO) { xout = 0.0; x->x_prev = 0.0; }
        else xout = NAN;
        state = -1.0;
    } else {
        const double prev = x->x_prev;

        // fallback: raiz mais próxima
        int best_any_k = 0;
        double best_any = roots[0];
        double best_any_d = fabs(best_any - prev);

        // preferir estáveis
        int found_stable = 0;
        int best_stable_k = 0;
        double best_stable = roots[0];
        double best_stable_d = DBL_MAX;

        for (int k = 0; k < nroots; ++k) {
            const double r = roots[k];
            const double d = fabs(r - prev);

            if (d < best_any_d) { best_any_d = d; best_any = r; best_any_k = k; }

            const double vpp = dpoly4(r, a, b); // V''(r)
            if (vpp > eps) {
                if (d < best_stable_d) {
                    best_stable_d = d;
                    best_stable = r;
                    best_stable_k = k;
                    found_stable = 1;
                }
            }
        }

        if (found_stable) { xout = best_stable; state = (double)best_stable_k; }
        else              { xout = best_any;    state = (double)best_any_k; }

        x->x_prev = xout;
    }

    const double r0 = (nroots > 0) ? roots[0] : NAN;
    const double r1 = (nroots > 1) ? roots[1] : NAN;
    const double r2 = (nroots > 2) ? roots[2] : NAN;
    const double r3 = (nroots > 3) ? roots[3] : NAN;

    for (long i = 0; i < sampleframes; ++i) {
        out_r0[i] = r0;
        out_r1[i] = r1;
        out_r2[i] = r2;
        out_r3[i] = r3;
        out_state[i] = state;
        out_n[i]  = (double)nroots;
        out_x[i]  = xout;
    }
}

static void swallowtail_dsp64(t_swallowtail *x, t_object *dsp64, short *count,
                             double samplerate, long maxvectorsize, long flags)
{
    (void)samplerate; (void)maxvectorsize; (void)flags;

    x->a_connected = count[0];
    x->b_connected = count[1];
    x->c_connected = count[2];
    x->w_connected = count[3];

    object_method(dsp64, gensym("dsp_add64"), x, swallowtail_perform64, 0, NULL);
}

static void *swallowtail_new(t_symbol *s, long argc, t_atom *argv)
{
    (void)s;
    t_swallowtail *x = (t_swallowtail *)object_alloc(s_swallowtail_class);
    if (!x) return NULL;

    dsp_setup((t_pxobject *)x, 4);

    // outlets: criar da direita pra esquerda (último criado = mais à esquerda)
    outlet_new((t_object *)x, "signal"); // x_out
    outlet_new((t_object *)x, "signal"); // nroots
    outlet_new((t_object *)x, "signal"); // state
    outlet_new((t_object *)x, "signal"); // r3
    outlet_new((t_object *)x, "signal"); // r2
    outlet_new((t_object *)x, "signal"); // r1
    outlet_new((t_object *)x, "signal"); // r0

    // defaults
    x->a = 0.0;
    x->b = 0.0;
    x->c = 0.0;
    x->width = 1.0;
    x->eps = 1e-12;
    x->width_eps = 1e-12;
    x->x_prev = 0.0;
    x->mode = SW_MODE_HOLD;

    // optional args: a b c width
    if (argc > 0) x->a = atom_getfloat(argv + 0);
    if (argc > 1) x->b = atom_getfloat(argv + 1);
    if (argc > 2) x->c = atom_getfloat(argv + 2);
    if (argc > 3) x->width = atom_getfloat(argv + 3);

    return x;
}

static void swallowtail_free(t_swallowtail *x)
{
    dsp_free((t_pxobject *)x);
}

void ext_main(void *r)
{
    t_class *c;

    c = class_new("swallowtail~",
                  (method)swallowtail_new,
                  (method)swallowtail_free,
                  (long)sizeof(t_swallowtail),
                  0L,
                  A_GIMME, 0);

    class_addmethod(c, (method)swallowtail_dsp64,  "dsp64",  A_CANT, 0);
    class_addmethod(c, (method)swallowtail_assist, "assist", A_CANT, 0);

    class_addmethod(c, (method)swallowtail_float,  "float",  A_FLOAT, 0);

    class_addmethod(c, (method)swallowtail_set_a,  "a",      A_FLOAT, 0);
    class_addmethod(c, (method)swallowtail_set_b,  "b",      A_FLOAT, 0);
    class_addmethod(c, (method)swallowtail_set_c,  "c",      A_FLOAT, 0);
    class_addmethod(c, (method)swallowtail_set_w,  "width",  A_FLOAT, 0);

    class_addmethod(c, (method)swallowtail_reset,  "reset",  0);
    class_addmethod(c, (method)swallowtail_eps,    "eps",    A_FLOAT, 0);
    class_addmethod(c, (method)swallowtail_width_eps, "width_eps", A_FLOAT, 0);
    class_addmethod(c, (method)swallowtail_mode,   "mode",   A_SYM, 0);
    class_addmethod(c, (method)swallowtail_info,   "info",   0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    s_swallowtail_class = c;
}
