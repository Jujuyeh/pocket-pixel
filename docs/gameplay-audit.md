# Gameplay Audit

This is the current known behavior after recovering and stabilizing the old
sketch.

## Core Loop

Pocket Pixel is a small virtual pet game. The main screen shows the pet, status
HUD, XP bar, money counter, and a right-side command drawer.

Startup skips the default Arduboy boot logo sequence. The sketch still runs the
standard hardware, flashlight, system-button, and audio initialization, but it
does not show the scrolling ARDUBOY logo or its RGB LED animation. It then
shows a lightweight custom boot animation using `bootLogo24x12`: the logo
shakes in the center over a black field, white particles fall, random-looking
white pixels fill the display with increasing speed, and a short center-out
white curtain finishes the transition into the normal white game background.
Pressing any button skips the custom animation.

The pet can randomly enter these states:

- `SLEEPING`: the pet sleeps and cannot be disturbed.
- `HUNGRY`: the pet needs food.
- `DIRTY`: the pet needs cleaning.
- `BORED`: the pet wants to play.
- `ANXIOUS`: the pet wants to walk.
- `SCRATCHING`: the pet is scratching and needs water.

The menu only opens an action when the matching state is active. Selecting an
inactive action plays a short denied sound.

Feed, play, clean, walk, and water now have playable minigames. The intended
minigame design is tracked in `docs/minigames.md`.

Random state frequency and fish/chicken spawn preference now come from the
active personality profile generated from `profiles/pixel.json`.

## Controls

On the idle screen:

- Left opens the right-side command drawer.
- Right closes the command drawer.
- Up/down move the menu cursor while the drawer is open.
- Each successful cursor move plays the next short note in a looping
  menu-navigation melody from the active personality profile.
- When the pet is otherwise idle, it may meow using the active profile's meow
  chance and pitch. The sound is a four-note pattern based on the profile note:
  base, one semitone up, base, one semitone down. `MEOW` appears below the
  sleep-Z area while the sound plays. Dirty pets are slightly more likely to
  meow.
- B selects the highlighted action while the drawer is open. Pressing B while
  the drawer is closed opens it.
- The command drawer is closed by default and keeps a narrow strip visible as
  an affordance.
- The pet and attached status effects recenter as the drawer opens or closes.
- The XP bar expands or contracts with the drawer and keeps the fill
  proportional to current XP.
- Money is shown in the top-right corner with a `$` marker.

On the placeholder walk screen:

- A exits back to idle.
- B performs the action.

On the feed minigame:

- Left/right move the bowl.
- Fish and chicken pieces increase the bowl fill when caught.
- Catching a chicken costs 20 money. Catching a fish costs 50 money.
- If a food catch leaves money at 0, the screen flashes like a poop hit but the
  bowl/hunger progress is not reduced.
- If current money cannot cover the food cost, caught food flashes like a poop
  hit, does not count toward the bowl, and does not change money.
- The feed money readout blinks slowly when money is below 100.
- Falling coins become more likely while money is below 100.
- Falling poop reuses the existing poop sprite; catching it halves the current
  bowl progress, therefore increasing hunger, and flashes the display.
- Occasional falling coins can be caught for 50 money without changing bowl
  progress.
- Food falls slightly faster than the first Feed version, and poop is slightly
  more frequent.
- Hearts are hidden during the minigame. The bowl starts on the frame matching
  current life. For example, life `3/6` starts on bowl frame `3`.
- The required number of fish/chicken pieces is the missing half-heart count
  multiplied by 2. For example, life `3/6` needs 6 food catches.
- Completing the bowl sets life to full, grants XP, saves, and returns to idle.
- A exits early, saves, and sets life to the current bowl progress.

On the clean minigame:

- Directional buttons move the tool tip freely inside the litter box.
- B toggles between rake and scoop mode.
- Rake mode moves sand and reveals hidden poop or coins.
- Scoop mode removes revealed poop pixel by pixel and collects revealed coins.
- Revealed poop patches are elongated and drawn with a 1-pixel white readability
  border.
- Collected coins play the shared coin pickup sound.
- A exits without completing the cleanup.
- Cleaning all poop clears `DIRTY`, grants XP, grants the base clean reward, and
  adds any hidden coin found while raking.

On the play minigame:

- A exits back to idle.
- The hand is out by default and loops through the three hand frames.
- B briefly retracts the hand; holding B does not keep it retracted.
- Pixel idles, warns with three fast left/right sways, jumps, then reaches a
  scratch frame.
- If the hand is still out on the scratch frame, the screen flashes and the
  progress counter loses 3 points.
- If the hand is retracted on the scratch frame, the progress counter gains 1
  point.
- Reaching 6 points clears `BORED`, grants the existing Play reward, saves, and
  returns to idle.

On the walk minigame:

- A exits back to idle and leaves `ANXIOUS` active.
- Holding up/down moves a laser target smoothly between the upper and lower
  lane.
- Pixel follows the laser smoothly with a slight delay while running in place.
- Dashed lane lines scroll left at the same speed as obstacles to imply forward
  movement.
- Obstacles move from right to left through the lanes and remain visible until
  they leave the screen.
- Lane dashes and obstacles are clipped at the screen edges so they enter and
  leave progressively.
- Passing an obstacle grants 1 point.
- Hitting an obstacle resets the score to 0 and flashes the screen.
- Coins can appear like lane objects. Touching one collects it, grants 50
  money, plays the shared coin pickup sound, and shows a short `+50` feedback.
- Reaching 16 points clears `ANXIOUS`, grants the existing Walk reward, saves,
  and returns to idle.
- Collision uses the lower-right pixel of the 16x12 running cat against the
  lower half of a 12x12 obstacle.

On the water minigame:

- A exits back to idle and leaves `SCRATCHING` active.
- The filled D-pad icon on the right shows which directional button to press.
- Pressing exactly that one directional button fills the spray meter and shows
  short spray particles. Pressing multiple directions at once does not count.
- The requested direction changes at random intervals.
- The spray meter drains smoothly over time, so the player has to keep up.
- Water is tuned to require a slightly more active spam rhythm than the first
  playable version.
- Scratch marks remain visible during the minigame, but the water minigame does
  not apply the idle-screen money penalty while it is active.
- Completing the meter clears `SCRATCHING`, grants XP, plays the shared success
  tone, saves, and returns to idle.
- While `SCRATCHING` is active, the Arduboy RGB LED alternates red and blue.
  The scratching penalty is applied once per scratch pulse while the idle-screen
  scratching animation is active, not in the water minigame.

On FX-C linked play:

- Connecting two FX-C units wakes both pets into linked idle and shows
  `INVITE?`.
- After confirming an invite, the guest pet leaves its console, a shuttered menu
  appears on the guest console, and the host screen shows both pets.
- The visit menu offers `BALL`, `WATER`, and `FOOD`. `BALL` starts Ball Hunt,
  `WATER` starts Water Battle, and `FOOD` is protocol-reserved but still falls
  back to Ball Hunt until Food Rush is implemented.
- In Ball Hunt, each player controls their own pet at the bottom of their screen
  and holds `B` to return the puck.
- In Water Battle, each console plays the Water spray meter locally; progress is
  shared and the first side to fill the meter wins.
- Holding `A` for roughly three seconds exits linked minigames.
- Linked minigames grant XP and hunger cost, then keep the invited pet on the
  host screen and the guest console in its linked menu.

On debug builds only:

- A+B+Up toggles the debug menu.
- Up/down select a debug row.
- Left/right edits the selected counter or flag.
- Holding B while pressing left/right uses larger counter steps where useful.
- B toggles selected flag rows and applies command rows.
- A closes the debug menu.
- Debug labels are abbreviated to keep the debug build under the Arduboy flash
  limit.

## Stats

- Life ranges from 1 to 6 and is rendered as three hearts.
- Hunger is derived from life. The pet is hungry whenever life is below
  `MAX_LIFE`.
- Random hunger events reduce life over time.
- While sleeping, random events cannot add hunger, dirtiness, boredom, anxiety,
  or scratching. Sleeping also suppresses dirty/scratching presentation if a
  debug or legacy state ever contains overlapping flags.
- Feeding is now a catch-food minigame. Completion restores life to full and
  food catches cost money. Exiting early applies partial bowl progress to life.
- Care actions grant XP and some actions grant money.
- Cleaning can reveal hidden coins worth 100, 200, 500, or 1000. Higher values
  are rarer.
- XP is stored as tenths to avoid floating point math on AVR.
- Level increases when XP reaches a full bar.

