# RVGL Randomizer — Configuration UI Knowledge Base

> **Scope:** This document covers every setting exposed in the randomizer's
> configuration tabs (Car Options, Stock Cars Spec, DC Cars Spec, Track Options,
> Track Spec, Cup Spec) and their exact runtime semantics — what each field
> controls, how it interacts with the others, and how the values are ultimately
> written to the JSON config consumed by the DLL.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Car Options Tab](#2-car-options-tab)
   - 2.1 [Car Randomization Mode](#21-car-randomization-mode)
   - 2.2 [Allowed Unlock Methods](#22-allowed-unlock-methods)
   - 2.3 [Rating Options](#23-rating-options)
   - 2.4 [Starting Car Configuration](#24-starting-car-configuration)
3. [Stock Cars Spec Tab](#3-stock-cars-spec-tab)
4. [DC Cars Spec Tab](#4-dc-cars-spec-tab)
5. [Cars Spec: Shared Row Structure](#5-cars-spec-shared-row-structure)
   - 5.1 [Car Choice Columns](#51-car-choice-columns)
   - 5.2 [Attribute Columns](#52-attribute-columns)
   - 5.3 [Lock Interactions](#53-lock-interactions)
6. [Track Options Tab](#6-track-options-tab)
   - 6.1 [Track Randomization Mode](#61-track-randomization-mode)
   - 6.2 [Random Obtain Methods](#62-random-obtain-methods)
7. [Track Spec Tab](#7-track-spec-tab)
   - 7.1 [Row Structure](#71-row-structure)
   - 7.2 [Lock Interactions](#72-lock-interactions)
8. [Cup Spec Tab](#8-cup-spec-tab)
   - 8.1 [Stage Mode](#81-stage-mode)
   - 8.2 [Track Variant Flags](#82-track-variant-flags)
   - 8.3 [Random Stage Constraints](#83-random-stage-constraints)
   - 8.4 [Global Cup Settings](#84-global-cup-settings)
   - 8.5 [Per-Cup Cards](#85-per-cup-cards)
   - 8.6 [User-Defined Stages](#86-user-defined-stages)
9. [Data Flow: UI → Rust → JSON → DLL](#9-data-flow-ui--rust--json--dll)
10. [Default State Reference](#10-default-state-reference)
11. [Enum / Constant Reference](#11-enum--constant-reference)

---

## 1. Architecture Overview

The configuration UI is a multi-tab React application backed by a single global
`AppProvider` context. All state lives in `state.configure`:

```
state.configure
├── carOptions          CarOptionsInput     (Car Options tab)
├── carsSpecState
│   ├── includeStockCars  bool
│   ├── includeD̈cCars    bool
│   ├── stockCars[]       CarSpec[28]        (Stock Cars Spec tab)
│   └── dcCars[]          CarSpec[14]        (DC Cars Spec tab)
├── trackOptions        TrackOptionsInput   (Track Options tab)
├── trackSpecState
│   ├── includeTracks    bool
│   └── tracks[]         TrackSpec[14]      (Track Spec tab)
└── cupSpecState        CupSpecState        (Cup Spec tab)
```

When the user clicks **Generate**, the Rust command `generate_result` receives
these objects, runs the randomization algorithm, and writes a `.json` file that
the DLL reads at game startup via `RVGL_RANDOMIZER_CONFIG`.

---

## 2. Car Options Tab

File: `ui/src/configure/carOptions/CarOptionsTab.jsx`

### 2.1 Car Randomization Mode

Controls how **ratings** (`CarInfo.rating`) and **unlock conditions**
(`CarInfo.obtainCondition`) are assigned to every car slot after the source
car has been selected.

| Mode ID | Label | Rating | Obtain |
|---|---|---|---|
| `random` | Full Random | Random | Random |
| `randomRatings` | Random Ratings | Random | Unchanged |
| `randomUnlock` | Random Unlock Criteria | Unchanged | Random |
| `unchanged` | Unchanged | Unchanged | Unchanged |
| `baseGame` | Base Game Distribution | Fixed pattern (see below) | Fixed pattern |

**Selecting a mode** triggers `handleModeSelect`, which:

1. Reads the current `carsSpecState` (or creates defaults from `STOCK_CARS` /
   `DC_CARS` if not yet initialised).
2. Iterates every row in `stockCars` and `dcCars` and calls `applyModeRules`
   to set `attrRating` and `attrObtain` in each row.
3. Writes back both `carsSpecState` and `carOptions` simultaneously.

**Base Game Distribution pattern** — applied directly in `handleModeSelect`
without going through `applyModeRules`:

*Stock cars (28 slots):*

| Slot range | attrRating | attrObtain |
|---|---|---|
| 0–7 | Rookie (0) | Starting Car (0) |
| 8–12 | Amateur (1) | Championship (1) for 0–1, Practice (3) for 2–3, Single Race (4) for 4 |
| 13–17 | Advanced (2) | Same 2-2-1 pattern |
| 18–22 | Semi-Pro (3) | Same 2-2-1 pattern |
| 23–27 | Pro (4) | Same 2-2-1 pattern |

*DC cars (14 slots):*

| Slot | attrRating | attrObtain |
|---|---|---|
| 0 | Rookie (0) | Starting Car (0) |
| 1 | Advanced (2) | Championship (1) |
| 2–4 | Amateur (1) | Time Trial (2) |
| 5–7 | Advanced (2) | Time Trial (2) |
| 8–10 | Semi-Pro (3) | Time Trial (2) |
| 11–13 | Pro (4) | Time Trial (2) |

### 2.2 Allowed Unlock Methods

Visible only when `unlockMode` is `random` or `randomUnlock`.

| Checkbox | Key | Obtain value | Effect |
|---|---|---|---|
| Include Cheat Only | `includeCheatOnly` | `-1` | Adds `-1` to the allowed obtain pool during Rust generation |
| Include Stunt Arena | `includeStuntArena` | `5` | Adds `5` to the allowed obtain pool |

Standard methods `[0, 1, 2, 3, 4]` are **always** in the pool when obtain is
random; these checkboxes only gate the edge cases.

### 2.3 Rating Options

Visible when `unlockMode` is `random` or `randomRatings`.

**Include Super Pro** (`includeSuperPro`, default `true`) — when unchecked,
forces `poolRatingDistributions["5"]` and `attrRatingDistributions["5"]` to
`{ enabled: true, min: 0, max: 0 }`, effectively banning Super Pro from all
rolls.

**Car Pool Rating Distribution** (`poolRatingDistributions`) — constraints on
which *source car ratings* may be picked for each slot (controls the car
selection step, not the attribute assignment step).

**Target Rating Distribution** (`attrRatingDistributions`) — constraints on
how many final rating *attributes* of each tier appear in the output.

Both tables share the same structure, rendered by `RatingDistTable`:

| Column | Meaning |
|---|---|
| Checkbox (`enabled`) | Only active rows participate in constraint allocation |
| Rating | The car rating tier (Rookie → Super Pro) |
| Min | Minimum number of slots that must have this rating |
| Max | Maximum number of slots that may have this rating |

**Allocation algorithm** (Rust, `randomizer/cars.rs → allocate_ratings`):

1. Identify enabled rows; if none are enabled, all ratings in `[0..4]` (or
   `[0..5]` with Super Pro) are treated as unconstrained.
2. Assign minimums first, respecting `max` as a hard cap.
3. Distribute remaining slots randomly among rows that have not yet hit their
   `max`.
4. If explicit maxes are collectively too low to fill all slots, overflow is
   placed in any enabled bucket.
5. Shuffle the final flat list before assigning.

**Normalization** — `CarOptionsUtils.js → normalizeDistributionMap` is called
every time a distribution value changes:

- `min` is clamped to `Math.max(min, fixedCount[rating])` where `fixedCount`
  is the number of spec rows already locked to that rating.
- If the total `sum(min)` exceeds available slots, minimums are reduced
  proportionally, but never below the fixed floor from the spec.
- If all ratings are enabled, `sum(max)` must be able to cover all slots; if
  not, the highest max is expanded automatically.

### 2.4 Starting Car Configuration

Visible when `unlockMode` is not `baseGame`.

**Enable custom starting car configuration** (`enableStartingCars`) — master
toggle; requires `numStartingCars ≥ 1` (auto-set to 1 if toggled on while 0).

**Number of starting cars** (`numStartingCars`, 1–42) — the first N stock slots
are treated as "starting cars". This means:

- In modes `random` / `randomUnlock`, `attrObtain` is forced to `"0"` (Starting
  Car) in the spec rows.
- In modes `unchanged` / `randomRatings`, `sourceObtain` is forced to `"0"`.

**Starting Cars: Set Source Pool** (`enableStartingCarsPool` + `startingCarsPool`)
— when enabled, the source pool dropdown for the first N stock slots is locked
to the chosen pool and the user cannot change it per-row.

**Starting Cars: Set Rating** (`enableStartingCarsRating` + `startingCarsRating`)
— when enabled, the source rating for the first N stock slots is locked to the
chosen rating.

Both locks are enforced in `CarsSpecSection.jsx` by passing `lockStartingPool`,
`lockStartingRating`, and `lockStartingObtain` props to each `CarSpecRow`.

---

## 3. Stock Cars Spec Tab

File: `ui/src/configure/carSpec/StockCarsFullSpecTab.jsx`  
Section component: `ui/src/configure/carSpec/CarsSpecSection.jsx`

Renders 28 rows, one for each stock car slot (index 0–27). The slot ID displayed
is the car's folder name from `STOCK_CARS` constant (e.g. `rc`, `mite`, …).

**Include Stock Cars in Randomization** (`includeStockCars`) — when unchecked:

- The rows are visually greyed out and non-interactive.
- On generation, `stockCars` is passed as an empty array `[]` to Rust, so no
  stock car output is produced.

**Presets** — apply a batch transformation to all rows:

| Preset | sourcePool | sourceDifficulty | attrRating | attrObtain |
|---|---|---|---|---|
| Full Random | `Full Random` | `Random` | (respects mode lock) | (respects mode lock) |
| Original Content | Specific folder for each slot | `Random` | `Unchanged` | `Unchanged` |

Preset application respects all active locks (Base Game Distribution mode, Starting
Car locks, etc.).

---

## 4. DC Cars Spec Tab

File: `ui/src/configure/carSpec/DcCarsFullSpecTab.jsx`

Identical structure to Stock Cars Spec with 14 rows (index 0–13 mapping to
DC car pool entries). Controlled by `includeDcCars` / `dcCars` in
`carsSpecState`. Slot IDs map to DC car folders (`bigvolt`, `bossvolt`, etc.).

---

## 5. Cars Spec: Shared Row Structure

Each row in both car spec tabs has two logical halves.

### 5.1 Car Choice Columns

These columns control **which car is selected** for a given slot. The Rust
resolver (`randomizer/cars.rs → resolve_car_list`) processes slots
fewest-candidates-first to minimise duplicates.

| Column | State key | Description |
|---|---|---|
| **Pool** | `sourcePool` | Determines the candidate set. Options: `Full Random`, `Stock`, `DC`, `Custom`, `Pack:<name>`, or a specific car folder name (set via the search modal). |
| **Rating** | `sourceRating` | Filters candidates by their scanned `rating` value. `Random` = no filter. Disabled when Pool is a specific car. |
| **Obtain** | `sourceObtain` | Filters candidates by their scanned `obtainMethod`. `Random` = no filter. Disabled when Pool is a specific car. |

**Specific Car selection** — clicking `Specific Car…` in the Pool dropdown opens
a search modal (`CarSearchModal.jsx`). Selecting a car writes its folder name as
the `sourcePool` value. When a specific car is selected, the Rating and Obtain
columns display the car's actual scanned values (read-only).

**Fallback chain** in Rust when no candidates remain after full filtering:

1. Relax rating and obtain, keep pool constraint.
2. Relax everything, pick from all available cars.
3. If still empty, return `None` (slot is omitted from output).

### 5.2 Attribute Columns

These columns control **what rating and obtain are written** to the JSON output,
independent of which car was selected.

| Column | State key | Description |
|---|---|---|
| **Rating** | `attrRating` | The final `rating` value written to the config. Options: `Random`, `Unchanged`, or a specific tier (Rookie–Super Pro). `Unchanged` copies the rating from the selected car. |
| **Obtain** | `attrObtain` | The final `obtain` value written to the config. Options: `Random`, `Unchanged`, or a specific method. `Unchanged` copies the obtain from the selected car. |

When `attrRating` is `Random`, the Rust layer draws from the globally-allocated
distribution (if any distribution constraints are active) or rolls `[0..4]` /
`[0..5]` uniformly.

When `attrObtain` is `Random`, the Rust `resolve_obtain` function builds an
allowed list from `[0,1,2,3,4]` plus any extras enabled in Car Options.

### 5.3 Lock Interactions

The following locks grey out columns and prevent user edits:

| Condition | Locked column |
|---|---|
| `unlockMode === "unchanged"` or `"randomUnlock"` | attrRating |
| `unlockMode === "unchanged"` or `"randomRatings"` | attrObtain |
| `unlockMode === "baseGame"` | Both attrRating and attrObtain |
| Slot is in Starting Car range AND `enableStartingCarsPool` | sourcePool |
| Slot is in Starting Car range AND `enableStartingCarsRating` | sourceRating |
| Slot is in Starting Car range | attrObtain (forced to Starting Car) |
| Pool is a specific car folder | sourceRating, sourceObtain |

---

## 6. Track Options Tab

File: `ui/src/configure/trackOptions/TrackOptionsTab.jsx`

### 6.1 Track Randomization Mode

Controls the default `attrDifficulty` and `attrObtain` values that are
auto-populated in the Track Spec rows when a mode is selected. Unlike Car
Options, selecting a mode **immediately updates all track spec rows**.

| Mode ID | attrDifficulty | attrObtain |
|---|---|---|
| `random` | `Random` | `Random` |
| `randomUnlock` | `Unchanged` | `Random` |
| `randomDifficulty` | `Random` | `1` (Championship) |
| `unchanged` | `Unchanged` | `1` (Championship) |
| `baseGame` | Fixed by slot index (see below) | `1` (Championship) |

**Base Game pattern for `attrDifficulty`:**

| Slots | Difficulty |
|---|---|
| 0–3 | Easy (1) |
| 4–7 | Medium (2) |
| 8–10 | Hard (3) |
| 11–13 | Extreme (4) |

### 6.2 Random Obtain Methods

Visible only when `unlockMode` is `random` or `randomUnlock`.

**Enable random obtain methods** (`enableRandomObtainMethods`, default `true`)
— if unchecked, all `Random` obtain attributes fall back to `1` (Championship)
regardless of the per-row setting.

| Checkbox | Key | Obtain value |
|---|---|---|
| Include Cheat Only | `includeCheatOnly` | `-1` |
| Include Unlocked by Default | `includeUnlockedByDefault` | `0` |
| Include Stunt Arena | `includeStuntArena` | `5` |

Standard track obtain methods `[1, 2, 3, 4]` are always in the pool when
random obtain is enabled.

---

## 7. Track Spec Tab

File: `ui/src/configure/trackOptions/TrackSpecTab.jsx`

Renders 14 rows for the 14 race tracks that occupy fixed positions in RVGL's
vanilla track table.

**Include Tracks in Randomization** (`includeTracks`) — when unchecked, the spec
rows are locked and `tracks` is passed as `[]` to Rust; the DLL then uses the
original vanilla track list.

**Presets:**

| Preset | sourcePool |
|---|---|
| Full Random | `Full Random` |
| Original Content | Specific folder for each slot (e.g. `nhood1` for slot 0) |

### 7.1 Row Structure

| Column | State key | Description |
|---|---|---|
| **Pool** | `sourcePool` | Candidate track set. Options: `Full Random`, `Stock`, `Custom`, `Pack:<name>`, or a specific folder name (via search modal). |
| **Difficulty** (choice) | `sourceDifficulty` | Filters candidates by difficulty. `Random` = no filter. Disabled for specific tracks. |
| **Difficulty** (attr) | `attrDifficulty` | Final difficulty written to the JSON. `Unchanged` copies from the scanned track. |
| **Obtain** (attr) | `attrObtain` | Final obtain written to the JSON. `Unchanged` copies from the scanned track. |

The Rust resolver (`randomizer/tracks.rs → resolve_track_list`) uses the same
fewest-candidates-first strategy as cars to avoid duplicate track assignments.

### 7.2 Lock Interactions

| Condition | Locked column |
|---|---|
| `unlockMode === "randomUnlock"`, `"unchanged"`, or `"baseGame"` | attrDifficulty |
| `unlockMode === "randomDifficulty"`, `"unchanged"`, or `"baseGame"` | attrObtain |

---

## 8. Cup Spec Tab

File: `ui/src/configure/cupSpec/CupSpecTab.jsx`

Controls the 4 base cups (Bronze, Silver, Gold, Platinum). The cup system
**depends on the Track Spec** — only tracks that appear in the resolved track
list can be assigned to cup stages.

**Include cups in randomization** (`enabled`) — if unchecked, the DLL receives
minimal default cup data matching the base game layout.

### 8.1 Stage Mode

Determines how stages are built for all cups.

| Mode | ID | Description |
|---|---|---|
| Default Stages | `default` | Mirrors the base game's 4-4-4-5 stage layout using the randomized track list |
| Random Stages | `random` | Stages are drawn randomly from the resolved track pool with optional constraints |
| User-Defined Stages | `userDefined` | Each stage in each cup is configured manually |

**Default Stages layout** (Rust, `cups.rs → build_default_stages`):

| Cup | Tracks used | Mode |
|---|---|---|
| Bronze | resolved[0..3] | Normal |
| Silver | resolved[4..7] | Normal |
| Gold | resolved[8..10] Normal + resolved[4] | Mirror |
| Platinum | resolved[11..13] Normal + resolved[5] (RevMirror or Mirror) + resolved[8] Mirror | Mixed |

The Platinum cup's second bonus stage uses Reverse+Mirror if `resolved[5]` has
a reversed version, otherwise falls back to Mirror only (or finds another
track with a reversed version from slots 0–10, excluding 5 and 8).

### 8.2 Track Variant Flags

Hidden in User-Defined mode.

| Checkbox | Key | Default | Effect |
|---|---|---|---|
| Allow Reverse stages | `allowReverse` | `true` | Unlocks `(isReverse=true, isMirror=false)` as a valid variant (only if the track has a reversed directory) |
| Allow Mirror stages | `allowMirror` | `false` | Unlocks `(isReverse=false, isMirror=true)` |
| Allow Reverse Mirror stages | `allowReverseMirror` | `false` | Unlocks `(isReverse=true, isMirror=true)` (requires reversed directory) |

Normal `(false, false)` is always available regardless of these flags.

### 8.3 Random Stage Constraints

Visible only in Random Stages mode.

**Guarantee first appearance in Normal mode** (`guaranteeFirstNormal`, default
`true`) — when a track appears for the first time across *all* cups, it is
forced to Normal mode regardless of which variants are allowed. Subsequent
appearances may use any allowed variant.

**Same-track handling** (`sameTrackHandling`) — controls what happens when the
same track is picked multiple times within a single cup:

| Option | ID | Behaviour |
|---|---|---|
| Forbid | `forbid` | A track can only appear once per cup |
| Allow Any | `allowAny` | No restriction; the same track and variant may repeat |
| Allow Variants | `allowVariants` | A track may repeat only if a different variant is used each time |

### 8.4 Global Cup Settings

Applies to all cups unless overridden per-cup.

| Setting | Key | Default | Description |
|---|---|---|---|
| Num Cars | `numCars` | 8 | Total racers per stage including the player |
| Num Tries | `numTries` | 3 | Number of attempts allowed to complete the cup |
| Per-Race Place | `perRaceRequiredPlace` | 3 | Player must finish this position or better in each race |
| Overall Place | `overallRequiredPlace` | 1 | Player must finish this overall position to unlock the next cup |
| Laps Min | `numLapsMin` | 6 | Minimum number of laps for a random stage |
| Laps Max | `numLapsMax` | 6 | Maximum number of laps; equal to Min means fixed laps |

**Points Table** — 16 entries for positions 1st through 16th. The last
`16 - numCars` entries are disabled in the editor (greyed out). Default:
`[10, 6, 4, 3, 2, 1, 0, 0, …]`.

### 8.5 Per-Cup Cards

Each of the 4 cups has a collapsible card with:

**Override global settings** (`overrideGlobal`) — when checked, the cup exposes
its own Num Cars, Num Tries, Per-Race Place, Overall Place, Laps Min, Laps Max,
and its own Points Table editor. When unchecked, all these values fall through
to the Global Cup Settings.

**Cars Per Class** (`carsPerClass`) — array of 6 values `[Rookie, Amateur,
Advanced, Semi-Pro, Pro, Super Pro]` specifying how many AI opponents of each
rating class are placed in the cup. The sum must equal `numCars - 1`.

Default distributions:

| Cup | Rookie | Amateur | Advanced | Semi-Pro | Pro | Super Pro |
|---|---|---|---|---|---|---|
| Bronze | 7 | 0 | 0 | 0 | 0 | 0 |
| Silver | 0 | 4 | 3 | 0 | 0 | 0 |
| Gold | 0 | 0 | 4 | 3 | 0 | 0 |
| Platinum | 0 | 0 | 1 | 3 | 3 | 0 |

**Stage count range** (Random mode only) — `numStagesMin` / `numStagesMax`
control how many stages are randomly generated for this cup.

### 8.6 User-Defined Stages

Available per-cup in User-Defined mode. Up to 16 stages per cup.

| Column | Key | Options |
|---|---|---|
| Track Pool | `sourcePool` | `Random`, difficulty level `1`–`4`, or a specific track folder |
| Laps | `numLaps` | `Inherit` (uses global Laps range) or fixed 1–10 |
| Rev | `isReverse` | `Rnd` (random), `No`, `Yes` |
| Mir | `isMirror` | `Rnd` (random), `No`, `Yes` |

When both Rev and Mir are set to `Rnd`, the stage draws from `allowed_variants`
(honoring the Track Variant Flags) — but `guaranteeFirstNormal` is
**automatically disabled** in User-Defined mode.

If Rev is forced to `Yes` but the selected track has no reversed version, it
silently falls back to Normal.

---

## 9. Data Flow: UI → Rust → JSON → DLL

```
UI state (AppProvider)
    │
    ▼
invoke("generate_result", { scanResult, carsSpecState, carOptions,
                             trackSpecState, trackOptions, cupSpecState,
                             fileName, profileName })
    │
    ▼  Rust: randomizer/commands.rs
    │
    ├── collect_available_cars(scanResult)
    ├── allocate_ratings(flexible slots, poolRatingDistributions)  → source ratings
    ├── resolve_car_list(specs, cars, scan)                       → selected cars
    ├── allocate_ratings(random-attr slots, attrRatingDistributions) → attr ratings
    ├── build_randomized_car(car, spec, opts)                     → RandomizedCar[]
    │
    ├── collect_available_tracks(scanResult)
    ├── resolve_track_list(specs, tracks, scan)                   → selected tracks
    ├── resolve_track_difficulty / resolve_track_obtain           → RandomizedTrack[]
    │
    └── generate_cups(cupState, resolvedTracks, scan)             → RandomizedCup[]
    │
    ▼
ConfigData {
    metadata: { seed, version, profileName }
    global_options: { load_extra_cars, load_extra_tracks, load_extra_cups }
    stockCars: RandomizedCar[]   → { folder, rating, obtain, selectable_player, selectable_cpu }
    dcCars:    RandomizedCar[]
    tracks:    RandomizedTrack[] → { folder, difficulty, obtain }
    cups:      RandomizedCup[]   → { name, difficulty, obtainCondition, numCars, numTries,
                                     perRaceRequiredPlace, overallRequiredPlace,
                                     carsPerClass[6], pointsTable[16], stages[] }
}
    │
    ▼  Written to <AppLocalData>/generated/<fileName>.json
    │
    ▼  Passed to RVGL via RVGL_RANDOMIZER_CONFIG environment variable
    │
    ▼  DLL (ConfigManager.cpp) reads and applies:
       ├── Hook_LoadVanillaCarPool  — patches g_VanillaCarPaths, applies rating/obtain overrides
       ├── Hook_LoadCustomCarPool   — skipped if load_extra_cars = false
       ├── Hook_LoadVanillaTracks   — patches folderName[], applies difficulty/obtain
       ├── Hook_LoadVanillaCups     — applies full CupProfile data
       └── Hook_UpdateCarSelectability — re-applies overrides after engine sync
```

**global_options flags** are currently hardcoded in Rust output:

```rust
load_extra_cars:   false   // Custom pool cars are not loaded
load_extra_tracks: false   // Custom tracks are not loaded
load_extra_cups:   true    // Custom cups are loaded (allows DLL to skip the hook early)
```

These can be made configurable in a future settings panel.

---

## 10. Default State Reference

Defined in `ui/src/utils/constants.js` and `AppProvider.jsx`.

### DEFAULT_CAR_OPTIONS

```js
{
  unlockMode: "random",
  enableStartingCars: false,
  numStartingCars: 0,
  enableStartingCarsPool: false,
  startingCarsPool: "Full Random",
  enableStartingCarsRating: false,
  startingCarsRating: "Random",
  includeCheatOnly: false,
  includeStuntArena: false,
  includeSuperPro: true,
  poolRatingDistributions: {
    "0": { enabled: false, min: 0, max: 42 },
    "1": { enabled: false, min: 0, max: 42 },
    // … same for "2"–"5"
  },
  attrRatingDistributions: { /* same structure */ }
}
```

### DEFAULT_TRACK_OPTIONS

```js
{
  unlockMode: "random",
  enableRandomObtainMethods: true,
  includeCheatOnly: false,
  includeUnlockedByDefault: false,
  includeStuntArena: false
}
```

### Default CarsSpecState

28 stock car rows + 14 DC car rows, each initialised as:
```js
{ id: "<folderName>", sourcePool: "Full Random", sourceRating: "Random",
  sourceObtain: "Random", attrRating: "Random", attrObtain: "Random" }
```

### Default TrackSpecState

14 track rows, each initialised as:
```js
{ id: "<folderName>", sourcePool: "Full Random", sourceDifficulty: "Random",
  attrDifficulty: "Random", attrObtain: "Random" }
```

### Default CupSpecState

```js
{
  enabled: true,
  stageMode: "default",
  guaranteeFirstNormal: true,
  sameTrackHandling: "forbid",
  allowReverse: true,
  allowMirror: false,
  allowReverseMirror: false,
  numCars: 8,
  numTries: 3,
  perRaceRequiredPlace: 3,
  overallRequiredPlace: 1,
  pointsTable: [10, 6, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  numLapsMin: 6,
  numLapsMax: 6,
  cups: [ /* 4 CupSpec objects, index 0–3 */ ]
}
```

---

## 11. Enum / Constant Reference

### Car Ratings (`CarRating` / `carOptions.rating`)

| Value | Label |
|---|---|
| `-2` | Unknown |
| `-1` | None |
| `0` | Rookie |
| `1` | Amateur |
| `2` | Advanced |
| `3` | Semi-Pro |
| `4` | Pro |
| `5` | Super Pro |

### Obtain Methods (`Obtain` / `obtainCondition`)

| Value | Label |
|---|---|
| `-1` | Cheat Only |
| `0` | Starting Car / Unlocked by Default |
| `1` | Championship |
| `2` | Time Trial |
| `3` | Practice |
| `4` | Single Race |
| `5` | Stunt Arena |

### Track Difficulties

| Value | Label |
|---|---|
| `1` | Easy |
| `2` | Medium |
| `3` | Hard |
| `4` | Extreme |

### Stock Car Slot Order (indices 0–27)

```
0:rc  1:mite  2:phat  3:moss  4:mud  5:beatall  6:volken  7:tc6
8:dino  9:candy  10:gencar  11:tc4  12:mouse  13:flag  14:tc2  15:r5
16:tc5  17:sgt  18:tc3  19:adeon  20:fone  21:tc1  22:rotor  23:cougar
24:sugo  25:toyeca  26:amw  27:panga
```

### DC Car Slot Order (indices 0–13 in dcCars, maps to game indices 35–48)

```
0:bigvolt  1:bossvolt  2:jg6rc  3:tc12  4:tc10  5:tc8  6:tc11
7:tc9  8:jg1jg7  9:tc7  10:jg3loco  11:jg4snw35  12:jg5purpxl  13:jg2fulonx
```

### Stock Track Slot Order (indices 0–13)

```
0:nhood1  1:market2  2:muse2  3:garden1  4:roof  5:toylite  6:wild_west1
7:toy2  8:nhood2  9:ship1  10:muse1  11:market1  12:wild_west2  13:ship2
```
