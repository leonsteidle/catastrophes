#ifndef BUTTERFLY_REAL_SOLVER_H
#define BUTTERFLY_REAL_SOLVER_H

/*
  Distinct real roots of a real polynomial of degree <= 5.

  This header is the controlled A5 repair candidate.  It uses the real roots
  of each derivative to partition the Cauchy-bounded real line into monotone
  intervals.  For the butterfly polynomial, the first derivative is the
  depressed quartic

      P'(x)/5 = x^4 + (3a/5)x^2 + (2b/5)x + c/5.

  Numerical policy
  ----------------
  * Topological decisions do not use the object's user-facing eps=1e-12.
    That value is too large to distinguish nearby, but representable, roots.
  * Polynomial signs are evaluated with compensated Horner arithmetic using
    fma().  A derivative root is treated as a common root only when

        |P(x)| <= 4096*u^2*sum |c_i||x|^(n-i) + location uncertainty,

    where u=DBL_EPSILON/2 is binary64 unit roundoff.
  * Bisection stops only when the midpoint is an endpoint (adjacent binary64
    values), with a 1100-step guard covering the full exponent range.
  * Returned roots are merged only within 64 local ulps.  Thus multiplicity is collapsed, as required
    by the object's "distinct real roots" outlets, without the former fixed
    1e-7 merge radius.

  The constants above define a numerical-resolution policy, not a claim that
  arbitrarily ill-conditioned topology can be recovered from binary64 input.
  The accompanying validation compares decisions with exact Sturm counts of
  the actual binary64 coefficients and reports generator/topology ambiguity
  separately.

  Build/runtime assumption: IEEE-754 binary64, round-to-nearest, explicit
  fma(), and no fast-math/reassociation.  Gradual underflow is required for the
  separately reported subnormal diagnostics; an audio host using FTZ/DAZ may
  legitimately produce different results in that out-of-scope regime.
*/

#include <float.h>
#include <math.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BFRI_MAX_DEGREE 5
#define BFRI_ZERO_FACTOR 4096.0
#define BFRI_MERGE_FACTOR 64.0
#define BFRI_CRITICAL_X_FACTOR 128.0
#define BFRI_MAX_BISECT 1100

typedef struct bfri_stats {
    unsigned long compensated_evaluations;
    unsigned long compensated_horner_steps;
    unsigned long bisection_steps;
    unsigned long recursive_calls;
    unsigned long critical_root_tests;
} bfri_stats;

typedef struct bfri_dd {
    double hi;
    double lo;
} bfri_dd;

static void bfri_stats_zero(bfri_stats *stats)
{
    if (stats) {
        stats->compensated_evaluations = 0;
        stats->compensated_horner_steps = 0;
        stats->bisection_steps = 0;
        stats->recursive_calls = 0;
        stats->critical_root_tests = 0;
    }
}

static bfri_dd bfri_two_sum(double a, double b)
{
    bfri_dd out;
    double s = a + b;
    double bb = s - a;
    out.hi = s;
    out.lo = (a - (s - bb)) + (b - bb);
    return out;
}

static bfri_dd bfri_dd_mul_double(bfri_dd value, double x)
{
    double product = value.hi * x;
    double error = fma(value.hi, x, -product) + value.lo * x;
    return bfri_two_sum(product, error);
}

static bfri_dd bfri_dd_add_double(bfri_dd value, double x)
{
    bfri_dd first = bfri_two_sum(value.hi, x);
    bfri_dd second = bfri_two_sum(first.hi, value.lo + first.lo);
    return second;
}

static bfri_dd bfri_eval_compensated(const double *coeff, int degree,
                                      double x, bfri_stats *stats)
{
    bfri_dd value;
    value.hi = coeff[0];
    value.lo = 0.0;
    for (int i = 1; i <= degree; ++i) {
        value = bfri_dd_mul_double(value, x);
        value = bfri_dd_add_double(value, coeff[i]);
    }
    if (stats) {
        stats->compensated_evaluations += 1;
        stats->compensated_horner_steps += (unsigned long)degree;
    }
    return value;
}

