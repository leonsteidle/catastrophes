/*
  butterfly~ (Thom butterfly catastrophe) — equilibria of:
    V'(x) = x^5 + a x^3 + b x^2 + c x + d = 0

  Stability:
    V''(x) = 5x^4 + 3a x^2 + 2b x + c    (stable if > 0)

  Canonical release 2.0.0:
   - Distinct real roots are isolated using the frozen numerical header.
   - See docs/MIGRATION.md for eps semantics and numerical limits.

  Conventions (catastrophes~):
   - Signal inlets, but if an inlet is unconnected we use stored parameter.
   - Sample-and-hold per vector: compute once using first sample of each inlet.
   - width scales b,c,d (a is not scaled): b_eff=b*width, c_eff=c*width, d_eff=d*width
   - Output selection x_out: prefer stable real root closest to x_prev; else closest root.
   - If no real roots found: mode = hold|zero|nan.

  Signal inlets (named messages set stored parameters):
    0 a, 1 b, 2 c, 3 d, 4 width

  Outlets (signal):
    0..4 r0..r4   (sorted real roots; unused = NAN)
    5    state    (chosen root index 0..4, or -1)
    6    nroots   (0..5)
    7    x_out

  Messages:
    a/b/c/d/width <f>, stability_eps <f>, eps <f> (legacy alias),
    width_eps <f>, mode <sym>, reset, info
    float (no selector) sets a
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <math.h>
#include <float.h>
#include <string.h>

#include "butterfly_real_solver.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BF_RELEASE_VERSION "2.0.0"
#define BF_SOLVER_SHA256 "3ab8e60fbdb58738de9306402cb2b40c37d8195a4adaf7c011d454ad3eb6d052"

#ifndef BF_MAX_CLASS_NAME
#define BF_MAX_CLASS_NAME "butterfly~"
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
    double eps;       // stability-classification epsilon (V'' > eps)
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


/* V''(x) = 5x^4 + 3a x^2 + 2b x + c  (here b,c are effective) */
static inline double vpp(double x, double a, double b_eff, double c_eff)
{
    double x2 = x * x;
    double x4 = x2 * x2;
    return 5.0 * x4 + 3.0 * a * x2 + 2.0 * b_eff * x + c_eff;
}


/* ---------------- Max methods ---------------- */

static void butterfly_assist(t_butterfly *x, void *b, long m, long a, char *s)
{
    (void)x; (void)b;
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0: snprintf(s, 256, "signal/float: a"); break;
            case 1: snprintf(s, 256, "signal: b; stored value: b <number> message"); break;
            case 2: snprintf(s, 256, "signal: c; stored value: c <number> message"); break;
            case 3: snprintf(s, 256, "signal: d; stored value: d <number> message"); break;
            case 4: snprintf(s, 256, "signal: width; stored value: width <number> message"); break;
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
    post("%s %s | solver SHA-256 %s", BF_MAX_CLASS_NAME, BF_RELEASE_VERSION, BF_SOLVER_SHA256);
    post("%s: a=%g b=%g c=%g d=%g width=%g stability_eps=%g width_eps=%g mode=%ld x_prev=%g",
         BF_MAX_CLASS_NAME, x->a, x->b, x->c, x->d, x->width,
         x->eps, x->width_eps, x->mode, x->x_prev);
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

    /* Individually finite inputs can overflow when width is applied. */
    if (!bf_isfinite(b_eff) || !bf_isfinite(c_eff) || !bf_isfinite(d_eff)) {
        for (long i = 0; i < sampleframes; ++i) {
            out_r0[i]=out_r1[i]=out_r2[i]=out_r3[i]=out_r4[i]=NAN;
            out_state[i]=NAN; out_nroots[i]=NAN; out_xout[i]=NAN;
        }
        return;
    }

    // quintic: x^5 + a*x^3 + b_eff*x^2 + c_eff*x + d_eff
    double roots[5] = {NAN, NAN, NAN, NAN, NAN};
    int nreal = bfri_solve_butterfly(a, b_eff, c_eff, d_eff, roots, NULL);

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

    // defaults
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

    c = class_new(BF_MAX_CLASS_NAME,
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

    class_addmethod(c, (method)butterfly_eps,       "stability_eps", A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_eps,       "eps",       A_FLOAT, 0); /* legacy alias */
    class_addmethod(c, (method)butterfly_width_eps, "width_eps", A_FLOAT, 0);
    class_addmethod(c, (method)butterfly_mode,      "mode",      A_SYM, 0);

    class_addmethod(c, (method)butterfly_reset, "reset", 0);
    class_addmethod(c, (method)butterfly_info,  "info",  0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    s_butterfly_class = c;

    post("%s %s loaded %s %s", BF_MAX_CLASS_NAME, BF_RELEASE_VERSION, __DATE__, __TIME__);
}
