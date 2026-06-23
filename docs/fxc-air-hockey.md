# FX-C Air Hockey

Pocket Pixel includes a first Arduboy FX-C link-cable minigame behind
`POCKET_PIXEL_FXC_LINK`, compiled with:

```sh
make compile-fxc
```

## Physical Setup

Place both Arduboy FX-C units on a table with the tops of the screens facing
each other. The shared rink boundary is the top edge of each screen. Each player
defends from the bottom of their own screen.

## Activation

When two FX-C units detect each other over the USB-C link cable, both pets wake
up and stay in a surprised linked-idle state. The normal care menu is blocked,
the pet shows a blinking `!`, and the idle screen shows:

```text
A LINK
```

Press `A` on either unit to invite the other pet to play. Both units enter the
air hockey scene. Hold `A` for 3 seconds during the match to leave early, or
disconnect the link cable.

## Controls

- Left/right: move the pet horizontally.
- Hold `B`: the pet lunges to return the puck.

The pet cannot move laterally while `B` is held. This keeps the interaction
timing-based instead of letting the player sweep the full goal while attacking.

## Rules

- The match is best of 7; the first side to 4 points wins.
- Each screen draws only its own pet at the bottom edge. The puck and score are
  shared across both screens.
- The host console simulates puck physics, score, and round resets.
- The peer console sends local input and receives the authoritative puck/score
  state.
- The puck bounces from the side walls.
- A strike returns the puck only if the pet is lunging when the puck reaches the
  pet's defensive line.
- The return angle depends on where the puck hits the pet.
- If the puck passes the pet, or crosses over the pet while it is not lunging,
  the opponent scores.
- After each score, the display flashes briefly, the score blinks, and the
  scored-on side serves the next puck.

## Personality

The first implementation derives air-hockey behavior from `playfulness` so it
does not need new profile schema yet:

- Lower playfulness gives slightly better lateral movement.
- Higher playfulness gives slightly more reach and return power.

Future profile work should replace this with explicit balanced values, such as
mobility, reach, and power, with a capped total budget so one pet cannot be best
at every dimension.

## Space Tradeoffs

The FX-C build keeps the normal stable/debug builds unchanged, but it makes one
FX-C-only flash tradeoff and one FX-C-only runtime optimization:

- draws the Play minigame background procedurally instead of keeping the large
  bitmap background in flash.
- uses a tiny sketch-owned `main()` so the flashcart-launched game does not pull
  in Arduino's USB CDC attach path. The FX-C link cable still works because
  multiplayer uses `ArduboyI2C` over the USB-C connector pins, not USB CDC.

This keeps the first link-cable build within the ATmega32U4 flash limit.