static int bfri_sign_at(const double *coeff, int degree, double x,
                        bfri_stats *stats)
{
    bfri_dd value = bfri_eval_compensated(coeff, degree, x, stats);
    double rounded = value.hi + value.lo;
    if (rounded > 0.0) return 1;
    if (rounded < 0.0) return -1;
    if (value.hi > 0.0) return 1;
    if (value.hi < 0.0) return -1;
    if (value.lo > 0.0) return 1;
    if (value.lo < 0.0) return -1;
    return 0;
}

static double bfri_abs_scale(const double *coeff, int degree, double x)
{
    double ax = fabs(x);
    double scale = fabs(coeff[0]);
    for (int i = 1; i <= degree; ++i) {
        scale = scale * ax + fabs(coeff[i]);
        if (!isfinite(scale)) return DBL_MAX;
    }
    return scale;
}

static double bfri_ulp(double x)
{
    double upward = nextafter(x, INFINITY);
    double distance = fabs(upward - x);
    if (distance == 0.0 || !isfinite(distance)) {
        double downward = nextafter(x, -INFINITY);
        distance = fabs(x - downward);
    }
    return distance > 0.0 && isfinite(distance) ? distance : DBL_TRUE_MIN;
}

static int bfri_is_common_root(const double *coeff, int degree,
                               const double *derivative, double x,
                               bfri_stats *stats)
{
    const double unit_roundoff = 0.5 * DBL_EPSILON;
    bfri_dd value = bfri_eval_compensated(coeff, degree, x, stats);
    double residual = fabs(value.hi + value.lo);
    double evaluation_tolerance = BFRI_ZERO_FACTOR * unit_roundoff * unit_roundoff
                                * bfri_abs_scale(coeff, degree, x);

    /* A returned critical point is a binary64 approximation.  Bound the
       induced P error using |P'(x)|*dx + |P''|_scale*dx^2/2.  The use of the
       evaluated derivative, rather than its absolute term scale, keeps this
       correction second order at an actual stationary point. */
    bfri_dd derivative_value = bfri_eval_compensated(derivative, degree - 1,
                                                     x, stats);
    double dx = BFRI_CRITICAL_X_FACTOR * bfri_ulp(x);
    double location_tolerance = fabs(derivative_value.hi + derivative_value.lo) * dx;
    if (degree >= 2) {
        double second[BFRI_MAX_DEGREE + 1];
        for (int i = 0; i < degree - 1; ++i)
            second[i] = (double)(degree - 1 - i) * derivative[i];
        location_tolerance += 0.5 * bfri_abs_scale(second, degree - 2, x) * dx * dx;
    }
    double tolerance = evaluation_tolerance + location_tolerance;
    return residual <= tolerance;
}

static double bfri_root_bound(const double *coeff, int degree)
{
    double leading = fabs(coeff[0]);
    if (leading == 0.0 || !isfinite(leading)) return INFINITY;
    double maximum = 0.0;
    for (int i = 1; i <= degree; ++i) {
        double magnitude = fabs(coeff[i]) / leading;
        if (magnitude > maximum) maximum = magnitude;
    }
    if (maximum >= DBL_MAX - 1.0) return DBL_MAX;
    return nextafter(1.0 + maximum, INFINITY);
}

static void bfri_sort(double *values, int count)
{
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (values[j] < values[i]) {
                double tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }
}

static int bfri_merge_distinct(double *values, int count)
{
    if (count <= 1) return count;
    bfri_sort(values, count);
    int write_index = 1;
    for (int i = 1; i < count; ++i) {
        double previous = values[write_index - 1];
        double tolerance = BFRI_MERGE_FACTOR
                         * fmax(bfri_ulp(previous), bfri_ulp(values[i]));
        if (fabs(values[i] - previous) > tolerance) {
            values[write_index++] = values[i];
        } else {
            values[write_index - 1] = 0.5 * (previous + values[i]);
        }
    }
    return write_index;
}

static double bfri_bisect(const double *coeff, int degree,
                          double lo, double hi, int sign_lo,
                          bfri_stats *stats)
{
    for (int iteration = 0; iteration < BFRI_MAX_BISECT; ++iteration) {
        double mid = lo + 0.5 * (hi - lo);
        if (mid == lo || mid == hi) break;

        int sign_mid = bfri_sign_at(coeff, degree, mid, stats);
        if (stats) stats->bisection_steps += 1;
        if (sign_mid == 0) return mid;

        if (sign_mid == sign_lo) {
            lo = mid;
            sign_lo = sign_mid;
        } else {
            hi = mid;
        }

    }
    return lo + 0.5 * (hi - lo);
}

