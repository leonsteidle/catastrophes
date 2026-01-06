# Reproducibility — catastrophes~

This package is distributed as a Max Package (externals + help files + demonstration patchers) intended to be
auditably reproducible for the CMJ submission bundle.

## Tested environment
- macOS (Apple Silicon + Intel; universal binaries)
- Cycling '74 Max 8.6.5

## Global Max audio settings (used for the evidence set)
- Sample rate (SR): 48,000 Hz
- I/O vector size: 64
- Signal vector size: 64

## Evidence patchers (A2–A5)
Open the following patchers in `patchers/`:

- A2_fold_bifurcation.maxpat (A2 / fold)
- A3_cusp_hysteresis.maxpat (A3 / cusp)
- A4_swallowtail_multistability.maxpat (A4 / swallowtail)
- A5_butterfly_multistability.maxpat (A5 / butterfly; supplementary)

Each patcher contains:
1) a sweep-and-return control trajectory (ida–volta) with a brief hold at the turning point;
2) visual traces for x_out, nroots, and state (where applicable);
3) an audio probe that audifies x_out as sine-oscillator pitch and uses nroots as a gate.

### Sweep messages (as shipped)
- A2 (sweep a):     -0.5, 0.1 4000 0.1 1000 -0.5 4000
- A3 (sweep alpha): -0.5, 0.5 4000 0.5 1000 -0.5 4000
- A4 (sweep c):     -1.,  1.  4000 1.  1000 -1.  4000
- A5 (sweep d):     -2.,  2.  4000 2.  1000 -2.  4000

## Figures and audio examples
- `figs/` contains figure-export patches (*_fig.maxpat) and the resulting images (*_fig.tif, 600 dpi).
- `audio/` contains short audio examples (*.aiff) rendered from the CMJ patchers.

The sweep messages in the figure patches are aligned with the CMJ patchers above.
