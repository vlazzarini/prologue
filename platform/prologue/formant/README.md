# Formant oscillator

This oscillator implements ModFM formant synthesis with the following parameters

- Shape: vowel: a - e - i - o - u - a
- Shift-Shape: formant width offset

The following menu parameters are available

- Max frequency shift: the maximum value of the carrier frequency shift parameter (1 - 10), as
multiples of the fundamental frequency.

- Frequency shift amount: amount of carrier frequency shift as a % of the max frequency shift.

- Formant set: 1 - bass; 2 - tenor; 3 - alto; 4 - soprano; 5 - 8
  splits SATB.

- Attack time: attack time in % (0 - 10s) of internal AR envelope generator.

- Release time: release time in % (0 - 10s) of internal AR envelope generator.

- Amount: amount of EG signal scaling the formant frequencies.


