# Implementation and interface

Implementation reference for `catastrophes~` 2.0.0. The numerical code is unchanged from the validated 2.0.0-rc.1. Final-executable confirmation and publication status are recorded separately in [RELEASE_STATUS.json](RELEASE_STATUS.json).

## Equations and calculation schedule

The objects solve `V'(x) = 0`; roots represent equilibria of the associated potential. A local minimum is stable under the associated gradient dynamics. This classification does not establish stability of a sound process subsequently controlled by the root.

| Object | Stationarity equation | Recalculation |
| --- | --- | --- |
| `fold~` | `x^2 + a*w = 0` | Each sample |
| `cusp~` | `x^3 + beta*w*x + alpha = 0` | Each sample |
| `swallowtail~` | `x^4 + a*x^2 + b*w*x + c*w = 0` | Once per signal vector |
| `butterfly~` | `x^5 + a*x^3 + b*w*x^2 + c*w*x + d*w = 0` | Once per signal vector |

Here `w` is the effective width after a near-zero clamp. For finite width, the clamp preserves its sign and treats zero as positive. A2 uses its effective `eps`; A3–A5 use `width_eps`. Width does not apply a uniform normalization across the family. A4/A5 read the first input sample of each vector and repeat the results throughout it; excursions wholly inside a vector can be missed.

| Object | Initial stored controls | Numerical defaults | Initial selection memory |
| --- | --- | --- | --- |
| A2 | `a=0`, `width=1` | `eps=1e-9` | 0 |
| A3 | `alpha=0`, `beta=0`, `width=1` | `eps=1e-12`, `width_eps=1e-12` | 0 |
| A4 | `a=b=c=0`, `width=1` | `eps=1e-12`, `width_eps=1e-12` | 0 |
| Revised A5 | `a=-3`, `b=0`, `c=1`, `d=0`, `width=1` | `stability_eps=1e-12`, `width_eps=1e-12` | 0 |

A connected signal overrides the corresponding stored coefficient during DSP. Parameter-named messages set the named stored value. A bare float received at any inlet changes the primary coefficient: `a` in A2/A4/A5 or `alpha` in A3. It does not set the coefficient associated with a secondary inlet. `reset` restores selection memory to zero without resetting coefficients.

## Roots, selection, and output codes

`nroots` counts distinct real-root locations identified by the calculation, not repeated algebraic multiplicity. Numerical classification can affect the count near coincident roots. No guarantee of finding every real root for every finite floating-point input is made.

| Object | `state` meaning | Root-channel filling |
| --- | --- | --- |
| A2 | 0: no roots; 1: two roots; 2: boundary classified at zero | Zero when no roots; coincident zero entries at the boundary |
| A3 | Lower/upper selection coded 0/1; with one root, negative/nonnegative is coded 0/1 | Repeats values when fewer than three distinct roots are reported |
| A4 | Index 0–3 in the current ordered root list; −1 if none | NaN in unused root channels |
| Revised A5 | Index 0–4 in the current ordered root list; −1 if none | NaN in unused root channels |

Only root indices below `nroots` designate returned A4/A5 roots. An index can change when another root appears or disappears; it is not a permanent branch identity. NaN indicates an unavailable numerical value. Select or replace it before using the channel as a synthesis control. The filling rule is specified by source assignments; earlier buffer-based recordings did not independently verify preservation of NaN through the capture path.

A2 selects its positive root when two roots exist. Its boundary classification returns zero. A3 normally chooses between the two outer roots, preferring positive curvature and proximity to the preceding output; its multiple-root boundary uses proximity between the extremes. A4 and revised A5 choose the nearest returned root whose curvature exceeds their selection threshold, or the nearest returned root if none passes. Equal distances favor the lower candidate in the ordered list. Hysteresis depends on the equilibrium structure, trajectory, and selection rule; it is not a uniform property of all four objects.

A3 additionally exposes `D = (alpha/2)^2 + (beta*w/3)^3`, Cardano's classification quantity. Outside its tolerance band, negative D indicates three distinct roots and positive D one. This sign convention is opposite to the polynomial discriminant. Root counts of one for A2 or two for A3 can occur at classified boundaries even if a legacy help label lists only the usual regions.

