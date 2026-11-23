# Lab 9 audit — November 23, 2026

| Requirement | Weight | Implementation / remaining evidence |
|---|---:|---|
| At least 2 external gameplay buttons | 10% | Four active-low inputs: PB16 UP, PB19 DOWN, PB12 MINE, PB17 BUILD. Uses supplied Switch driver; verify wiring. |
| Periodic slide-pot ADC | 15% | ADCin called from 30 Hz TIMG12 ISR, PB18. Pot controls horizontal velocity. Measure rate, verify conversion reliability and establish input bandwidth below 15 Hz; post-sampling smoothing is not an anti-alias filter. |
| At least 3 moving sprites | 20% | Player, rabbit, butterfly, slime; main2 isolates the art. |
| At least 2 appropriate DAC sounds | 20% | Mining, placing, damage and celebration; SysTick at approximately 11 kHz, PB0..4 5-bit DAC. Verify speaker/amplifier. |
| Visible score | 10% | PTS in HUD and score on win/loss screen. |
| 2–3 minute video | 5% | Not recorded. VIDEO_PLAN.md is an outline, not a substitute. Public playlist opt-out is allowed by supplied rubric. |
| At least 2 appropriate ISRs | 10% | TIMG12 input acquisition and SysTick audio; no LCD work in interrupts. |
| Easy/fun | 0% | Instructions, short resource/build goal; actual playtesting still needed. |
| At least 2 languages | 10% | English/Spanish selected at startup; instructions, HUD and results localized. Main1 cycles screens for inspection. |

## Procedure and submission gaps
- Proposal must match the platformer controls: UP jumps, DOWN selects material, pot controls horizontal velocity. It does not use a joystick.
- Submit the proposal and obtain TA approval in the Google Doc; approval not verified here.
- Modular ADC, Switch, ST7735, Sound, DAC and LED drivers have separate source/header interfaces (course ADC/Switch/LCD files remain dependencies).
- Main0..main4 provide individual diagnostics; main5 runs the game. Run and document them on the actual board.
- Build with your installed CCS/TI SDK and check the linker map, display orientation, timing, ADC, sound and all inputs. Host checks do not verify these.
- Record and submit the required video.

## Pin use
Four external buttons: PB16/PB19/PB12/PB17; slide pot PB18; DAC PB0..4; existing ST7735 LCD wiring unchanged. One on-board diagnostic LED PB27; no joystick or PA15 DAC. The diagnostic does not use PB21 (listed as an on-board switch in the rubric). PA26/PA27/PA22 are not used by these new routines; no new light-sensor jumper requirement is introduced.

## Engineering design discussion
Describe needs-to-design decisions, testing and iteration: low-voltage wiring, comfortable audio, readable text and visual feedback, bilingual access, reusable components, component cost and electronic waste. Do not claim measurements or user testing that have not been performed.
