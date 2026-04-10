# catastrophes~ — catastrophe models as Max/MSP audio externals (v1.0.0)

catastrophes~ is a small library of Max/MSP audio externals implementing elementary catastrophe models as **audio‑rate equilibrium operators**. Each object computes real equilibria of a potential’s gradient in real time, exposing (i) the full set of real roots and (ii) a **single selected state** `x_out` determined by local stability and continuity (hysteresis).

The current library implements series **A₂–A₅**:

- `fold~` — A₂ (fold / *dobra*)
- `cusp~` — A₃ (cusp / *cúspide*)
- `swallowtail~` — A₄ (swallowtail / *andorinha*)
- `butterfly~` — A₅ (butterfly / *borboleta*)

This repository was anonymized for peer review.
