/*
 cusp~ (Thom cusp catastrophe) — audio-rate equilibria for the canonical cusp potential

 Potential:
   V(x)  = x^4/4 + (beta/2) x^2 + alpha x
 Equilibria:
   V'(x) = x^3 + p x + q = 0   where p = beta * width, q = alpha
 Stability:
   V''(x)= 3x^2 + p   (stable minima when > 0)

 Inlets (signal/float):
   1: alpha
   2: beta
   3: width (default 1.0 when unconnected; clamped away from 0)

 Outlets (signal):
   1: r0    (lowest real root)
   2: r1    (middle real root, when distinct)
   3: r2    (highest real root)
   4: state (0=low branch selected, 1=high branch selected)
   5: disc  (discriminant D)
   6: nroots (distinct real roots: 1,2,3)
   7: x_out (selected stable equilibrium with continuity/hysteresis)

 Messages:
   alpha <f>, beta <f>, width <f>, eps <f>, reset, info
   float (no selector) sets alpha (like fold~ sets a)

 Notes:
   - Robust cubic handling near D≈0, avoids NaN when p>0.
   - No proxy_getinlet() needed; use typed messages for beta/width.
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct _cusp {
    t_pxobject x_obj;

    // stored parameters (used when inlet is unconnected)
    double alpha;
    double beta;
    double width;

    // numerical controls
    double eps;        // discriminant tolerance (>=0)
    double width_eps;  // clamp for |width| (>=0)

    // continuity / hysteresis memory
    double x_prev;

    // inlet connection flags
    long alpha_connected;
    long beta_connected;
    long width_connected;
} t_cusp;

static t_class *s_cusp_class = NULL;

// ---------- helpers ----------

static int cusp_isfinite(double x)
{
    return (x == x) && (x <= DBL_MAX) && (x >= -DBL_MAX);
}

static double cusp_clamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void cusp_sort3(double *a, double *b, double *c)
{
    double x = *a, y = *b, z = *c;
    if (x > y) { double t = x; x = y; y = t; }
    if (y > z) { double t = y; y = z; z = t; }
    if (x > y) { double t = x; x = y; y = t; }
    *a = x; *b = y; *c = z;
}

/*
 Solve depressed cubic: x^3 + p x + q = 0
 Returns distinct real root count: 1,2,3
 Always writes r0,r1,r2 (duplicates possible when count < 3)
 Writes disc_out = D
*/
static int cusp_solve_cubic(double p, double q, double epsD,
                            double *r0, double *r1, double *r2,
                            double *disc_out)
{
    const double D = (q*q)/4.0 + (p*p*p)/27.0;
    if (disc_out) *disc_out = D;

    // Robust regime selection
    if (D > epsD) {
        // 1 real root (Cardano)
        const double sqrtD = sqrt(D);
        const double u = cbrt(-q/2.0 + sqrtD);
        const double v = cbrt(-q/2.0 - sqrtD);
        const double x = u + v;
        *r0 = *r1 = *r2 = x;
        return 1;
    }

    if (D < -epsD) {
        // 3 distinct real roots (trigonometric), requires p < 0
        if (p >= 0.0) {
            // fallback: treat as 1 real root (numerical weirdness)
            const double sqrtD = sqrt(fmax(D, 0.0));
            const double u = cbrt(-q/2.0 + sqrtD);
            const double v = cbrt(-q/2.0 - sqrtD);
            const double x = u + v;
            *r0 = *r1 = *r2 = x;
            return 1;
        }

        const double two_sqrt = 2.0 * sqrt(-p/3.0);
        const double denom = sqrt(-(p*p*p)/27.0);  // >0

        double arg = (-q/2.0) / denom;
        arg = cusp_clamp(arg, -1.0, 1.0);

        const double phi = acos(arg);
        const double x0 = two_sqrt * cos(phi/3.0);
        const double x1 = two_sqrt * cos((phi + 2.0*M_PI)/3.0);
        const double x2 = two_sqrt * cos((phi + 4.0*M_PI)/3.0);

        *r0 = x0; *r1 = x1; *r2 = x2;
        return 3;
    }

    // |D| <= epsD : multiple roots (degenerate) or numerical boundary.
    // If p and q ~ 0: triple root at 0.
    const double eps0 = 1e-14;
    if (fabs(p) <= eps0 && fabs(q) <= eps0) {
        *r0 = *r1 = *r2 = 0.0;
        return 1;
    }

    // If p ~ 0: x^3 + q = 0
    if (fabs(p) <= eps0) {
        const double x = cbrt(-q);
        *r0 = *r1 = *r2 = x;
        return 1;
    }

    // Classic D=0 case: one single + one double root.
    // Using u = cbrt(-q/2), roots: x1=2u, x2=x3=-u
    const double u = cbrt(-q/2.0);
    const double x1 = 2.0 * u;
    const double x2 = -u;

    *r0 = x1;
    *r1 = x2;
    *r2 = x2;
    return 2;
}

