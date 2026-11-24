# Course concept map — Muchi edition

Sources checked November 23, 2026. This is a custom game built from public course concepts, NOT an official solution or proof of exactly what your Fall ECE419K section covered. The user's current rubric controls requirements; some textbook examples still mention older Tiva/Keil hardware. Preserve the MSPM0 course drivers and verify with your TA where needed.

| Firmware concept | Public primary source | Application |
|---|---|---|
| States, structures, integer state variables | [Chapter 4, §§4.3–4.4](https://users.ece.utexas.edu/~valvano/mspm0/ebook/Ch4_FiniteStateMachines.html) | MENU, HELP, PLAY, ENDING; switch chooses screen behavior. The branch-based FSM is a custom application, not a copied indexed-table listing. |
| Periodic ADC and sampling limits | [Chapter 7, §§7.4,7.7](https://users.ece.utexas.edu/~valvano/mspm0/ebook/Ch7_ADC.htm) | TIMG12 samples PB18 at nominal 30 Hz. Prove the input bandwidth and measured rate on your hardware. |
| Main plus two interrupt threads | [Chapter 9, §9.2](https://users.ece.utexas.edu/~valvano/mspm0/ebook/Ch9_EmbeddedSystemDesign.htm) | Input ISR signals main; SysTick sends PCM samples; all display output stays in main. |
| Constant arrays and demand-based sprite drawing | Chapter 9, §§9.3,9.5 | Sprite/font arrays; erase old 8x8 positions before drawing new actors. No framebuffer or tile cache. |
| Pointer/count DAC playback | Chapter 9, §§9.2,9.4 | Sound_Start passes array address and count; each SysTick sends one 5-bit sample. |
| Switch debouncing | Chapter 9, §9.7 | Two consecutive changed readings per button. This specific counter implementation is custom. |
| Modular drivers | Chapter 9, §9.2 | Existing ADC, Switch, LCD, Timer; supplied Sound, DAC, LED modules. |
| Course lab sequence | [2026 lab page](https://users.ece.utexas.edu/~valvano/mspm0/labs.html) | Lab 4 FSM, Lab 5 DAC, Lab 6 LCD, Lab 7 ADC, Lab 8 FIFO, Lab 9 game. |

## What was simplified
No FIFO/ring buffer (although the published lab sequence does teach FIFO), xorshift random generator, string formatting, string-memory helper functions, scrolling terrain, or tile cache in game firmware. No dynamic allocation or floating-point firmware. Fixed coordinates make completion predictable. The input handoff uses volatile globals and a ready flag. Main briefly disables interrupts to copy/clear shared values; it enters that block with interrupts enabled and never draws there.

The custom font is still original code: a constant 2-D array, loops and shifts, applied to LCD rectangles. This is an application of elementary C concepts, not a claim that this font implementation appeared in lecture. Similarly, timers/counters implement our jump and collision rules; the game-specific algorithms are original. Course libraries and CMSIS definitions provide hardware register access. Sound.c retains PRIMASK save/restore to preserve caller interrupt state; that helper must be understood, not disguised as your own lecture code.

## Honest scope
Host tests and Python asset/test tools are development aids, not code running on the MCU. They do not prove target timing or exact syllabus coverage. Review CHECKOUT_GUIDE.md until you can explain each decision. Follow your section's rules on outside assistance and attribution; the commit history records the development history. Do not claim externally assisted code was independently written.
