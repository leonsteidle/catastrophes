# catastrophes~

Real-time catastrophe-model operators for Max/MSP by Leon Steidle.

`catastrophes~` provides `fold~` (A2), `cusp~` (A3), `swallowtail~` (A4), and `butterfly~` (A5). Each object calculates equilibrium roots from control signals and exposes the roots alongside a selected output. A patch can map these signals to synthesis parameters, voice organization, or spatial coordinates. `fold~` and `cusp~` recalculate every sample; `swallowtail~` and `butterfly~` recalculate once per signal vector.

## Version 2.0.0

Version **2.0.0 is prepared locally; public release is pending**. It revises the A5 real-root solver while preserving the source code and executables of the other three objects. The package contains one public `butterfly~`.

The A5 correction can change root counts and selected outputs near repeated roots. `stability_eps` is the recommended name for the positive-curvature selection threshold; `eps` remains its alias and no longer affects root extraction or merging. Read the [migration guide](catastrophes~/docs/MIGRATION.md) before adopting this version in an existing A5 patch.

The final A5 executable passed its specified functional confirmation in the x86_64 slice at 48 kHz, signal vector 64, and I/O vector 512. Its supplied binary also contains arm64, which was not runtime-tested. The A5 deployment target is macOS 15.0 or later. The [release record](catastrophes~/docs/RELEASE_STATUS.json) identifies the supplied build and the confirmation scope; its publication field records the status at package assembly.

## Installation and documentation

The installable asset prepared for release is `catastrophes-2.0.0.zip`. After extraction, place the complete `catastrophes~` folder in the user Packages folder for the intended Max installation. Quit Max before changing package versions, and keep only one version in its search paths. Preserve older packages outside those paths with the work that depends on them.

Start with the [package README](catastrophes~/README.md) and the object patches in [`help/`](catastrophes~/help/). Further documentation covers:

- [Implementation and interface](catastrophes~/docs/IMPLEMENTATION.md)
- [Migration and compatibility](catastrophes~/docs/MIGRATION.md)
- [Changes in this version](catastrophes~/docs/CHANGELOG.md)
- [Build and verification identity](catastrophes~/docs/RELEASE_STATUS.json)

The package's audio and rendered figures are historical material, including A5 examples made with the earlier implementation. They have not been regenerated for 2.0.0. The paired-path and compositional examples for the revised Computer Music Journal article are separate supplements.

## License and citation

The library uses the [MIT License](catastrophes~/LICENSE). Retain its copyright and license notices when redistributing it.

Use [CITATION.cff](CITATION.cff) to identify the software and version. The prepared 2.0.0 citation metadata does not yet contain a version DOI. The earlier archive identifies `v1.0` and must not be presented as the DOI of 2.0.0.
