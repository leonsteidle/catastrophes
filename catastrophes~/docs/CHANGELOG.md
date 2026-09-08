# Changelog

## 2.0.0 — prepared, not yet published

The final executable passed its specified functional confirmation at 48 kHz, signal vector 64, and I/O vector 512 in the x86_64 slice; see [RELEASE_STATUS.json](RELEASE_STATUS.json). Public release is pending. Earlier solver and Max integration records retain their original source and executable identities, including the RC.1 vector-timing measurements.

### A5 root finding

- Replace complex-root candidate extraction with recursive real-root isolation using derivative roots and monotone intervals. This addresses duplicated or missing root locations near repeated roots.
- Use compensated polynomial evaluation, power-of-two scaling, and machine-relative classification and merging. Retain the documented extreme-scale limitation in which scaling underflows coefficients and a small root pair is lost.
- Detect overflow in the width-scaled `b`, `c`, and `d` coefficients. Invalid raw controls or effective coefficients produce NaN outputs while preserving selection memory.

### A5 interface and compatibility

- Add `stability_eps` as the recommended name for the positive-curvature selection threshold. Keep `eps` as an alias; it no longer affects root extraction or merging.
- Retain the `butterfly~` class name, five inlets, eight outlets, outlet order, nearest-eligible-root selection, lower-candidate tie handling, reset behavior, and once-per-vector calculation.
- Retain the existing bare-float behavior: a float received at any inlet changes stored `a`. Use named messages or connected signals for other coefficients.
- Supply one canonical A5 object. The experimental class used in historical tests is an evidence identifier, not an additional object in this package.

Corrected root sets can change `nroots`, `state`, selected outputs, and sound in existing mappings. The major-version change reflects these effects and the changed scope of `eps`; identical output from the old and new A5 implementations is not promised.

### Build and documentation

- Build `butterfly~` from its C11 source and frozen solver header with strict floating-point flags. The supplied A5 target is macOS 15.0 with arm64 and x86_64 slices.
- Remove inactive legacy solver code from the canonical source and update version identification, comments, assist strings, and A5 help.
- Add optional `info` and `stability_eps` messages to A5 help while retaining its existing signal routing.
- Correct A2/A3 help count labels to include classified boundary cases.
- Document migration, output conventions, update schedules, and numerical limits.

### Preserved material

- Preserve the source code and executables of `fold~`, `cusp~`, and `swallowtail~`.
- Preserve the historical audio and rendered figures without editing or rerendering. Their A5 material was produced with the earlier implementation.
- Keep the historical technical demonstrations separate from the new Computer Music Journal supplementary examples.
- Add no legacy-solver switch or public root-tolerance parameter.

## v1.0 / package metadata 1.0.0 — historical version

The earlier package contains the A2–A5 implementations, help patches, technical demonstrations, and associated audio and figure material. Its public tag is `v1.0`; its package metadata identifies `1.0.0`. Preserve these identifiers when reporting or reproducing work made with that version.

Earlier packaging notes mention a 0.9.0 prerelease. This changelog adds no retrospective testing or release-status claims to that record.
