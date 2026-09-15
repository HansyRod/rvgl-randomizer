**v0.2.0**

New game modes:

- 30-car mode: Race with up to 30 cars in Single Race and Championship modes.
    - Starting grids are generated based on the track geometry.
    - All cup configuration settings now allow up to 30 cars if the 30-car mode is active.
- Knockout mode: Adds a new mode similar to Single Race, where the last-placed cars can be eliminated during a race.
    - Configure lap count, elimination rate and car state after elimination in the new "Mod options" menu.

New randomization features:

- Custom unlocks: Unlock cars and tracks in new ways not supported in the original game.
    - Win Single Races, catch Practice Stars, and beat Time Trials in specific tracks
    - Win Single Races, catch Practice Stars, and beat Time Trials in a number of tracks
    - Catch a number of Stunt Arena stars
- Fixed opponent cups: Play randomized cups with a specific set of CPU opponents.
- Extra car slots: Add new slots for car randomization beyond the base 28 stock cars + 14 DC cars.
- Limit cup race length: Automatically adjust the number of laps in a cup stage to prevent going above a maximum race length.

Other features:

- Load extra content: You can now load cars, tracks and cups into the game that weren't used in the randomization process.
- Version verification: Checks your rvgl.exe as soon as it's selected to prevent setting up an incompatible version.
- Extended Progress Table: The Progress Table now displays all tracks that were loaded in the game.
- Track length is now displayed in the "Tracks" tab of "Setup" for both Normal and Reverse versions.

Preset changes:

- Added new presets: "30-Car Cups" and "Custom Unlocks"
- Fixed bugs in the "All Rookies" and "Challenge Mode" presets to guarantee the intended cars are loaded into the game and chosen during cups.

Bug fixes:

- Fixed Practice Star progression: Generates a Practice Star for tracks that didn't have one.
- Fixed bugs in the generation process to correctly enforce the "Duplicate Track Handling" and "Rating Distribution" options.
- Custom tracks randomized into stock slots will now prioritize playing their MUSIC/MP3 soundtrack instead of defaulting to the REDBOOK.
- Fixed bug that prevented launching the game while using a profile with spaces in its name.

**v0.1.1**

Fixed bug in the preset definition for "All Rookies" preset which prevented the game from launching seeds generated with that preset.

- To play the "All Rookies" preset, install this update and generate a new "All Rookies" randomized seed.

**v0.1.0**

Initial release of RVGL Randomizer.

Randomization features:

- Modify list of stock and DC cars
- Modify rating and unlock method of cars
- Modify list of stock tracks
- Modify rating and unlock method of tracks
- Modify stages and settings of stock cups

App features:

- Scan RVGL installation, supporting both classic and launcher install
- Select content packs for cars and tracks in launcher installs
- Use configuration presets or custom configuration
- Generate and share randomized seeds
- Create in-game profile for randomized games
- Launch randomized game with selected seed, packs and launch arguments

See the [Readme](https://github.com/HansyRod/rvgl-randomizer/blob/dev/README.MD) for full details.