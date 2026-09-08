# Migration to catastrophes~ 2.0.0

The final executable passed its specified functional confirmation; the package is prepared locally and public release is pending. See [RELEASE_STATUS.json](RELEASE_STATUS.json). Archive working installations before evaluating this version.

## Effects on an existing butterfly~ patch

`butterfly~` keeps its public name, stationarity equation, five inputs, and eight outputs. Its revised solver can return different root sets from the earlier implementation, especially near repeated roots. This can change `nroots`, the index reported by `state`, and the nearest root selected for `x_out`. A patch that gates voices by root count or maps root channels to sound may consequently behave differently.

| Setting or behavior | Earlier A5 | A5 in 2.0.0 |
| --- | --- | --- |
| `eps` | Affects root acceptance and output selection | Alias of `stability_eps`; output selection only |
| `stability_eps` | Not available | Recommended name for the positive-curvature selection threshold |
| Root merging | Includes a fixed absolute-distance criterion | Uses the solver header's machine-relative criteria |
| Overflow in width-scaled coefficients | No separate guard on the products | NaN outputs; selection memory retained |
| Calculation schedule | First input sample per signal vector | Unchanged |
| Bare float received at any inlet | Changes stored `a` | Unchanged |

When a message is intended to set the selection threshold, use the explicit name, for example:

```text
stability_eps 0.000000000001
```

`eps` remains accepted with the same numerical value, but it no longer restores the earlier control over root extraction. The revised object introduces no replacement public root-tolerance parameter. Reassess patches that deliberately varied `eps` to alter root counts.

For stored secondary coefficients, use named messages such as `b 0.5`, or connect a signal to the appropriate inlet. A bare float at a secondary inlet still changes `a`. During DSP, a connected signal overrides the corresponding stored value. The stored settings printed by `info` are not a record of the connected signal trajectory.

## Root channels and selection memory

`nroots` reports distinct returned root locations rather than algebraic multiplicity. `state` indexes the current ordered root list; it is not a persistent identifier for a mathematical branch. Root indices can change when another root appears or disappears.

Only indices below `nroots` designate returned A5 roots. Unused root channels contain NaN and should be handled before mapping them to synthesis or spatialization controls. The selected output uses the nearest root whose curvature exceeds `stability_eps`, falling back to the nearest returned root when none passes that threshold. Equal distances favor the lower candidate in the ordered list.

For non-finite raw controls or overflow in the width-scaled coefficients, the revised object writes NaN to all outputs and preserves selection memory. A numerical failure to return candidates takes the existing no-root fallback: `mode hold` retains the previous output, `mode zero` writes and remembers zero, and `mode nan` writes NaN without changing memory. For finite coefficients, an odd-degree polynomial has at least one real root; this fallback describes numerical failure, not a mathematical no-root region.

## Preserve earlier work

1. Archive the package, source or executable identity, patches, and recordings used by existing work. Keep that archive outside Max's Packages folders and search paths.
2. Quit Max before changing the available package. Expose only one `catastrophes~` version at a time; renaming an old package folder inside a search path does not reliably prevent duplicate classes.
3. Open the intended package and check the `butterfly~` version and solver identity using `info`. Confirm the result of the actual patch before adopting the new version.
4. Save an intentional musical migration as a new version of the work. Retain the earlier patch and render for comparison.

The package contains one canonical `butterfly~`. A renamed experimental bundle is not a substitute: changing a bundle filename does not change the class registered by its executable.

The source code and executables of A2, A3, and A4 are unchanged. Cusp-based works therefore do not require rerecording merely because the A5 solver has changed. Other dependencies or changes to a complete musical system should be documented separately.

## Historical examples and platform scope

The bundled audio and rendered figures were retained from the earlier package. In particular, the A5 audio and figure are not renders of the corrected 2.0.0 implementation. Rerunning a historical A5 patch with this version may produce a different result. The paired-path and compositional examples for the revised Computer Music Journal article are distributed separately and use the unchanged `cusp~` implementation.

The revised A5 build contains arm64 and x86_64 slices and targets macOS 15.0 or later. The final `2.0.0` executable passed its functional confirmation at 48 kHz, signal vector 64, and I/O vector 512 in the x86_64 slice. Native arm64 operation was not exercised. Earlier integration captures retain their original executable identities; vector-timing measurements at signal sizes 16, 64, and 256 remain attributed to RC.1 and were not repeated for the final executable.

The revised solver retains a documented extreme-scale underflow limitation. Its finite test record does not establish root completeness for all floating-point inputs or unrestricted real-time performance. See [IMPLEMENTATION.md](IMPLEMENTATION.md) for the numerical and interface details.
