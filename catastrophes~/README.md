# catastrophes~ — René Thom catastrophe models as Max/MSP audio externals (v1.0.0)
**DOI (Zenodo, v1.0):** https://doi.org/10.5281/zenodo.18166590

`catastrophes~` is a small library of Max/MSP audio externals implementing René Thom’s elementary catastrophes as **audio‑rate equilibrium operators**. Each object computes real equilibria of a Thom potential’s gradient in real time, exposing (i) the full set of real roots and (ii) a **single selected state** `x_out` determined by local stability and continuity (hysteresis).

The current library implements the classical series **A₂–A₅**:

- `fold~` — A₂ (fold / *dobra*)
- `cusp~` — A₃ (cusp / *cúspide*)
- `swallowtail~` — A₄ (swallowtail / *andorinha*)
- `butterfly~` — A₅ (butterfly / *borboleta*)

The design is **operational**: the objects are meant to be driven by time‑varying control signals and to act as musically meaningful morphogenetic operators for synthesis, spatialisation, and control.


**Version**: v1.0.0

---

## 1. Installation

### 1.1 Binary installation (macOS)

The recommended way to use `catastrophes~` is as a **Max package**.

1. Download the archive (e.g. `catastrophes~-v1.0.0.zip`) and unzip it.
2. You should obtain a folder named `catastrophes~`. Move that folder into your Max Packages directory, for example:

   - `~/Documents/Max 9/Packages/`
   - `~/Documents/Max 8/Packages/`
   - `~/Documents/Max 7/Packages/`

3. Restart Max. Open the help patches in `help/` (e.g. `cusp~.maxhelp`) to confirm everything loads.

**Troubleshooting (macOS Gatekeeper / quarantine):**  
If Max refuses to load an external after downloading/unzipping, remove the quarantine attribute from the package folder:

```bash
xattr -dr com.apple.quarantine "$HOME/Documents/Max 9/Packages/catastrophes~"
```

### 1.2 Installing the prebuilt externals (optional)

If you already have the package folder in place and want to (re)install the prebuilt `.mxo` bundles from `externals/`, run:

```bash
cd scripts
./build_and_install.sh
```

You can override the destination folder with:

```bash
INSTALL_DIR="$HOME/Documents/Max 9/Packages/catastrophes~/externals" ./build_and_install.sh
```

### 1.3 Building from source (optional)

To build the externals yourself, install the Cycling ’74 Max SDK and set `MAXSDK_PATH` to the SDK folder, then run `make` from each object folder.

Example:

```bash
export MAXSDK_PATH="$HOME/Documents/Max 9/Packages/max-sdk"
make -C "src/cusp~"
make -C "src/fold~"
make -C "src/swallowtail~"
make -C "src/butterfly~"
```

The build outputs a `.mxo` bundle in each `src/<object>/` folder.


## 2. Objects overview (models)

Each object implements the gradient of a Thom potential.

### fold~ (A₂)

- Potential: `V(x; a_eff) = x^3/3 + a_eff * x`
- Gradient:  `V'(x) = x^2 + a_eff = 0`

### cusp~ (A₃)

- Potential: `V(x; alpha, beta_eff) = x^4/4 + (beta_eff/2) * x^2 + alpha * x`
- Gradient:  `V'(x) = x^3 + beta_eff * x + alpha = 0`

### swallowtail~ (A₄)

- Gradient: `V'(x) = x^4 + a * x^2 + b_eff * x + c_eff = 0`

### butterfly~ (A₅)

- Gradient: `V'(x) = x^5 + a * x^3 + b_eff * x^2 + c_eff * x + d_eff = 0`

**Selection rule (`x_out`)**

Internally, each solver:

- computes all real equilibria at each time step (per sample or per signal vector),
- tests local stability via `V''(x) > 0`,
- selects a single output `x_out` based on stability and continuity with the previous state `x_prev` (hysteresis).

---

## 3. Examples

The `patchers/` folder contains demonstration patches:

- `A2_fold_bifurcation.maxpat` — fold (A₂) regimes (0/1/2 roots) with mode control
- `A3_cusp_hysteresis.maxpat` — cusp (A₃); discriminant, number of real roots and branch selection
- `A4_swallowtail_multistability.maxpat` — swallowtail (A₄); multi‑root tracking and selection
- `A5_butterfly_multistability.maxpat` — butterfly (A₅); multi‑root tracking and selection

These patches are intended as **operational examples** rather than finished pieces. They can be used as starting points to embed the objects into synthesis chains, spatialisation networks, or control architectures.

---

## 4. License and citation

This code is released under the **MIT License** (see `LICENSE`).

If you use `catastrophes~` in academic work, cite it along the lines of:

> Steidle, Leon. catastrophes~: Thom catastrophe models as audio‑rate equilibrium operators in Max/MSP. Version 1.0.0, 2026.

A BibTeX entry can be added in `CITATION.cff` when the archival record / publication is finalized.

---

## 5. Acknowledgements

This library was developed in the context of a doctoral research project on **energy and morphogenesis in music**, drawing on René Thom’s catastrophe theory, Gilbert Simondon’s theory of individuation, and Ilya Prigogine’s non‑equilibrium thermodynamics.
