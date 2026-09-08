# catastrophes~ 2.0.0

Real-time catastrophe-model operators for Max/MSP.

**Release status:** the final executable passed its specified functional confirmation; the package is prepared locally and public release is pending. [RELEASE_STATUS.json](docs/RELEASE_STATUS.json) records the build identity, confirmation scope, and publication status at package assembly.

`catastrophes~` provides four equilibrium operators: `fold~` (A2), `cusp~` (A3), `swallowtail~` (A4), and `butterfly~` (A5). They calculate real-root approximations to polynomial stationarity equations and expose the roots together with a selected output. These signals can control synthesis parameters, voice organization, or spatial coordinates. A sound-generating or spatialization patch supplies the mapping from the model to its audible result.

Each object has its own selection rules, output conventions, and calculation schedule. `fold~` and `cusp~` recalculate every sample. `swallowtail~` and `butterfly~` read the first sample of each signal vector and hold their results for that vector. The operators calculate equilibria rather than integrate the associated differential equations.

## What changes in 2.0.0

This version corrects the A5 root-finding implementation while retaining one public object named `butterfly~`. Its revised solver addresses duplicated or missing root locations near repeated roots. Root counts and selected outputs can consequently differ from the earlier implementation. The major-version change also reflects the narrower scope of `eps`: it remains accepted as an alias of `stability_eps`, which controls output selection only.

The source code and executables of `fold~`, `cusp~`, and `swallowtail~` are unchanged. Read the [migration guide](docs/MIGRATION.md) before opening an existing A5 patch with this version.

## Installation and first use

The package supplies macOS `.mxo` externals. The revised A5 executable targets macOS 15.0 or later and contains arm64 and x86_64 slices. No Windows build is included. The package metadata specifies Max 8.6.5 as its minimum version; this does not imply testing on every later Max or macOS version.

1. Quit Max. Archive any previous `catastrophes~` package outside Max's Packages folders and search paths, together with the work that depends on it.
2. Place the complete `catastrophes~` folder in the user Packages folder for the Max installation you intend to use. Keep only one version of the library in Max's search paths.
3. Restart Max and open `help/butterfly~.maxhelp` through Max's File > Open command. The other three objects have corresponding help files.
4. Send `info` to `butterfly~` and check that the console reports `2.0.0` and the expected solver identity. An `info` response describes stored settings; it does not verify DSP execution or report the changing values of connected signals.

Begin with low speaker or headphone gain when enabling a new synthesis mapping. If macOS blocks loading, review the source and the system's security guidance before proceeding. This package does not require disabling system security settings.

## Documentation and examples

- [Implementation and interface](docs/IMPLEMENTATION.md) explains equations, controls, output codes, calculation schedules, and numerical limits.
- [Migration](docs/MIGRATION.md) describes A5 compatibility changes and how to preserve earlier work.
- [Changelog](docs/CHANGELOG.md) lists the changes in this version.
- `help/` contains object help patches. A5 help describes the revised interface; A2/A3 count labels include classified boundary cases.
- `patchers/` and `figs/` contain the library's historical technical demonstrations and figure material.
- `audio/` contains the historical recordings distributed with the earlier package.

The bundled audio and rendered figures retain their original provenance. They have not been regenerated with 2.0.0; in particular, the A5 recording and figure were produced with the earlier A5 implementation. Rerunning an A5 demonstration with the corrected solver may produce a different result. The paired-path and compositional examples prepared for the revised Computer Music Journal article are separate supplementary materials, not these historical package examples.

## Building A5 from source

The source tree includes per-object Makefiles. The revised A5 requires both `src/butterfly~/butterfly~.c` and `src/butterfly~/butterfly_real_solver.h`, a Max SDK checkout, and the macOS command-line build tools. From the package root, supply the SDK location:

```sh
MAXSDK_PATH="/path/to/max-sdk" bash scripts/build_butterfly.sh
```

This rebuilds the package's own A5 bundle; it neither installs the package nor publishes it. The build uses C11, explicit fused multiply-add operations, and strict floating-point compilation. Keep the supplied floating-point flags: enabling fast-math or reassociation can change numerical behavior. A rebuilt executable has its own identity and requires appropriate checks before use in a reproducible study.

## Evidence and limits

The final `2.0.0` executable passed its 23-case functional confirmation in Max at 48 kHz, signal vector 64, and I/O vector 512. The checks covered four canonical root cases, four selection-threshold/alias cases, four API cases, a NaN/Inf classification oracle before buffer storage, and ten invalid-input/overflow sequences. The capture identifies the loaded x86_64 slice and matches the supplied executable. Native arm64 operation was not exercised.

Earlier standalone and Max records retain their original source and executable identities. Vector-timing measurements at signal sizes 16, 64, and 256 belong to `2.0.0-rc.1`; the final version has unchanged numerical code, but those measurements were not repeated. The combined finite evidence does not establish complete root recovery at every floating-point scale or unrestricted real-time performance. A documented extreme-scale case loses a small root pair when coefficient scaling underflows. The [implementation reference](docs/IMPLEMENTATION.md) describes this limit and the object-specific behavior of unavailable outputs.

## License and citation

The library is distributed under the MIT License. Retain `LICENSE` and the existing copyright notices when redistributing it.

Identify the version and, where reproducibility requires it, the source or executable used. The earlier public tag is `v1.0`, with package metadata `1.0.0`. Its archive identifies that earlier version. The prepared `2.0.0` package has not yet received a published version DOI; citation metadata must be completed with the new archive record after publication.
