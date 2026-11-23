# MyCraft — Cozy Garden
A Lab 9 source update for Arya's existing MSPM0G3507 CCS project, not a standalone CCS project.

## Install
1. Back up your project. Replace its active `Lab9Main.c`, `Sound.c`, and `Sound.h` with these files. Add `CozySounds.h` beside Sound.c.
2. Replace `../inc/DAC5.c` with the supplied DAC5.c, or retain your existing DAC driver. Ensure exactly one DAC5.c is compiled.
3. Replace `LED.c` and `LED.h` with the included versions (the diagnostic uses only the on-board PB27 LED). Keep your existing ADC1.c/ADC1.h, Switch.c/Switch.h, LaunchPad, Clock, Timer and ST7735 library sources. Your pasted ADCinit/ADCin and active-low Switch_In interfaces match this update.
4. Remove/exclude old duplicate main/ISR source files. No images/images.h or sounds/sounds.h is needed by this version.
5. Keep the supplied 80 MHz clock setup. Both game timer and audio use that frequency. Changing the CPU clock requires changing both timer constants.
6. Default LCD initialization is INITR_REDTAB. If your panel requires INITR_BLACKTAB, change MYCRAFT_LCD_TAB in Lab9Main.c. Use portrait 128x160 orientation.
7. Build in CCS, flash, and run the hardware checklist below.

## Controls
- Pot PB18: left end moves left, middle stops, right end moves right. This is velocity control, not absolute position.
- PB16 UP: jump; repeated presses permit climbing.
- PB19 DOWN: cycle dirt / stone / wood / leaves.
- PB12 MINE: collect the block in front, otherwise below; advance menus.
- PB17 BUILD: place the selected material in front.
- Buttons connect to GND when pressed; keep your internal pull-ups.
- Mine three crystals and leave five player-placed blocks in the world. They need not form a particular house shape. Avoid the slime; three collisions end the round.
- Mining a placed block returns its material but removes its building progress and gives no mining points. Building itself gives no points, so repeated placing/mining cannot farm score.

## What changed
The uploaded active main was a permanent button test. This replaces it with language selection, instructions, gameplay, win/loss, and replay. The original world/character art is adapted to a pastel cherry-garden palette, pink shirt, cream instructions, heart details and a dark readable inventory. Actual moving sprites are the player, bunny, butterfly, and slime.

TIMG12 samples ADC and debounces each button at 30 Hz (two consecutive changed samples), then sends snapshots through a 15-entry usable queue. Main drains input before rendering. Full queues increment MyCraft_DroppedInputs: inspect this during testing. LCD access never occurs in an ISR. Tile caching redraws changed background and previous actor cells rather than repainting the whole display every tick. Camera changes still redraw the world. RAM does not contain a full-screen framebuffer.

SysTick plays four original generated 5-bit PCM effects at approximately 11 kHz, using interrupt priority 1; TIMG12 uses priority 2. New sounds replace current sounds. The final sample receives a full sample interval before the DAC returns to midscale. These are 5-bit samples: do NOT use the original 8-bit sound arrays with this Sound_Start.

## Important hardware validation
This package has host-side logic/render checks, NOT a CCS build or physical-board test. The TI SDK, your exact course driver versions and hardware are not present here. In particular:
- Your ADCin busy-waits without a timeout. Verify it returns reliably at the supplied clock before relying on it inside the timer ISR. If it stalls, the game timer will stall; fix/test the ADC driver in your ADC test project first.
- A 30 Hz sampling rate requires input bandwidth below 15 Hz for Nyquist compliance. Software smoothing after sampling reduces jitter but does not prevent analog aliasing. For a physical anti-alias pole, a 10 kOhm series resistor and 2.2 uF capacitor to GND at PB18 yield about 7.2 Hz before pot source resistance; validate acquisition settling and control response. One pole is not a brick-wall filter.
- Measure 30 Hz timer cadence and ~11 kHz audio output on the board; watch MyCraft_DroppedInputs during camera scrolling and sound playback.
- Verify all four active-low switches, pot endpoints, LCD tab colors, and safe amplifier gain.
- Confirm no duplicate SysTick_Handler or TIMG12_IRQHandler in the CCS project.
- Run both languages, mine all three visible surface crystals, place five blocks, win, restart, and lose to the slime.
- Check flash/RAM usage in CCS linker output. Existing course libraries determine final memory consumption.

## Proposal alignment
This preserves your existing platformer: UP jumps and DOWN selects a material. Update the proposal to these controls rather than claiming independent vertical movement or a joystick. The game has a sunny scene, not a day/night cycle. The building goal counts five blocks currently placed, not a shape-validated shelter.

## Art and licensing
Sprites and procedural tile drawing are adapted from your supplied source; no third-party image files are embedded. Sounds were generated for this package. An optional future source is Drummyfish's Universal 8x8 Tileset, listed CC0 on OpenGameArt: https://opengameart.org/content/8x8-tileset . It is not included or claimed as integrated.

## Host checks
Run `python3 tests/run.py` (GCC required). This compiles the actual game source against test-only stubs and checks debounce, queue bounds, placement/recycling, win/loss/restart setup, movement boundaries, and render coordinates. It does not validate TI register behavior or target timing. A PPM render is produced for visual inspection.


## Lab procedure test modes
Set MYCRAFT_TEST_MODE in Lab9Main.c to 0 (ADC), 1 (languages), 2 (tiles and animated sprites), 3 (switches/on-board LED), 4 (audio), or 5 (complete game, default). Rebuild and flash each mode separately; main dispatches to the corresponding main0..main5. Return to 5 for checkout. Test routines use English diagnostic labels; the final game provides English/Spanish text.

The existing repository ZIP is the historical starter project. The root source files here are the current overrides: copy them into your existing project as directed above. Do not expect the historical ZIP to contain these updates.

See REQUIREMENTS.md for the rubric audit and VIDEO_PLAN.md for the required recording outline. Neither host tests nor adding test entry points is evidence that hardware checkout was completed.


## Readability update
The game uses an embedded 5x7 font with exact pixel placement. Main menu choices, action labels and score are 2x size (10x14 glyphs with 12px advance). Small supporting text is dark on cream or white on a dark HUD. Instructions now have two pages; press MINE to advance and again to play. The HUD shows score, progress and only the selected material, removing crowded inventory boxes. No new header is required for this font; it is embedded in Lab9Main.c.
