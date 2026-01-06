# Implementation contract (summary)

The `catastrophes~` objects implement Thom’s elementary catastrophes A₂–A₅ as **real-time equilibrium operators**.

## Common principles
1. Each object computes real equilibria of a polynomial equation of the form `V'(x; u)=0`.
2. It exposes **all real roots** (`r_k`), plus diagnostics:
   - `nroots`: number of real equilibria (regime multiplicity)
   - `state`: which equilibrium is selected for `x_out` (selection index / branch)
   - `disc` (cusp~): discriminant diagnostic for cubic regimes
3. Local stability is tested by the second derivative: `V''(r_k; u) > eps`.
4. Selection rule:
   - prefer stable roots;
   - if multiple stable roots exist, choose by continuity with the previous output (`x_prev`) to make hysteresis explicit.
5. When no real roots exist (object-dependent), the output behavior is governed by a declared mode (`hold` / `zero` / `nan`).

## Width scaling
Where an inlet `width` is present, parameters are normalized to keep regimes usable in audio-rate control:
- define `width_eff = sign(width) * max(abs(width), width_eps)`.
- certain objects scale some control parameters by `width_eff` (e.g., b_eff, c_eff, d_eff).

This contract is intentionally simple: it makes regime structure and branch choice auditable as compositional decisions.