static int bfri_solve_recursive(const double *input_coeff, int degree,
                                double *out_roots, bfri_stats *stats)
{
    double coeff[BFRI_MAX_DEGREE + 1];
    double derivative[BFRI_MAX_DEGREE + 1];
    double critical[BFRI_MAX_DEGREE];
    double points[BFRI_MAX_DEGREE + 1];
    int point_is_root[BFRI_MAX_DEGREE + 1];
    double candidates[2 * BFRI_MAX_DEGREE + 1];

    if (stats) stats->recursive_calls += 1;
    if (degree < 1 || degree > BFRI_MAX_DEGREE) return 0;
    if (!isfinite(input_coeff[0]) || input_coeff[0] == 0.0) return 0;

    double leading = input_coeff[0];
    for (int i = 0; i <= degree; ++i) {
        /* Preserve the encoded coefficient relations.  Dividing every
           recursive derivative by its leading coefficient can turn an exact
           multiple derivative root into two roots of a rounded polynomial. */
        coeff[i] = input_coeff[i];
        if (!isfinite(coeff[i])) return 0;
    }

    if (degree == 1) {
        out_roots[0] = -coeff[1] / leading;
        return isfinite(out_roots[0]) ? 1 : 0;
    }

    /* Exact algebraic deflation prevents a representable x=0 root from being
       replaced by a neighbouring subnormal after an asymmetric bisection. */
    if (coeff[degree] == 0.0) {
        double deflated[BFRI_MAX_DEGREE];
        int count = bfri_solve_recursive(coeff, degree - 1, deflated, stats);
        for (int i = 0; i < count; ++i) out_roots[i] = deflated[i];
        out_roots[count++] = 0.0;
        count = bfri_merge_distinct(out_roots, count);
        return count > degree ? degree : count;
    }

    for (int i = 0; i < degree; ++i)
        derivative[i] = (double)(degree - i) * coeff[i];

    int critical_count = bfri_solve_recursive(derivative, degree - 1,
                                              critical, stats);
    double radius = bfri_root_bound(coeff, degree);
    if (!isfinite(radius)) return 0;

    int point_count = 0;
    points[point_count++] = -radius;
    point_is_root[0] = 0;
    for (int i = 0; i < critical_count; ++i) {
        if (critical[i] > -radius && critical[i] < radius) {
            points[point_count++] = critical[i];
            point_is_root[point_count - 1] = 0;
        }
    }
    points[point_count++] = radius;
    point_is_root[point_count - 1] = 0;

    int candidate_count = 0;
    for (int i = 1; i < point_count - 1; ++i) {
        if (stats) stats->critical_root_tests += 1;
        if (bfri_is_common_root(coeff, degree, derivative, points[i], stats)) {
            candidates[candidate_count++] = points[i];
            point_is_root[i] = 1;
        }
    }

    for (int i = 0; i < point_count - 1; ++i) {
        /* A monotone open interval adjacent to an endpoint root cannot contain
           another root.  Skipping it avoids manufacturing duplicate roots
           around odd-multiplicity stationary roots. */
        if (point_is_root[i] || point_is_root[i + 1]) continue;
        double lo = points[i];
        double hi = points[i + 1];
        int sign_lo = bfri_sign_at(coeff, degree, lo, stats);
        int sign_hi = bfri_sign_at(coeff, degree, hi, stats);

        if (sign_lo == 0) candidates[candidate_count++] = lo;
        if (sign_hi == 0) candidates[candidate_count++] = hi;
        if (sign_lo != 0 && sign_hi != 0 && sign_lo != sign_hi) {
            candidates[candidate_count++] = bfri_bisect(coeff, degree,
                                                        lo, hi, sign_lo, stats);
        }
    }

    candidate_count = bfri_merge_distinct(candidates, candidate_count);
    if (candidate_count > degree) candidate_count = degree;
    for (int i = 0; i < candidate_count; ++i) out_roots[i] = candidates[i];
    return candidate_count;
}

