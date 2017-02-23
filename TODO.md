- Maybe add the IF (Interrupt Flag) register as memory-mapped by the CPU
  (and not by HWIO) ? -> should modify HWIO interface7
- Take into account DI and EI delay, as well as HALT glitch
