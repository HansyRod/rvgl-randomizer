# RVGL Randomizer

* RVGL Randomizer is a mod that allows changing the single-player campaign of Re-Volt, by customizing and randomizing the cars, tracks and cups in the game.
* It works by scanning the available content on your RVGL installation and creating a generated seed based on the chosen randomization options.
* Then launch the game through RVGL Randomizer to play a single-player campaign based on your seed file, in a new profile.

## Getting Started

### Prerequisites

- Windows 64-bit version - Confirmed to work in Windows 10 and 11.
- RVGL version 23.1030a1, Windows 64-bit. This is the only supported version. Classic and Launcher installs are both supported.
- Microsoft Visual C++ Redistributable - If not installed, download the latest x64 version from [Microsoft's website](https://learn.microsoft.com/en-gb/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version).
- WebView2 runtime - Obtained automatically during installation.

### Installation
1. Download the installer from Re-Volt World or the [Github releases page](https://github.com/HansyRod/rvgl-randomizer/releases).
2. Install to a location of your choice.
3. Run `rvgl-randomizer.exe` to launch the application.

**Note:** RVGL Randomizer creates a new profile in your RVGL installation. Outside of this profile, it doesn't affect your installation. Launching the game outside of RVGL Randomizer will continue to work with the unmodified behaviour.

## How does the randomization process work?

* RVGL Randomizer manipulates the original game content by changing which cars and tracks are present in the game. The original stock and DC content can be shuffled or replaced by custom content.
* To manage this, RVGL Randomizer uses "slots" which match the 42 cars and 14 tracks present in stock + DC content.
* Only content that is placed into those slots (either randomly or by manual configuration) will be loaded into the game. Other content is discarded.
* Then, for each car/track, the rating/difficulty and unlock methods can also be randomized or customized.
* The championship cups depend on which tracks are selected to be in the game, and will be changed to use those tracks, but can also be randomized or customized.

### Stock mode

* RVGL Randomizer accepts installations with and without custom content.
* If your installation has only the original cars and tracks for Re-Volt on PC, RVGL Randomizer will detect this and enable "Stock Mode".
* Stock Mode uses 28 cars instead of 42, and 13 tracks instead of 14.

## How to Use

This is a step-by-step guide of what you can do in RVGL Randomizer.

### Setup

In this section you can prepare your RVGL installation and available content.

1. Click the Browse button and find your RVGL installation.
2. Select the `rvgl.exe` executable.
3. In the Cars and Tracks tabs, you can see the content available on your RVGL installation.
4. If using an RVGL Launcher install, you can toggle which content packs will be used by the randomizer.

### Configure

In this section you can choose a randomization preset, or fine-tune the randomization criteria for cars, tracks and cups.

#### Presets

For a quick start, you can choose a configuration preset that will randomize the game with curated instructions.

* Click a preset to learn in more detail what it does.
* Some presets have specific content requirements. If your installation doesn't meet the requirements, the preset will be disabled and display a message explaining why.
* Once you're satisfied, continue to the Generate step with your chosen preset.

#### Custom configuration

Instead of using a preset, you can also customize exactly how you want the randomization for your game to work.

1. Select the "Custom" option at the end of the preset list.
2. Choose "Default Manual Configuration" if you want a clean setup, or "Copy Another Preset" to use an existing preset as a starting point.
3. The "Cars", "Tracks" and "Cups" sections of the Configure menu will be available.

See the [Configure Options](https://github.com/HansyRod/rvgl-randomizer/blob/dev/CONFIGURE.MD) page for detailed instructions on what each option does.

### Generate

In this section you can generate your randomized seed, based on your installed content and configured settings.

To generate a seed, you need to provide:
* **Instance Name** - a unique name for a JSON file. This file will store all chosen settings and the result of the randomization.
* **Profile Name** - this is the in-game name for your profile in the randomized seed. The name should be different from existing profiles to ensure there are no conflicts in progress table data.

You can share your randomized seed with other players. The JSON files are saved in the `{user}/AppData/Local/com.rod.rvgl-randomizer/generated` folder.

A list of previously generated or imported files is available, allowing to switch between campaigns.

### Play

In this section you can launch the game using the seed that is currently loaded by RVGL Randomizer. This is the only way of playing a game generated by RVGL Randomizer - launching the game directly or through RVGL Launcher will not load the randomizer mod.

Before launching, review the command line arguments to pass into the game. If using a Launcher installation, you can also activate extra packs that do not have car/track content, such as music packs.

Press "Launch Game" and enjoy your randomized Re-Volt experience!

## FAQ / Troubleshooting

* **I cannot generate an instance or launch the game, the buttons are disabled.**

Check if you have any errors in your configuration. In the bottom-left corner of the app, you will see an error badge. Click it to get more details on what's wrong in your case.

* **RVGL Randomizer doesn't start with this error: "Could not find the WebView2 Runtime".**

WebView2 is already provided in the process of installing RVGL Randomizer. If that fails, you can download WebView2 in [Microsoft's website](https://developer.microsoft.com/en-us/microsoft-edge/webview2#download). The Evergreen Bootstrapper automatically detects the correct WebView2 version for your system.

* **RVGL doesn't start with this error: "Launch failed: DLL injection failed: LoadLibraryA returned NULL inside RVGL — DLL failed to load."**

Confirm you have Microsoft Visual C++ Redistributable in your system. You can download the latest x64 version from [Microsoft's website](https://learn.microsoft.com/en-gb/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-supported-redistributable-version).

* **Can I use other mods with RVGL Randomizer?**

RVGL Randomizer is compatible with any custom content mods for cars and tracks. It was not tested and is not guaranteed to be compatible with other mods such as scripting mods.

* **How does RVGL Randomizer work?**

RVGL Randomizer modifies the game's running memory to change how the game loads car, track and cup content. No changes are made to the game files, so you can keep using your RVGL installation outside of the mod.

* **Will RVGL Randomizer be available for other versions of RVGL?**

There are no plans to support other versions of RVGL or the original version of Re-Volt. Only the Windows x64 23.1030a1 version of RVGL is supported.

## Uninstall

To uninstall, run the `uninstall.exe` provided in the installation folder.
* After uninstalling, you will be asked if you want to keep or remove your data. Removing your data will delete the `AppData` folder containing the generated seeds.
* For a complete cleanup, remove the profiles in your RVGL save data folder that were created by RVGL Randomizer.

## Known Issues / Limitations

* Practice Star unlocks can break with randomized track setups, as some custom tracks don't have practice stars.
* In some custom tracks which have their own soundtrack, the game is playing the default game soundtrack ("Redbook") instead.

## Credits and Special Thanks

* Theman and Sinaosal, they shared their modding knowledge without which this work wouldn't be possible.
* Xarc, he created the [HD stock carboxes](https://www.revoltworld.net/dl/256x256-stock-carboxes-remake/) the randomizer uses to present stock and DC cars.
* Jan and Bourdain, their testing and feedback was invaluable to iterate and improve the app.
* All the friends who've been supporting this project!

## Contact

You can send me a message through Discord (@hansyrod) or create an issue in [Github](https://github.com/HansyRod/rvgl-randomizer/issues).