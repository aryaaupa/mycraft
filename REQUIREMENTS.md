# Rubric audit — Muchi edition

| Requirement | Weight | Implementation / remaining check |
|---|---:|---|
| External buttons | 10% | Four: UP jump, DOWN hint, MINE collect, BUILD cottage. Verify actual wiring. |
| Slide pot/periodic ADC | 15% | PB18 sampled by nominal 30 Hz TIMG12; movement thresholds. Measure rate and justify bandwidth below 15 Hz. |
| Three moving sprites | 20% | Player, slime, rabbit, butterfly. |
| Two DAC sounds | 20% | Mining, building, damage, celebration via PB0..4 DAC and SysTick. Verify audible output. |
| Score | 10% | Large HUD score and ending total. |
| 2–3 minute video | 5% | Not recorded; see VIDEO_PLAN.md. |
| Two ISRs | 10% | TIMG12 input sampling and SysTick DAC playback. |
| Simple/fun | 0% | Three explicit goals, contextual HUD, gentle penalty. Human playtesting still needed. |
| Two languages | 10% | Startup selection, English/Spanish gameplay/instructions/ending. Nickname remains Muchi. |

Proposal needs updating: five wood pickups, five cottage stages, three flowers; UP jumps and DOWN shows hints; pot is velocity control. No joystick, mining caves or crystal objective. TA approval not verified. Module diagnostics main0..main4 are supplied; main5 is the game. Course concepts and limitations are documented in COURSE_MAP.md. Hardware checkout, measurement and video remain necessary for a completed submission.