## Revised butterfly~

The public class has five signal inlets, in order `a`, `b`, `c`, `d`, `width`, and eight signal outlets: `r0`–`r4`, `state`, `nroots`, `x_out`. Optional positional arguments set `a b c d width`. The canonical class name is `butterfly~`; the historical experimental name is not an alternative package object.

The revised solver isolates real roots recursively using derivative roots and bounded monotone intervals. Its header uses compensated polynomial evaluation with explicit FMA, power-of-two scaling, machine-relative classification, and merging within 64 local ulps. These are numerical-resolution choices, not a proof of correctness at arbitrary scales. An archived extreme-scale case loses a small root pair after scaling underflows coefficients.

`stability_eps` controls only the positive-curvature preference in output selection. `eps` is an accepted alias for the same setting; neither controls root finding or merging in this revision. Both use the same positive-finite-value setter. `width_eps` is a separate positive finite near-zero-width threshold. See [MIGRATION.md](MIGRATION.md) for the semantic change from v1.0.

For non-finite raw controls, revised A5 writes NaN to all outputs and preserves selection memory. It also checks the width-scaled `b`, `c`, and `d` products for overflow and takes the same response. If the solver instead returns no candidates, it reports `nroots=0`, `state=-1`, and NaN root channels. `mode hold` retains the stored output; `mode zero` writes and remembers zero; `mode nan` writes NaN while preserving memory. For finite coefficients an odd-degree A5 polynomial has at least one real root, so an empty returned set is a numerical fallback, not a mathematical no-root region.

## Other-object boundary behavior

A2 and A4 retain their existing implementations. A2's numeric `mode` attribute uses 0/1/2 for hold/zero/nan; A4 uses symbolic `mode hold`, `mode zero`, and `mode nan` messages. A2 preserves memory during its no-root response even in zero mode; A4 zero mode changes the memory to zero. A3 has no no-root mode selector. These interfaces should not be treated as interchangeable.

Exceptional-input handling also differs. A2 replaces non-finite `a` with zero and applies its own width fallback. A3 writes NaN to every output for non-finite raw controls and retains memory, but does not separately guard overflow of effective coefficients. A4 replaces non-finite width with one and takes its no-root path when effective coefficients are invalid. These source-level rules are not a claim of exhaustive runtime validation.

## Build and evidence limits

The revised A5 requires its solver header and C11. The build uses `-O2`, `-fno-fast-math`, `-fno-associative-math`, `-fno-unsafe-math-optimizations`, `-fno-finite-math-only`, and `-ffp-contract=off`, retaining explicit `fma()` calls. The deployment target is macOS 15.0 for arm64 and x86_64. The archived 2.0.0-rc.1 captures exercised x86_64 only; they do not establish physical host hardware or native arm64 operation. Refer to RELEASE_STATUS.json for the final executable.

Historical offline and candidate Max tests describe their own sources, executables, arithmetic, settings, and test corpora. They are not retroactively tests of the canonical 2.0.0-rc.1 binary. The archived 2.0.0-rc.1 gate separately verified four root cases, four selection-threshold/alias cases, four API cases, a NaN/Inf classification check before buffer storage, ten invalid-input/overflow memory paths, and vector timing at signal sizes 16/64/256. All captures used 48 kHz and I/O 512. Hashes and archived captures accompany the preparation record. This finite test set does not establish universal root completeness, CPU scaling, or dropout-free performance.

The final `2.0.0` executable subsequently passed the same 23 functional cases at 48 kHz, signal vector 64, and I/O vector 512, with 512 samples per recorded field. Its capture, dated 8 September 2026 UTC, identifies the final x86_64 slice; JSON and CSV records agree for all 220,160 scalar values. The final source differs from RC.1 only in its release comment and version string. The solver header is byte-identical. Cadence measurements were not repeated, and no native arm64 runtime, CPU, or xrun result is added by this confirmation. The final capture and executable hashes are recorded in [RELEASE_STATUS.json](RELEASE_STATUS.json).
