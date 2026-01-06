/*
    fold~ (Thom fold catastrophe) – audio-rate equilibria

    V(x)  = x^3 / 3 + a x
    V'(x) = x^2 + a

    Com width:
        a_eff = a * width

    Regras:
      - Para a_eff < 0: duas raízes reais x = ±sqrt(-a_eff)
        r0 = raiz negativa  (instável)
        r1 = raiz positiva  (estável)
        x_out = r1 (estável)

      - Para |a_eff| <= eps: caso degenerado (raiz dupla em 0)
        r0 = r1 = 0, nroots = 1, state = 2, x_out = 0

      - Para a_eff > 0: nenhuma raiz real
        r0 = r1 = 0, nroots = 0, state = 0,
        x_out depende de mode: hold | zero | nan

    Inlets:
      1: a      (signal/float)
      2: width  (signal/float)

    Outlets:
      1: r0      (signal)
      2: r1      (signal)
      3: state   (signal)
      4: nroots  (signal)
      5: x_out   (signal)
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <math.h>
#include <float.h>
#include <stdio.h>

typedef struct _fold {
    t_pxobject  x_obj;

    // parâmetros armazenados (quando inlet está desconectado)
    double      a;
    double      width;

    // parâmetros numéricos
    double      eps;
    long        mode;           // 0 = hold, 1 = zero, 2 = nan

    // flags de conexão de sinal
    long        a_connected;
    long        width_connected;

    // memória do último x_out (para modo hold)
    double      x_prev;
} t_fold;

static t_class *s_fold_class = NULL;

// modos
enum {
    FOLD_MODE_HOLD = 0,
    FOLD_MODE_ZERO,
    FOLD_MODE_NAN
};

// -----------------------------------------------------------------------------
// helpers numéricos
// -----------------------------------------------------------------------------

// checa finitude de maneira portável
static int fold_isfinite(double x)
{
    return (x == x) && (x <= DBL_MAX) && (x >= -DBL_MAX);
}

// -----------------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------------

void    ext_main(void *r);

static void *fold_new(t_symbol *s, long argc, t_atom *argv);
static void  fold_free(t_fold *x);

static void  fold_assist(t_fold *x, void *b, long m, long a, char *s);

static void  fold_dsp64(t_fold *x, t_object *dsp64,
                        short *count, double samplerate,
                        long maxvectorsize, long flags);
static void  fold_perform64(t_fold *x, t_object *dsp64,
                            double **ins, long numins,
                            double **outs, long numouts,
                            long sampleframes,
                            long flags, void *userparam);

// mensagens/atributos
static void  fold_float(t_fold *x, double f);
static void  fold_set_a(t_fold *x, double f);
static void  fold_set_width(t_fold *x, double f);
static void  fold_reset(t_fold *x);

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

void ext_main(void *r)
{
    t_class *c;

    c = class_new("fold~",
                  (method)fold_new,
                  (method)fold_free,
                  (long)sizeof(t_fold),
                  0L,
                  A_GIMME,
                  0);

    class_dspinit(c);

    class_addmethod(c, (method)fold_dsp64,  "dsp64",  A_CANT, 0);
    class_addmethod(c, (method)fold_assist, "assist", A_CANT, 0);

    class_addmethod(c, (method)fold_float,  "float",  A_FLOAT, 0);
    class_addmethod(c, (method)fold_set_a,      "a",      A_FLOAT, 0);
    class_addmethod(c, (method)fold_set_width,  "width",  A_FLOAT, 0);
    class_addmethod(c, (method)fold_reset,  "reset",  0);

    // atributos
    CLASS_ATTR_DOUBLE(c, "eps", 0, t_fold, eps);
    CLASS_ATTR_LABEL(c, "eps", 0, "Numerical epsilon");
    CLASS_ATTR_SAVE(c, "eps", 0);

    CLASS_ATTR_LONG(c, "mode", 0, t_fold, mode);
    CLASS_ATTR_ENUMINDEX(c, "mode", 0, "hold zero nan");
    CLASS_ATTR_LABEL(c, "mode", 0, "Behaviour when there are no real roots");
    CLASS_ATTR_SAVE(c, "mode", 0);

    class_register(CLASS_BOX, c);
    s_fold_class = c;
}

// -----------------------------------------------------------------------------
// new / free
// -----------------------------------------------------------------------------

static void *fold_new(t_symbol *s, long argc, t_atom *argv)
{
    t_fold *x = (t_fold *)object_alloc(s_fold_class);
    if (!x)
        return NULL;

    dsp_setup((t_pxobject *)x, 2); // 2 inlets de sinal: a, width

    // 5 outlets de sinal: r0, r1, state, nroots, x_out
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");
    outlet_new((t_object *)x, "signal");

    x->a     = 0.0;
    x->width = 1.0;
    x->eps   = 1e-9;
    x->mode  = FOLD_MODE_HOLD;

    x->a_connected     = 0;
    x->width_connected = 0;

    x->x_prev = 0.0;

    attr_args_process(x, argc, argv);

    return x;
}

static void fold_free(t_fold *x)
{
    dsp_free((t_pxobject *)x);
}

// -----------------------------------------------------------------------------
// assist
// -----------------------------------------------------------------------------

static void fold_assist(t_fold *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) {
        switch (a) {
        case 0:
            snprintf(s, 256,
                     "Inlet 1 (signal/float): a — fold control parameter");
            break;
        case 1:
            snprintf(s, 256,
                     "Inlet 2 (signal/float): width — scaling factor for a");
            break;
        }
    } else { // outlets
        switch (a) {
        case 0:
            snprintf(s, 256,
                     "Outlet 1 (signal): r0 — negative (unstable) equilibrium");
            break;
        case 1:
            snprintf(s, 256,
                     "Outlet 2 (signal): r1 — positive (stable) equilibrium");
            break;
        case 2:
            snprintf(s, 256,
                     "Outlet 3 (signal): state — 0=no roots, 1=fold, 2=degenerate");
            break;
        case 3:
            snprintf(s, 256,
                     "Outlet 4 (signal): nroots — number of real equilibria");
            break;
        case 4:
            snprintf(s, 256,
                     "Outlet 5 (signal): x_out — selected equilibrium");
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// mensagens/atributos simples
// -----------------------------------------------------------------------------

// float sem seletor → controla 'a' (inlet principal)
static void fold_float(t_fold *x, double f)
{
    x->a = f;
}

static void fold_set_a(t_fold *x, double f)
{
    x->a = f;
}

static void fold_set_width(t_fold *x, double f)
{
    x->width = f;
}

static void fold_reset(t_fold *x)
{
    x->x_prev = 0.0;
}

// -----------------------------------------------------------------------------
// DSP
// -----------------------------------------------------------------------------

static void fold_dsp64(t_fold *x, t_object *dsp64,
                       short *count, double samplerate,
                       long maxvectorsize, long flags)
{
    x->a_connected     = (count[0] != 0);
    x->width_connected = (count[1] != 0);

    object_method(dsp64, gensym("dsp_add64"),
                  x, fold_perform64, 0, NULL);
}

static void fold_perform64(t_fold *x, t_object *dsp64,
                           double **ins, long numins,
                           double **outs, long numouts,
                           long sampleframes,
                           long flags, void *userparam)
{
    double *in_a     = ins[0];
    double *in_width = ins[1];

    double *out_r0    = outs[0];
    double *out_r1    = outs[1];
    double *out_state = outs[2];
    double *out_nroots= outs[3];
    double *out_xout  = outs[4];

    double eps    = (x->eps > 0.0 ? x->eps : 1e-12);
    double x_prev = x->x_prev;

    for (long i = 0; i < sampleframes; ++i) {

        double a     = x->a_connected     ? in_a[i]     : x->a;
        double width = x->width_connected ? in_width[i] : x->width;

        // sanitiza entrada
        if (!fold_isfinite(a))
            a = 0.0;

        if (!fold_isfinite(width) || fabs(width) < eps) {
            // evita width ~ 0 matando o modelo
            width = (width >= 0.0 ? eps : -eps);
        }

        double a_eff = a * width;

        double r0     = 0.0;
        double r1     = 0.0;
        double state  = 0.0;
        double nroots = 0.0;
        double xout   = x_prev; // default (para modo hold)

        if (!fold_isfinite(a_eff)) {
            // input totalmente zoado → considera "sem raízes"
            state  = 0.0;
            nroots = 0.0;
            switch (x->mode) {
            case FOLD_MODE_ZERO:
                xout = 0.0;
                break;
            case FOLD_MODE_NAN:
                xout = NAN;
                break;
            case FOLD_MODE_HOLD:
            default:
                // mantém x_prev
                break;
            }
        } else if (a_eff < -eps) {
            // região fold: duas raízes reais
            double root = sqrt(-a_eff);
            r0     = -root; // instável
            r1     =  root; // estável
            state  = 1.0;
            nroots = 2.0;

            xout   = r1;    // escolhe a raiz estável
            x_prev = xout;  // atualiza memória
        } else if (fabs(a_eff) <= eps) {
            // caso degenerado: raiz dupla em 0
            r0     = 0.0;
            r1     = 0.0;
            state  = 2.0;
            nroots = 1.0;

            xout   = 0.0;
            x_prev = xout;
        } else { // a_eff > eps → sem raízes reais
            r0     = 0.0;
            r1     = 0.0;
            state  = 0.0;
            nroots = 0.0;

            switch (x->mode) {
            case FOLD_MODE_ZERO:
                xout = 0.0;
                break;
            case FOLD_MODE_NAN:
                xout = NAN;
                break;
            case FOLD_MODE_HOLD:
            default:
                // xout permanece x_prev
                break;
            }
        }

        out_r0[i]     = r0;
        out_r1[i]     = r1;
        out_state[i]  = state;
        out_nroots[i] = nroots;
        out_xout[i]   = xout;
    }

    x->x_prev = x_prev;
}
