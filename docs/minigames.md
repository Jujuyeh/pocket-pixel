# Care Minigames

Pocket Pixel should make care actions feel like small cat-specific moments, not
confirmation screens. Each minigame should stay short, readable on a 128x64
1-bit screen, and cheap enough for Arduboy memory.

## Shared Contract

Each care minigame should:

- start only when its matching pet state is active,
- have a small amount of local state,
- provide immediate visual feedback,
- finish in a few seconds or after a small objective,
- clear the matching pet state on success,
- grant the configured XP/money reward,
- save after completion,
- remain easy to force from the debug menu.

Failure should usually reduce the reward or delay completion, not hard-punish
the player. The game is about caring for a cat, not beating arcade levels.

## Feed: Catch Food In The Bowl

State: `HUNGRY`

Status: v1 implemented with a moving 16x8 bowl, 12x12 fish/chicken food
sprites, and falling poop hazards.

Concept: move a food bowl and catch falling food until there is enough in the
bowl for Pixel to eat.

Controls:

- Left/right move the bowl.
- A exits.

Gameplay:

- Fish and chicken pieces fall from the top. Their relative probability comes
  from the active pet profile.
- Poop can fall too and should be avoided.
- Occasional coins can fall and be caught for 50 money. They do not affect bowl
  progress.
- Food preserves the previous approximate fall speed at 30 FPS using subpixel
  movement.
- Poop currently has a 1-in-7 spawn chance.
- Catching food increases the bowl progress.
- Catching poop flashes the display and halves current bowl progress, which can
  make Pixel hungrier if the player exits early.
- Required catches are based on missing half-hearts multiplied by 2. Life `3/6`
  starts with bowl frame `3` and needs 6 food catches.
- When the bowl is full, Pixel eats and recovers to full life.
- Exiting early applies the current bowl progress back to life.
- Feeding costs money per completed bowl.

Visual notes:

- Bowl at bottom with six visual states: empty plus five fill levels.
- Falling fish and chicken are 12x12.
- Poop reuses `poo1` for now.
- HUD elements are below the bowl; hearts are hidden during Feed.
- The current catch/poop/done sounds are simple tone placeholders. The done
  sound is shared by successful care actions, and coin pickups share a distinct
  success sound across Feed, Clean, and Walk.

## Clean: Litter Box Scratch-And-Find

State: `DIRTY`

Status: v2 implemented with a free cursor over a persistent framebuffer canvas
and procedural placeholder art.

Concept: rake or scrape the litter box to clean it, with a chance to reveal
coins.

Controls:

- Directional buttons move the tool tip freely around the litter box.
- B toggles between rake and scoop.
- A exits.

Gameplay:

- The litter box occupies most of the screen.
- The tool tip is the active brush. The handle may extend outside the litter
  box visually, but the brush point is clamped inside the litter area.
- The rake moves sand as it travels, revealing lower sand, hidden poop, and
  hidden coins.
- The scoop does not move sand. It removes already revealed poop pixel by
  pixel.
- Poop is not removed as a whole object. The scoop must erase the visible poop
  shape completely.
- On minigame entry, randomly decide whether a hidden coin exists and its value.
- Raking over the hidden coin reveals and collects a bonus.
- Cleaning all poop shapes completes the action.
- Current v2 uses four hidden poop patches.
- Current v2 has a 35% chance to hide a coin.
- Coin values are 100, 200, 500, or 1000. Higher values are rarer.
- Completing the minigame grants the base clean reward plus any found coin.

Visual notes:

- Litter box can be a shallow rectangle with textured pixels.
- Rake/spatula can be a small handle plus flat edge.
- Coins should eventually get proper 1-bit sprites for 100, 200, 500, and 1000.
- Current v2 uses simple procedural placeholders.
- Revealed poop is drawn as an elongated black shape with a 1-pixel white
  readability border so nearby or overlapping patches remain distinguishable.
- The previous placeholder rake sound was removed because short repeated tones
  felt too percussive. Rake audio should be redesigned with the audio tooling
  before being integrated again.

Art homework:

- Litter box frame and sand texture.
- Rake or scoop tool.
- Dirty clump/poop shapes.
- Coin sprites or readable value markers for 100, 200, 500, and 1000.

## Play: Paw Timing

State: `BORED`

Status: v2 implemented with Pet Studio background, hand, and cat paw drafts.

Concept: tease Pixel with a hand, then pull the hand away before the cat's paw
lands.

Controls:

- B briefly puts the hand out. Holding B does not keep it out.
- In the current implementation the hand is out by default. Pressing B briefly
  retracts it, then it returns automatically.
- A exits.

Gameplay:

- Pixel idles with a slow one-pixel left/right sway.
- Before a jump, Pixel repeats the same sway three times faster.
- The jump uses cat frame 1 and then cat frame 2, where the scratch check
  happens.
