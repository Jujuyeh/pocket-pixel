# Personality Model

This reference defines the first Pocket Pixel personality profile model.

## Schema

Required top-level fields:

- `schemaVersion`: integer, currently `1`.
- `name`: display name of the pet.
- `species`: currently `cat`; future values can exist but should not be treated
  as drop-in gameplay equivalents.
- `description`: concise natural-language summary.
- `traits`: score map from 0 to 100.
- `behavior`: derived gameplay values.

Optional top-level fields:

- `evidence`: list of short strings from the user's description.
- `assets`: paths for banner/title/sprites.
- `audio`: pet-specific sound configuration.
- `notes`: implementation or design notes.

## Traits

All traits are 0 to 100:

- `sleepiness`: tendency to nap or be unavailable.
- `appetite`: tendency to ask for food or lose life through hunger.
- `bathroomFrequency`: tendency to become dirty or need cleanup.
- `playfulness`: tendency to request play.
  The game also uses this trait to tune the Play minigame's jump timing: higher
  playfulness makes the cat try to pounce again sooner.
- `affection`: tendency to seek interaction and recover from care.
- `independence`: tendency to ignore player initiative and demand interaction
  only on the pet's terms.
- `anxiety`: tendency to request walks/soothing and become restless.
- `curiosity`: tendency to produce varied/novel states or animations.
- `routineSensitivity`: tendency to prefer repeated care rhythm.

## Behavior Values

Pocket Pixel currently uses "chance as 1/x" values. Lower numbers mean a state
appears more frequently.

Current recovered baseline:

```json
{
  "chanceSleeping": 30,
  "chanceHungry": 90,
  "chanceDirty": 120,
  "chanceBored": 120,
  "chanceAnxious": 200,
  "chanceScratching": 250,
  "chanceMeow": 190,
  "feedCost": 20,
  "fishPreference": 50,
  "chickenPreference": 50
}
```

For Pixel-like behavior, keep sleep frequent and dirty reasonably frequent.

Suggested derivation ranges:

- `chanceSleeping`: 18 to 90; lower for sleepy cats.
- `chanceHungry`: 45 to 180; lower for hungry/food-motivated cats.
- `chanceDirty`: 45 to 220; lower for frequent bathroom cats.
- `chanceBored`: 55 to 220; lower for playful cats.
- `chanceAnxious`: 70 to 255; lower for anxious/restless cats.
- `chanceScratching`: 90 to 255; lower for itchy, dramatic, or high-maintenance
  cats.
- `chanceMeow`: 60 to 255; lower for talkative cats. Dirty pets get a runtime
  boost without needing a separate profile value.
- `feedCost`: 10 to 40; higher only if balancing economy asks for it.
- `fishPreference`: 0 to 100; relative spawn weight for fish in the feed
  minigame.
- `chickenPreference`: 0 to 100; relative spawn weight for chicken in the feed
  minigame.

## Asset Values

`assets.banner` points to the Arduboy FX catalog banner PNG for the profile.
`make fx-entry` and the FX deploy helper use this path when packaging the game.
Pet Studio can upload a replacement PNG from Preferences and will update this
field in the profile.

## Audio Values

`audio.menuMelody` is an optional non-empty list of note names used by the idle
menu cursor sound. Each successful cursor move plays the next note and wraps at
the end.

`audio.meowNote` is an optional note name used as the pet's short meow pitch.
The generated game sound plays that note, the next semitone up, that note again,
and the previous semitone down.

Use note strings such as `C5`, `CS5`, `D5`, or `REST` for melodies. `C#5` is
accepted and generated as `NOTE_CS5`. Keep melodies short; the validator caps
the sequence at 64 notes to keep Arduboy flash usage predictable. `meowNote`
must be a real note, not `REST`.

## Mapping Guidance

Use descriptions, not raw vibes:

- "Sleeps all day", "always in bed", "hard to wake": high sleepiness.
- "Zoomies", "brings toys", "asks to play": high playfulness.
- "Only wants attention when they choose": high independence.
- "Follows me everywhere", "lap cat": high affection, lower independence.
- "Startles easily", "hides from noise": high anxiety.
- "Always investigating boxes/doors": high curiosity.
- "Needs the same routine": high routine sensitivity.
- "Uses litter often", "bathroom frequent": high bathroomFrequency.
- "Always begging for food": high appetite.

## Output Discipline

When Codex creates a profile, include a short explanation:

- traits inferred,
- evidence for non-obvious scores,
- expected gameplay behavior,
- what the user should test in debug builds.

Do not overfit one anecdote. For a one-off behavior, use moderate scores unless
the user says it is typical.
