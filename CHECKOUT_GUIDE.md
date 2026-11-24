# Explain and test MyCraft

1. **What runs every 33.3 ms?** TIMG12 samples ADCin, reads switches, updates per-button debounce counters, ORs fresh press bits into presses, and sets ready. It does no display work.
2. **Why volatile?** Main and an ISR share adcSample/buttons/presses/ready. Volatile ensures accesses occur; it does not make a multi-variable transaction atomic.
3. **Why disable interrupts briefly?** Main copies the input values and clears the pending flag/edges without the producer changing them midway. Only a few assignments occur there. If main falls behind, MyCraft_MissedFrames counts overwritten samples. Repeated identical button presses can merge; measure frame service time, do not claim lossless queue behavior.
4. **Why 30 Hz?** The period is 80,000,000/30 core cycles for the supplied timer API. Nyquist requires analog bandwidth below 15 Hz; a human-operated pot alone is not proof of anti-aliasing. Measure and filter appropriately. The old post-sampling smoothing is removed.
5. **What is the state machine?** MENU chooses language; HELP has two pages; PLAY runs the three objectives; ENDING shows Muchi's ice-cream message and permits replay.
6. **How are objectives counted once?** Fixed arrays record whether each wood/flower location was collected. A fresh button edge plus proximity plus an uncollected flag is required. Building is possible only in phase 1 near the cottage. Five stages consume the five pieces of wood.
7. **How does jumping work?** A 20-tick counter selects a triangular height curve using integer arithmetic. A new jump is allowed only at zero. This is intentionally simple, not a physics engine.
8. **What happens on collision?** Position comparisons detect contact. Score loses at most five points and never underflows. The player moves away and receives 45 ticks of temporary protection. Resources remain collected.
9. **How do sprites move without flicker?** Main erases all old rectangles, then draws all new rectangles. Actor lanes contain only sky, so background restoration is a solid-color fill. Static decorations do not occupy actor paths. Full-screen clearing is only for screen transitions/diagnostics.
10. **How does the font work?** Each of seven rows holds five bits. A bit selects a filled LCD rectangle. Scale 2 makes each bit a 2x2 block. This font is custom data/code using arrays, loops and masks.
11. **How does sound work?** Sound_Start selects a constant 5-bit PCM array and sample count. SysTick outputs one array entry through DAC5_Out at about 11 kHz. Count zero ends playback and returns to midscale. PRIMASK save/restore protects the pointer/count update and preserves the caller's original interrupt state.
12. **How are GPIO writes friendly?** DAC5_Out toggles only PB0..4 bits. LED functions use SET/CLR/TGL for PB27. Neither should reset GPIOB after other modules configure it.

## Board checklist
Run MYCRAFT_TEST_MODE 0 through 4 separately, then restore 5. Check ADC returns reliably, each switch maps to the correct bit, all art displays, and four sounds differ. Build with CCS and inspect flash/RAM use. Measure TIMG12 cadence and audio sample rate. Watch MyCraft_MissedFrames during play. Finish all three phases, test collision and jump, repeat in Spanish, and replay. Record the mandatory 2–3 minute hardware video. TA approval remains required.

Practice answering these without reading. If a function cannot be explained, trace it with a concrete input and the debugger before checkout.