- If the hand is still out on the scratch frame, Pixel scratches it, flashes the
  screen, and the progress counter loses 3 points.
- If the hand is retracted on the scratch frame, the counter gains 1 point.
- Reaching 6 points completes the action.
- The wait before each jump is randomized. The active profile's
  `traits.playfulness` controls the range: playful pets pounce again sooner,
  less playful pets wait longer.
- Completing the minigame clears `BORED`, grants the existing Play reward, and
  saves the game.

Visual notes:

- The background and hand are integrated from Pet Studio drafts.
- The hand uses editable masks from `assets/work/sprites/playHandMask.*`.
- Cat paw frames can have different dimensions; they are drawn using a shared
  bottom-left anchor aligned with the background floor.
- Cat paw frames use editable masks from `assets/work/sprites/playCatPawMask.*`.
  Black pixels in those mask drafts mean visible/opaque.
- Keep the timing window readable at 30 FPS with frame-scaled timing constants.

## Walk: Laser Lane Guide

State: `ANXIOUS`

Concept: guide Pixel through three lanes by moving a laser pointer. Pixel
follows the laser with a slight delay and must avoid obstacles.

Controls:

- Hold up/down to aim the laser smoothly between the upper and lower lane.
- A exits.

Gameplay:

- Obstacles appear ahead in one or more lanes and move left as the lane dashes
  scroll, making Pixel feel like he is running forward while staying near the
  same screen X.
- Coins can appear in lanes using the same movement as obstacles. They are
  optional pickups worth 50 money.
- Pixel follows the laser lane with smooth vertical movement and a slight delay.
- The laser target moves smoothly rather than snapping to three valid
  positions.
- Passing an obstacle successfully grants 1 point.
- If Pixel hits an obstacle, the score resets to 0.
- Touching a coin collects it immediately without affecting score.
- Obstacles stay in the world after scoring or hitting and leave the screen
  progressively through the left edge.
- Obstacles and lane dashes enter from the right edge and leave through the
  left edge progressively, with clipping at both screen edges.
- Reaching 16 points clears `ANXIOUS`.
- Collision is intentionally simple: a hit occurs when the lower-right pixel of
  the 16x12 cat sprite touches the lower half of a 12x12 obstacle.

Visual notes:

- Three horizontal lanes use discontinuous dashed lines animated horizontally at
  the same speed as the obstacles. Dash segments are clipped at both screen
  edges instead of wrapping in whole segments.
- Pixel is a 16x12 solid black side-view cat facing right, with one white eye
  pixel, animated with three running frames. Current draft:
  `assets/work/sprites/walkCatRun.{txt,h}`.
- `assets/work/sprites/walkCatRunBorder.{txt,h}` is a generated 18x14
  one-pixel halo mask. Draw it one pixel up-left in white before drawing the
  16x12 cat on top.
- Obstacles are 12x12 sprites. Current drafts live under
  `assets/work/sprites/walkObstacle*.{txt,h}`.
- Draw world objects by the Y coordinate of their bottom reference point so
  objects in lower lanes can appear in front of objects in upper lanes.
- The laser pointer can be procedural: a continuous black line ending in a 3x3
  cross, with the machine drawn from the same aiming vector as two parallel
  rails behind the laser origin. This keeps the device and laser rotating as one
  readable object without storing rotated sprites.

## Water: Spray The Scratches

State: `SCRATCHING`

Concept: use a spray bottle to stop Pixel scratching the wall.

Controls:

- The filled D-pad icon shows the current target button.
- Press the matching directional button repeatedly to spray.
- Pressing more than one directional button at the same time does not count.
- A exits.

Gameplay:

- Scratch marks appear on the wall while Pixel is still scratching.
- The Arduboy RGB LED alternates red/blue while `SCRATCHING` is active.
- Pixel idles with the reused idle sprite. Successful spray feedback draws
  closed eyes briefly.
- Correct presses fill a progress bar. The bar drains smoothly over time.
- The requested button changes at random intervals, so the player has to watch
  the D-pad prompt instead of holding one button.
- Completing the meter clears `SCRATCHING`. Exiting early leaves the scratching
  state active.

Visual notes:

- Keep it playful: this is a "pss pss, stop scratching" moment, not aggression.
- The D-pad prompt and spray bottle use editable project sprite drafts from
  `assets/work/sprites/`.
- Spray arcs can be short dotted lines emitted from the bottle nozzle.
- Pixel can blink or recoil briefly when sprayed.

## Implementation Order

1. Add a reusable minigame state pattern.
2. Implement `Clean` first because it is simple, visual, and tests random bonus
   rewards.
3. Implement `Feed` next because it uses hunger/life and money.
4. Implement `Play`, then `Walk`, then `Water`.
5. Replace placeholder care screens only after each minigame is testable from
   the debug menu.