static int bfri_solve_real_polynomial(const double *coeff, int degree,
                                      double *out_roots, bfri_stats *stats)
{
    bfri_stats_zero(stats);
    if (!coeff || !out_roots) return 0;
    return bfri_solve_recursive(coeff, degree, out_roots, stats);
}

static int bfri_solve_butterfly(double a, double b_eff, double c_eff,
                                double d_eff, double *out_roots,
                                bfri_stats *stats)
{
    /* Fujiwara-style variable scaling: x = scale*y.  The transformed
       nonleading coefficients are formed by repeated division to avoid
       overflow in scale^k. */
    double bound_candidate = 0.0;
    double candidate = 2.0 * sqrt(fabs(a));
    if (candidate > bound_candidate) bound_candidate = candidate;
    candidate = 2.0 * cbrt(fabs(b_eff));
    if (candidate > bound_candidate) bound_candidate = candidate;
    candidate = 2.0 * sqrt(sqrt(fabs(c_eff)));
    if (candidate > bound_candidate) bound_candidate = candidate;
    candidate = 2.0 * pow(fabs(d_eff), 0.2);
    if (candidate > bound_candidate) bound_candidate = candidate;
    if (!isfinite(bound_candidate)) return 0;

    /* Round upward to a power of two.  Binary scaling is exact and therefore
       does not perturb an exact multiple-root relation in the input
       coefficients. */
    double root_scale = 1.0;
    if (bound_candidate > 0.0) {
        int exponent = 0;
        double fraction = frexp(bound_candidate, &exponent);
        root_scale = ldexp(1.0, fraction == 0.5 ? exponent - 1 : exponent);
    }
    if (!isfinite(root_scale) || root_scale <= 0.0) return 0;

    double a_scaled = scalbn(a, -2 * ilogb(root_scale));
    double b_scaled = scalbn(b_eff, -3 * ilogb(root_scale));
    double c_scaled = scalbn(c_eff, -4 * ilogb(root_scale));
    double d_scaled = scalbn(d_eff, -5 * ilogb(root_scale));
    const double coeff[6] = {1.0, 0.0, a_scaled, b_scaled, c_scaled, d_scaled};
    double scaled_roots[5];
    int count = bfri_solve_real_polynomial(coeff, 5, scaled_roots, stats);
    for (int i = 0; i < count; ++i) out_roots[i] = root_scale * scaled_roots[i];

    /* A few accepted Newton steps on the original (unscaled) polynomial undo
       final rescaling error.  A step is retained only when compensated
       normalized residual decreases. */
    const double original[6] = {1.0, 0.0, a, b_eff, c_eff, d_eff};
    const double derivative[5] = {5.0, 0.0, 3.0 * a, 2.0 * b_eff, c_eff};
    for (int i = 0; i < count; ++i) {
        double x = out_roots[i];
        for (int iteration = 0; iteration < 4; ++iteration) {
            bfri_dd p = bfri_eval_compensated(original, 5, x, stats);
            bfri_dd dp = bfri_eval_compensated(derivative, 4, x, stats);
            double derivative_value = dp.hi + dp.lo;
            if (derivative_value == 0.0 || !isfinite(derivative_value)) break;
            double old_scale = bfri_abs_scale(original, 5, x);
            double old_residual = old_scale > 0.0 ? fabs(p.hi + p.lo) / old_scale : 0.0;
            double step = (p.hi + p.lo) / derivative_value;
            double trial = x - step;
            if (!isfinite(trial) || trial == x) break;
            bfri_dd trial_p = bfri_eval_compensated(original, 5, trial, stats);
            double trial_scale = bfri_abs_scale(original, 5, trial);
            double trial_residual = trial_scale > 0.0
                                  ? fabs(trial_p.hi + trial_p.lo) / trial_scale : 0.0;
            if (trial_residual > old_residual) break;
            x = trial;
        }
        out_roots[i] = x;
    }
    return bfri_merge_distinct(out_roots, count);
}

#ifdef __cplusplus
}
#endif

#endif /* BUTTERFLY_REAL_SOLVER_H */
