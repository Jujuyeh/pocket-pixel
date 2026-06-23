# FX-C Linked Play

Pocket Pixel includes Arduboy FX-C link-cable gameplay behind
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
INVITE?
```

Press `A` on either unit to open the invite confirmation prompt. Left/right
choose `YES` or `NO`, `B` confirms, and `A` cancels.

After an invite, the invited pet leaves its own console, the guest console drops
a shutter menu, and the host console keeps both pets together. While both pets
are on the host screen, a 5-second conversation cycle runs between them. 25% of
cycles stay silent. The rest choose heart, chicken, fish, spiral, cross, or an
empty topic; while a topic is active, both pets alternate short meows using
their own profile pitch.

## Ball Hunt Controls

- Left/right: move the pet horizontally.
- Hold `B`: the pet lunges to return the puck.

The pet cannot move laterally while `B` is held. This keeps the interaction
timing-based instead of letting the player sweep the full goal while attacking.

## Ball Hunt Rules

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

## Water Battle

The linked visit menu can also start Water Battle. It reuses the local Water
spray rhythm as a race:

- Each console plays its own spray prompt and progress bar.
- The link shares progress and the host decides the winner.
- The first side to fill the meter wins.
- Holding `A` for roughly 3 seconds exits, matching Ball Hunt's accidental-exit
  protection.

Food Rush is reserved in the link protocol and menu, but still falls back to
Ball Hunt until there is enough flash budget to implement it properly.

## Space Tradeoffs

The FX-C build keeps the normal stable/debug builds unchanged, but it makes one
FX-C-only flash tradeoff and one FX-C-only runtime optimization:

- draws the Play minigame background procedurally instead of keeping the large
  bitmap background in flash.
- uses a tiny sketch-owned `main()` so the flashcart-launched game does not pull
  in Arduino's USB CDC attach path. The FX-C link cable still works because
  multiplayer uses `ArduboyI2C` over the USB-C connector pins, not USB CDC.

This keeps the first link-cable build within the ATmega32U4 flash limit.
