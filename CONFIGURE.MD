# RVGL Randomizer - Configure Options

This page has a detailed description of how you can fine-tune the randomization criteria for cars, tracks and cups.

## Car Options

Cars can be modified in three ways:
1. Changing the cars present in the game - RVGL Randomizer can load custom content and randomly replace the stock and DC cars with other cars.
2. Modify car ratings - shuffles the Rookie/Amateur/etc attribute of each car. For example, Phat Slug can become a Pro car while Toyeca becomes an Amateur.
3. Modify unlock methods - shuffles the "Obtain" attribute of each car. This makes different cars be available at the start, and being rewarded when winning championships, finding practice stars, etc.

The car options control the restrictions on where/how the randomizer can apply those modifications.

### Randomization Scope

Choose if Stock Cars and/or DC Cars should be randomized. All other options are only applied to the groups of cars selected here.

### Car Randomization

Choose which car attributes should be modified during randomization. Also affects which other settings are available.
* **Full Random**, **Random Ratings**, **Random Unlock Criteria** and **Unchanged** lock/unlock modifications on the rating/obtain attributes, depending on the option chosen.
* **Base Game Distribution** creates a car distribution that follows the original game's rules: 8 Rookies, 5 each of Amateur to Pro, with 2 unlocks in each rating by Championship, 2 by Practice Stars and 1 by Single Race Wins. This restricts only the seed's end result - cars can still have attributes different from the original ones.

### Allowed Unlock Methods

Optionally allows the randomizer to give a **Cheat Only** and **Stunt Arena** obtain attribute to a car. "Cheat Only" cars can still be used by the AI.

### Rating Options

These settings control how car ratings are distributed by the randomizer, both when selecting which cars will be available in the game, and when shuffling the rating of each car.

* **Include Super Pro rating** - Controls if Super Pro cars can be available in the game, and if the Super Pro can be randomly attributed to cars. Enable to include more powerful custom cars, or disable for a more traditional experience.
* **Car Pool Rating Distributions** - How many cars from each rating will be picked to be included in the game.
* **Target Rating Distributions** - How many cars will have each rating in the game, after the rating attributes are shuffled.

Disabled ratings are unrestricted, not removed. To remove a rating from the car pool / end result, enable it and set Max to 0.

### Starting Car Configuration

Makes the first N randomized slots be available as starting cars.
* You can set a source filter (*Stock Pool*, *DC Pool*, *Custom Pool* and *Pack: ...* if using a Launcher installation) and rating filter to control which cars can be picked as starting cars.
* If randomization of stock cars is disabled, this setting is applied to the DC cars instead.

## Stock Specification / DC Specification

Specification pages allows complete control over the randomization process for each car.
* Each row represents one car, with the **Target Slot** showing the original car in that position.
* **Car Choice** columns manipulate which car is picked. You can set restrictions on the car pool, rating or obtain method of the randomized car.
* In the *Pool* column, the *Specific car* option bypasses randomization and forces that car to be present in the game in the chosen slot.
* **Attributes** columns control the Rating and Obtain method in the result. Set a specific value, or use the *Unchanged* option to keep the original value of the chosen car.

## Track Options

Analogous to car options, track options can be modified in the same three ways:

1. Changing the tracks present in the game - custom tracks can randomly replace the stock tracks.
2. Modify track ratings - shuffles the Easy/Medium/etc attribute of each track. Affects which tracks are available from the start, how they unlock and which tracks must be played to unlock cars.
3. Modify unlock methods - In the base game, all tracks are either starting tracks or unlocked by winning a championship. The randomizer can change the Obtain method of tracks so new tracks can also be unlocked by winning races, finding practice stars, etc.

### Randomization Scope

Choose if track randomization is enabled. When disabled, the randomizer assumes the 13 stock tracks + Rooftops are available.

### Track Randomization

Choose which track attributes should be modified during randomization.
1. **Full Random**, **Random Ratings**, **Random Unlock Criteria** and **Unchanged** lock/unlock modifications on the rating/obtain attributes, depending on the option chosen.
2. **Base Game Distribution** creates a track distribution that follows the original game's rules: 4 Easy tracks, 4 Medium tracks, 3 Hard tracks and 3 Extreme tracks. Each tier's tracks are unlocked after winning the previous tier's championship.

* **Allowed Unlock Methods**

Optionally allows the randomizer to give a **Stunt Arena** obtain attribute to a track.

## Track Specification

Also similar to the car specification pages, this allows complete control over the randomization process for each track.
* Each row represents one track, with the **Target Slot** showing the original track in that position.
* **Track Choice** columns manipulate which track is picked. You can set restrictions on the source pool and difficulty rating of the randomized track.
* In the *Pool* column, the *Specific track* option bypasses randomization and forces that track to be present in the game in the chosen slot. This is required if you want to use a specific track in a championship cup.
* **Attributes** columns control the Difficulty and Obtain method in the result. Set a specific value, or use the *Unchanged* and *Default* options to keep the original value of the chosen track.

## Cup Settings

Control how each championship cup is generated. If track randomization is enabled, it's necessary to review these settings, since if stock tracks are excluded from the randomization, then the original cups would no longer be playable.

This page has settings that can apply to all cups, then Bronze / Silver / Gold / Platinum Cup pages can override settings specific to that cup.

### Stage Mode

Determines how the randomizer creates the stages for each cup. The options are:
* **Default Stages** - The cup stages follow the same pattern as the base game. For example, for Bronze Cup, the randomizer would replace the tracks used in the cup (Toys in the Hood 1 - Botanical Garden, the first four stock tracks) by the first four randomized tracks on the list of tracks, and leave everything else (number of stages, number of laps, reverse/mirror variant) unchanged.
* **Random Stages** - Creates stages randomly for each cup, respecting the restrictions configured in **Random Stage Constraints**.
* **User-Defined Stages** - Available as an override option, allows defining your own list of stages.
  * To use specific tracks, they must be defined in **Track Specification** so they're guaranteed to be in the resulting seed.
  * Alternatively, if track randomization is disabled, any of the 14 stock tracks can be used.
  * You can also pick specific slots from the list of 14 track slots. This will select the track that was randomized to be in that slot for the cup stage.

### Random Stage Constraints

If the **Random Stages** option is selected, controls how the randomization process is restricted:
* **Track Variant Flags** - select if Reverse, Mirror, and Reverse Mirror versions of tracks are allowed in random cups.
* **Guarantee first appearance in Normal mode** - The first time a track is played in a random cup, it will be played in its Normal form. This gives progression a more stock-like feel as this rule is respected in the original game.
* **Track Duplicates in Cup** - Sets whether playing the same track multiple times in the same cup is allowed.
* **Minimum/Maximum Number of Stages** and **Minimum/Maximum Number of Laps** - controls how many stages are generated and how many laps can each stage have. Can be overridden in each cup.

### Global Cup Settings

Controls the common cup properties - as documented in [Custom Cup](https://re-volt.gitlab.io/rvgl-docs/cups.html) creation.
* **Number of Cars**, **Number of Tries**, **Minimum Per-Race Position**, **Minimum Overall Position**, **Points Table** can be set globally, and overridden on each cup.
* **Cars Per Class** is specific to each cup, and should be set on the cup page.

## Bronze / Silver / Gold / Platinum Cup

In the pages for each cup, you can:
* Manually define a list of stages for the cup, by activating the **User-Defined Stages** mode.
* Set the **Cars Per Class** for the cup.
* Override any global settings to have a custom value for the cup.
