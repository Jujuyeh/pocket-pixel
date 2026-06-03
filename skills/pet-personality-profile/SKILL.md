---
name: pet-personality-profile
description: Create, validate, and apply Pocket Pixel virtual pet personality profiles from rich natural-language descriptions of real cats or cat-like pets. Use when Codex needs to tune gameplay behavior such as sleepiness, hunger, bathroom frequency, affection, independence, anxiety, playfulness, curiosity, allowed interactions, pet name/title cards, or future pet-specific sprites/assets without asking the user for raw numeric parameters.
---

# Pet Personality Profile

Use this skill to translate a detailed real-pet description into a reproducible
Pocket Pixel personality profile and, when requested, apply it to the game.

The user should describe the animal naturally. Do not ask for raw numeric
parameters unless the description is too ambiguous to infer behavior.

## Workflow

1. Read the pet description and extract concrete behavioral evidence.
2. Infer trait scores from 0 to 100 with short justifications.
3. Derive gameplay values from the trait scores.
4. Save a profile JSON under the project, normally `profiles/<slug>.json`.
5. Validate the JSON:

```sh
skills/pet-personality-profile/scripts/profile_tool.py validate profiles/<slug>.json
```

6. If the project supports generated profile headers, render one:

```sh
skills/pet-personality-profile/scripts/profile_tool.py header \
  profiles/<slug>.json \
  --output src/generated/ActivePersonalityProfile.h
```

7. Compile stable/debug and document behavior changes.

## Project Integration Guidance

For Pocket Pixel, move toward a profile-aware architecture:

- Keep the `.ino` minimal.
- Add a `Personality` or `Profile` module when implementation begins.
- Keep profile data static and AVR-friendly; avoid dynamic allocation.
- Keep generated files clearly marked and reproducible from JSON.
- Keep pet-specific art optional:
  - profile JSON can reference asset paths,
  - profile JSON can later reference named sound variants,
  - default sprites remain valid when no custom sprites exist.
- Keep cat behavior as the current gameplay model:
  - actions are available when the pet wants them,
  - the player mostly responds to the pet,
  - do not turn cat profiles into dog-like always-available commands.

Future dog support should be treated as a deeper gameplay model change, not only
a different personality profile.

## Profile Authoring

Read `references/personality-model.md` when creating or tuning profiles. It
defines the schema, trait meanings, derivation rules, and current Pocket Pixel
baseline.

Minimum JSON shape:

```json
{
  "schemaVersion": 1,
  "name": "Pixel",
  "species": "cat",
  "description": "Natural-language summary of the real pet.",
  "traits": {
    "sleepiness": 85,
    "appetite": 50,
    "bathroomFrequency": 80,
    "playfulness": 45,
    "affection": 55,
    "independence": 75,
    "anxiety": 25,
    "curiosity": 60,
    "routineSensitivity": 70
  },
  "behavior": {
    "chanceSleeping": 30,
    "chanceHungry": 90,
    "chanceDirty": 120,
    "chanceBored": 120,
    "chanceAnxious": 200,
    "chanceScratching": 250,
    "feedCost": 20,
    "fishPreference": 50,
    "chickenPreference": 50
  },
  "assets": {
    "banner": "assets/fx/banner.png"
  },
  "audio": {
    "menuMelody": ["E5", "E5", "F5", "G5", "G5", "F5", "E5", "D5"]
  }
}
```

## Decision Rules

- Prefer behavior backed by description over cute but unsupported assumptions.
- Preserve the user's pet identity: name, quirks, constraints, and care pattern.
- If the user says "like Pixel", use the current baseline as strong evidence.
- If conflicting evidence appears, encode it explicitly in the summary and pick
  conservative mid-range scores.
- For cats, keep high independence meaningful: fewer player-initiated actions,
  more state-driven interactions.
- For highly anxious cats, avoid making every state punitive; prefer more
  anxious/walk states but keep recovery possible.
- For sleepy cats, increase sleep frequency and reduce play demand.
- For frequent bathroom cats, increase dirty-state frequency.
- Use `traits.playfulness` for both bored-state tendency and Play minigame
  pounce timing; high values should make play interactions more eager/frequent.

## Useful Commands

Validate:

```sh
skills/pet-personality-profile/scripts/profile_tool.py validate profiles/pixel.json
```

Summarize:

```sh
skills/pet-personality-profile/scripts/profile_tool.py summarize profiles/pixel.json
```

Render a header:

```sh
skills/pet-personality-profile/scripts/profile_tool.py header profiles/pixel.json \
  --output src/generated/ActivePersonalityProfile.h
```
