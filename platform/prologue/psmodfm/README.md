# Phase Synchronous ModFM oscillator

This oscillator implements PS ModFM with the following parameters

- Shape: formant frequency
- Shift-Shape: formant Q

The following menu parameters are available

- Max frequency shift: the maximum value of the carrier frequency shift parameter (1 - 10), as
multiples of the fundamental frequency.

- Frequency shift amount: amount of carrier frequency shift as a % of the max frequency shift.

- Formant frequency track mode:
   * 1: the shape control tracks the fundamental frequency.
   * 2 - 10: determines the number of octaves of range of the shape control downards from 12 KHz (which is the maximum formant frequency).  Formant frequencies cannot be set to less than the fundamental frequency.

- Attack time: attack time in % (0 - 10s) of internal AR envelope generator.

- Release time: release time in % (0 - 10s) of internal AR envelope generator.

- Amount: amount of EG signal added to the formant frequency.