// ---------- Max methods ----------

static void cusp_assist(t_cusp *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) {
        switch (a) {
            case 0: snprintf_zero(s, 512, "signal/float: alpha"); break;
            case 1: snprintf_zero(s, 512, "signal/float: beta"); break;
            case 2: snprintf_zero(s, 512, "signal/float: width (default 1 when unconnected)"); break;
        }
    } else {
        switch (a) {
            case 0: snprintf_zero(s, 512, "signal: r0 (lowest real root)"); break;
            case 1: snprintf_zero(s, 512, "signal: r1 (middle real root)"); break;
            case 2: snprintf_zero(s, 512, "signal: r2 (highest real root)"); break;
            case 3: snprintf_zero(s, 512, "signal: state (0 low branch / 1 high branch)"); break;
            case 4: snprintf_zero(s, 512, "signal: disc (discriminant D)"); break;
            case 5: snprintf_zero(s, 512, "signal: nroots (distinct real roots: 1/2/3)"); break;
            case 6: snprintf_zero(s, 512, "signal: x_out (selected stable equilibrium)"); break;
        }
    }
}

static void cusp_float(t_cusp *x, double f)
{
    // padrão: float sem seletor controla alpha (como fold~ controla a)
    x->alpha = f;
}

static void cusp_alpha(t_cusp *x, double f) { x->alpha = f; }
static void cusp_beta(t_cusp *x, double f)  { x->beta  = f; }
static void cusp_width(t_cusp *x, double f) { x->width = f; }

static void cusp_reset(t_cusp *x) { x->x_prev = 0.0; }

static void cusp_eps(t_cusp *x, double f)
{
    if (f < 0.0) f = 0.0;
    x->eps = f;
}

static void cusp_info(t_cusp *x)
{
    post("cusp~ build %s %s | alpha=%.6g beta=%.6g width=%.6g eps=%.3g width_eps=%.3g",
         __DATE__, __TIME__, x->alpha, x->beta, x->width, x->eps, x->width_eps);
}

static void cusp_perform64(t_cusp *x, t_object *dsp64,
                           double **ins, long numins, double **outs, long numouts,
                           long sampleframes, long flags, void *userparam)
{
    double *in_alpha = ins[0];
    double *in_beta  = ins[1];
    double *in_width = ins[2];

    double *out_r0     = outs[0];
    double *out_r1     = outs[1];
    double *out_r2     = outs[2];
    double *out_state  = outs[3];
    double *out_disc   = outs[4];
    double *out_nroots = outs[5];
    double *out_xout   = outs[6];

    double x_prev = x->x_prev;
    const double epsD = (x->eps >= 0.0) ? x->eps : 0.0;
    const double weps = (x->width_eps > 0.0) ? x->width_eps : 1e-12;

    for (long i = 0; i < sampleframes; i++) {

        double alpha = x->alpha_connected ? in_alpha[i] : x->alpha;
        double beta  = x->beta_connected  ? in_beta[i]  : x->beta;
        double width = x->width_connected ? in_width[i] : x->width;

        if (!cusp_isfinite(alpha) || !cusp_isfinite(beta) || !cusp_isfinite(width)) {
            out_r0[i] = out_r1[i] = out_r2[i] = NAN;
            out_state[i]  = NAN;
            out_disc[i]   = NAN;
            out_nroots[i] = NAN;
            out_xout[i]   = NAN;
            continue;
        }

        // clamp width away from 0 to avoid accidental nulling when unconnected/0
        if (fabs(width) < weps) width = (width >= 0.0 ? weps : -weps);

        // SBCM convention preserved:
        const double p = beta * width;
        const double q = alpha;

        double r0, r1, r2, D;
        const int nroots = cusp_solve_cubic(p, q, epsD, &r0, &r1, &r2, &D);

        cusp_sort3(&r0, &r1, &r2);

        out_r0[i] = r0;
        out_r1[i] = r1;
        out_r2[i] = r2;
        out_disc[i]   = D;
        out_nroots[i] = (double)nroots;

        // default selection
        double x_out = r0;
        double state = (x_out >= 0.0) ? 1.0 : 0.0;

        if (nroots == 3) {
            // stable minima: usually outer roots; verify via V''(x)=3x^2+p
            const double v2_0 = 3.0*r0*r0 + p;
            const double v2_2 = 3.0*r2*r2 + p;

            const int stable0 = (v2_0 > 0.0);
            const int stable2 = (v2_2 > 0.0);

            if (stable0 && stable2) {
                const double d0 = fabs(x_prev - r0);
                const double d2 = fabs(x_prev - r2);
                if (d0 <= d2) { x_out = r0; state = 0.0; }
                else          { x_out = r2; state = 1.0; }
            } else if (stable0 && !stable2) {
                x_out = r0; state = 0.0;
            } else if (!stable0 && stable2) {
                x_out = r2; state = 1.0;
            } else {
                // fallback: continuity on outer roots
                const double d0 = fabs(x_prev - r0);
                const double d2 = fabs(x_prev - r2);
                if (d0 <= d2) { x_out = r0; state = 0.0; }
                else          { x_out = r2; state = 1.0; }
            }
        } else if (nroots == 2) {
            // Degenerate boundary: pick the closer of the two distinct roots
            // Distinct roots are: r0 and r1 (since r1==r2 or r0==r1 depending on sort)
            // We'll treat outermost candidates r0 and r2 for continuity.
            const double d0 = fabs(x_prev - r0);
            const double d2 = fabs(x_prev - r2);
            if (d0 <= d2) { x_out = r0; state = 0.0; }
            else          { x_out = r2; state = 1.0; }
        }

        out_state[i] = state;
        out_xout[i]  = x_out;

        x_prev = x_out;
    }

    x->x_prev = x_prev;
}