## Persistence

The pet state is saved to EEPROM with:

- a magic value,
- a save format version,
- the `PetState` payload,
- a simple XOR checksum.

Save address `128` is used to avoid the first EEPROM bytes. The game writes on
intentional care actions instead of writing every frame.

Invalid save data resets to the default recovered state:

- full life,
- level 0,
- one tenth of the XP bar filled,
- 1000 money,
- no active status flags.

## Known Technical Notes

- The game runs at 30 FPS. Timing constants are expressed relative to the
  recovered 10 FPS baseline through small frame-scaling helpers so existing
  random events, blinking, sounds, and animations keep roughly the same duration
  while movement can render more smoothly.
- The original `unsetPet()` implementation cleared all status flags. It has been
  replaced by single-bit clearing in `Pet.cpp`.
- The original menu already had states for play, clean, walk, and water, but
  only feeding was implemented.
- Asset data now lives in `src/Assets.*`. Bitmap data remains in `PROGMEM`.
- HUD, menu, and button-hint drawing now lives in `src/Render.*`.
- Arduboy itself is 1-bit. Gray fringes or ghosted-looking lines in local
  emulation are expected to come from RetroArch scaling, smoothing, shaders, or
  overlays, not from the game framebuffer. The local `make libretro` target uses
  `config/retroarch/arduboy-clean.cfg` to force clean pixel output.
- The game now clears the screen buffer to white at the start of every frame and
  redraws the current scene. This avoids stale pixels from partial updates and
  makes emulator artifacts easier to separate from actual game rendering bugs.
- Debug builds define `POCKET_PIXEL_DEBUG=1` and include a hidden menu for
  editing life, resources, XP, level, status flags, and resetting the save.
  Stable builds compile the debug handler as a no-op.
- Feed, play, clean, walk, and water are implemented care minigames.
- Successful care actions share the Feed completion tone.
- Clean uses the Arduboy framebuffer as a persistent canvas while that scene is
  active, so the main loop intentionally does not clear the screen every frame
  during `CLEAN`.
- Clean tools are drawn as a temporary overlay: the bytes under the tool are
  backed up before drawing and restored on the next frame. Only the brush effect
  changes the persistent litter canvas.
- The clean tool overlay backup must stay within its fixed page buffer. The
  tool drawing currently fits in a narrow vertical band around the cursor; do
  not increase the saved vertical bounds without increasing the backup storage.

## FX-C Link Minigame

`BUILD=fxc` adds a first link-cable Air Hockey minigame between two Arduboy
FX-C units. When a peer is detected in idle, the pet wakes up, the normal care
menu is blocked, a blinking `!` appears, and `INVITE?` is shown. Press `A` to
open a small confirmation prompt, left/right choose `YES` or `NO`, `B` confirms,
and `A` cancels.

Invites now start a compact visit-data exchange. Each console sends a small
personality packet plus the two 26x24 idle frames in 5-byte sprite chunks. The
invited pet walks down offscreen, waits briefly, then a shop-style shutter drops
before a small `MENU` panel appears. The inviting console keeps its pet onscreen
and draws the invited pet arriving from the bottom once both idle frames have
been received. The menu currently shows `BALL`, `WATER`, and `FOOD`; up/down
wrap around the list. `BALL` launches Ball Hunt, `WATER` launches Water Battle,
and `FOOD` is protocol-reserved but still falls back to Ball Hunt until Food
Rush is implemented.

When both pets are together on the inviting console, a lightweight conversation
cycle runs every 5 seconds. Each cycle has a 25% chance to stay silent. The
remaining outcomes are evenly split between a heart, chicken, fish, spiral,
cross, or an empty topic. While a non-silent topic is active, both pets
alternate short `MEOW` calls using their own profile pitch; the guest pitch is
sent over the link with the visit profile. The heart, chicken, and fish reuse
existing game sprites; the spiral and cross are drawn procedurally.

Controls:

- Left/right moves the pet horizontally.
- Holding `B` lunges to return the puck.
- The pet cannot move while lunging.
- Hold `A` for 3 seconds to exit early.

Ball Hunt is first to 4 points. Each console draws only its own pet; the puck
and score are shared across the two screens. The host console owns puck physics
and score; the peer sends input and receives authoritative puck/score state. A
completed match grants XP and makes the pet hungrier.
