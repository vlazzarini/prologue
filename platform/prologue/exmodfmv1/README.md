# Extended ModFM oscillator (version 1)

This oscillator implements Extended ModFM with the following parameters

- Shape: spectral shape, 0  -> classic FM, 0.25 -> lower sidebands
  only, 0.5 -> modFM, 0.75 -> upper sidebands only, 1 -> classic FM.

- Shift-Shape and Shape LFO: index of modulation.

The following menu parameters are available

- Car ratio: carrier frequency multiplier

- Mod ratio: modulation frequency multiplier

- Mod fine: modulation frequency fine detune

- Attack time: attack time in % (0 - 10s) of internal AR envelope generator.

- Release time: release time in % (0 - 10s) of internal AR envelope generator.

- Amount: amount of EG signal added to the index of modulation