static void cusp_dsp64(t_cusp *x, t_object *dsp64, short *count,
                       double samplerate, long maxvectorsize, long flags)
{
    x->alpha_connected = (count[0] != 0);
    x->beta_connected  = (count[1] != 0);
    x->width_connected = (count[2] != 0);

    object_method(dsp64, gensym("dsp_add64"), x, cusp_perform64, 0, NULL);
}

static void cusp_free(t_cusp *x)
{
    dsp_free((t_pxobject *)x);
}

static void *cusp_new(t_symbol *s, long argc, t_atom *argv)
{
    t_cusp *x = (t_cusp *)object_alloc(s_cusp_class);
    if (!x) return NULL;

    dsp_setup((t_pxobject *)x, 3);

    // defaults
    x->alpha = 0.0;
    x->beta  = 0.0;
    x->width = 1.0;
    x->eps   = 1e-12;
    x->width_eps = 1e-12;
    x->x_prev = 0.0;

    // outlets: r0 r1 r2 state disc nroots x_out
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");

    attr_args_process(x, argc, argv);
    return x;
}

void ext_main(void *r)
{
    t_class *c;

    c = class_new("cusp~",
                  (method)cusp_new,
                  (method)cusp_free,
                  (long)sizeof(t_cusp),
                  0L,
                  A_GIMME, 0);

    class_addmethod(c, (method)cusp_dsp64,  "dsp64",  A_CANT, 0);
    class_addmethod(c, (method)cusp_assist, "assist", A_CANT, 0);

    class_addmethod(c, (method)cusp_float,  "float",  A_FLOAT, 0);
    class_addmethod(c, (method)cusp_alpha,  "alpha",  A_FLOAT, 0);
    class_addmethod(c, (method)cusp_beta,   "beta",   A_FLOAT, 0);
    class_addmethod(c, (method)cusp_width,  "width",  A_FLOAT, 0);

    class_addmethod(c, (method)cusp_reset,  "reset",  0);
    class_addmethod(c, (method)cusp_eps,    "eps",    A_FLOAT, 0);
    class_addmethod(c, (method)cusp_info,   "info",   0);

    // attributes (saved)
    CLASS_ATTR_DOUBLE(c, "alpha", 0, t_cusp, alpha);
    CLASS_ATTR_SAVE(c, "alpha", 0);

    CLASS_ATTR_DOUBLE(c, "beta", 0, t_cusp, beta);
    CLASS_ATTR_SAVE(c, "beta", 0);

    CLASS_ATTR_DOUBLE(c, "width", 0, t_cusp, width);
    CLASS_ATTR_SAVE(c, "width", 0);

    CLASS_ATTR_DOUBLE(c, "eps", 0, t_cusp, eps);
    CLASS_ATTR_LABEL(c, "eps", 0, "Discriminant tolerance");
    CLASS_ATTR_SAVE(c, "eps", 0);

    CLASS_ATTR_DOUBLE(c, "width_eps", 0, t_cusp, width_eps);
    CLASS_ATTR_LABEL(c, "width_eps", 0, "Clamp for |width|");
    CLASS_ATTR_SAVE(c, "width_eps", 0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    s_cusp_class = c;
}
