Dash Faction
============

About
-----
Dash Faction is a Red Faction game modification designed to fix original game bugs, improve compatibility with modern
hardware and software, extend functionality, and improve graphical quality and engine performance.

Features:
* Multiplayer level auto-downloader (uses https://autodl.factionfiles.com just like Pure Faction)
* Spectate mode (first person or free camera)
* Widescreen support
* Windowed and borderless display modes
* Autosaving support for singleplayer
* Better graphical quality (anti-aliasing, higher texture color depth, anisotropic filtering, higher scanner view and mirror resolutions, LOD model improvements and more)
* Improved security (enabled Data Execution Prevention, multiple buffer overflow fixes)
* Multiple game limits raised/removed
* Improved scoreboard with Kills/Deaths column
* Information about killer's weapon displayed in chat
* Option to disable level ambient sounds
* `ui.vpp` cheating prevention
* Multiple fixes for high FPS (currently limited to 240), especially the infamous exploding submarine bug
* Country-specific edition support (properly handles game directory structure for German and French editions)
* Improved game performance
* Voting support for dedicated servers
* Hit-sound support for dedicated servers
* Other usability and stability enhancements

See the [CHANGELOG file](docs/CHANGELOG.md) for a detailed list of all features.

You can also check out [examples of graphical improvements compared to the base game](docs/graphics_comparison).

### Client commands

Name                          | Description
----------------------------- | --------------------------------------------
`. cmd_name_fragment`         | find a console variable or command
`maxfps value`                | set maximal FPS
`hud`                         | show and hides HUD
`bighud`                      | toggle HUD between big and normal size
`spectate [player]`           | start spectate mode (first person or free camera)
`inputmode`                   | switch between default and DirectInput mouse input handling
`ms value`                    | set mouse sensitivity
`vli`                         | toggle volumetric lightining
`fullscreen`                  | enter fullscreen mode
`windowed`                    | enter windowed mode
`antialiasing`                | toggle anti-aliasting
`nearest_texture_filtering`   | toggle nearest neighbor texture filtering
`damage_screen_flash`         | toggle screen flash effect when player is getting hit
`mesh_static_lighting`        | toggle static lighting for clutters and items
`reticle_scale scale`         | set reticle scale
`findlevel rfl_name_fragment` | find level using rfl name fragment
`download_level rfl_name`     | download level from FactionFiles.com
`linear_pitch`                | toggle linear pitch angle
`skip_cutscene_bind control`  | set binding for cutscene skip action
`levelsounds`                 | set volume scale for level ambient sounds
`swap_assault_rifle_controls` | swap primary and alternate fire controls for Assault Rifle weapon
`swap_grenade_controls`       | swap primary and alternate fire controls for Grenades weapon
`show_enemy_bullets`          | toggle visibility of enemy bullets in multiplayer
`kill_messages`               | toggle printing of kill messages in the chatbox and the game console
`mute_all_players`            | toggle processing of chat messages from other players
`mute_player`                 | toggle processing of chat messages from a specific player
`fps_counter`                 | toggle FPS counter
`debug_event_msg`             | toggle tracking of event messages in console

### Server commands

Name                     | Description
------------------------ | --------------------------------------------
`unban_last`             | unban last banned player
`map_ext`                | extend round
`map_rest`               | restart current level
`map_next`               | load next level
`map_prev`               | load previous level
`kill_limit value`       | set kill limit
`time_limit value`       | set time limit
`geomod_limit value`     | set geomod limit
`capture_limit value`    | set capture limit

Compatibility
-------------
Dash Faction is compatible with Red Faction 1.20 North America (NA).
If your game version is 1.00 or 1.10 you have to update it to 1.20 first.
If your edition is not NA or you are using the Steam/GOG version, you have to replace RF.exe with one from the
1.20 NA version (it can be found on https://www.factionfiles.com). The launcher will ask you to do it if it detects
an unsupported executable. The Dash Faction installer does all required changes to the installation automatically.

Supported operating systems: Windows 7 and newer.

Dash Faction also works on Linux when launched through Wine. The latest Ubuntu LTS and vanilla Wine from the latest stable branch are recommended.

Usage
-----
1. Unpack the Dash Faction files to any folder (there is no requirement to put it in the Red Faction folder).

2. Run `DashFactionLauncher.exe`.

3. When running the launcher for the first time, select the "Options" button and adjust the settings to your preference. Make sure the
   game executable path is valid.

4. Close the options window and click the "Launch Game" button to start Dash Faction.

Advanced usage
--------------
You can provide additional command line arguments to the `DashFactionLauncher.exe` application, and they will be forwarded
to the Red Faction process. For example, to start a dedicated server use the `-dedicated` argument just like in the original
game.

Dash Faction specific arguments:

* `-game` - launch the game immediately without displaying the launcher window
* `-editor` - launch the level editor immediately without displaying the launcher window
* `-win32-console` - use a native Win32 console in the dedicated server mode
* `-exe-path` - override the launched executable file path (RF.exe or RED.exe) - useful for running multiple dedicated servers using separate RF directories

Problems
--------
If your antivirus software detects Dash Faction as a malicious program, add it to a whitelist or try to disable
reputation-based heuristics in the antivirus options. It may help as some antivirus programs flag new files as malicious just because they are not widely used.
If you do not trust the author of Dash Faction, you can review its code and compile it yourself - keep in mind it is
open-source software.

During video capture in OBS, please disable MSAA in Options - they do not perform well together.

If you experience any other problems, you can ask for help in the Faction Files Discord server at https://discord.gg/factionfiles.

Additional server configuration
-------------------------------
Dedicated server specific settings are configured in the `dedicated_server.txt` file.
Dash Faction specific configuration must be placed below the level list (`$Level` keys) and must appear in the order
provided in this description.

Configuration example:

    //
    // Dash Faction specific configuration
    //
    // Enable vote kick
    $DF Vote Kick: true
        // Vote specific options (all vote types have the same options)
        // Vote time limit in seconds (default: 60)
        +Time Limit: 60
    // Enable vote level
    $DF Vote Level: true
    // Enable vote extend
    $DF Vote Extend: true
    // Enable vote restart
    $DF Vote Restart: true
    // Enable vote next
    $DF Vote Next: true
    // Enable vote previous
    $DF Vote Previous: true
    // Duration of player invulnerability after respawn in ms (default is the same as in stock RF - 1500)
    $DF Spawn Protection Duration: 1500
    // Initial player life (health) after spawn
    $DF Spawn Health: 100
    // Initial player armor after spawn
    $DF Spawn Armor: 0
    // Time before a dropped CTF flag will return to base in ms (default is same as stock RF - 25000)
    $DF CTF Flag Return Time: 25000
    // Enable hit-sounds
    $DF Hitsounds: true
        // Sound used for hit notification
        +Sound ID: 29
        // max sound packets per second - keep it low to save bandwidth
        +Rate Limit: 10
    // Replace all "Shotgun" items with "rail gun" items when loading RFLs
    $DF Item Replacement: "Shotgun" "rail gun"
    // If enabled players are given full ammo when picking up weapon items, can be useful with the Weapons Stay standard option
    $DF Weapon Items Give Full Ammo: false
    // Replace default player weapon class
    $DF Default Player Weapon: "rail_gun"
        // Ammo given on respawn (by default 3 * clip size)
        +Initial Ammo: 1000
    // Anti-cheat level determines how many checks the player must pass to be allowed to spawn. Higher levels include
    // checks from lower levels. Default level is 0. Supported levels:
    // 0 - everyone can play
    // 1 - player must use non-custom build of Dash Faction 1.7+ or Pure Faction (unpatched clients are disallowed)
    // 2 - essential game parameters must match (blue P symbol in Pure Faction)
    // 3 - client-side mods are disallowed (gold P symbol in Pure Faction)
    //$DF Anticheat Level: 0
    // If true and server is using a mod (-mod command line argument) then client is required to use the same mod
    // Can be disabled to allow publicly available modded servers
    $DF Require Client Mod: true
    // Multiplied with damage when shooting at players. Set to 0 if you want invulnerable players e.g. in run-map server
    $DF Player Damage Modifier: 1.0
    // Enable '/save' and '/load' chat commands (works for all clients) and quick save/load controls handling (works for Dash Faction 1.5.0+ clients). Option designed with run-maps in mind.
    $DF Saving Enabled: false
    // Enable Universal Plug-and-Play (enabled by default)
    $DF UPnP Enabled: true
    // Force all players to use the same character (check pc_multi.tbl for valid names)
    $DF Force Player Character: "enviro_parker"
    // Maximal horizontal FOV that clients can use for level rendering (unlimited by default)
    //$DF Max FOV: 125
    // Allow clients to use `mesh_fullbright`
    $DF Allow Fullbright Meshes: false
    // Allow clients to use `lightmaps_only`
    $DF Allow Lightmaps Only Mode: false
    // Allow clients to use `disable_screenshake`
    $DF Allow Disable Screenshake: false
    // If enabled a private message with player statistics is sent after each round.
    //$DF Send Player Stats Message: true
    // Send a chat message to players when they join the server ($PLAYER is replaced by player name)
    //$DF Welcome Message: "Hello $PLAYER!"
    // Reward a player for a successful kill
    $DF Kill Reward:
    // Increase player health or armor if health is full (armor delta is halved)
    +Effective Health: 0
    // Increase player health
    +Health: 0
    // Increase player armor
    +Armor: 0
    // Limit health reward to 200 instead of 100
    +Health Is Super:
    // Limit armor reward to 200 instead of 100
    +Armor Is Super:


Building
--------

Information about building Dash Faction from source code can be found in [docs/BUILDING.md](docs/BUILDING.md).

License
-------
Most of the Dash Faction source code is licensed under Mozilla Public License 2.0. It is available in the GitHub repository.
See [LICENSE.txt](LICENSE.txt).

Only the Pure Faction anti-cheat support code is not open-sourced, because it would make PF anti-cheat features basically useless.
It consists of a few files in the *game_patch/purefaction* directory. It is linked statically during a release process
of Dash Faction by the owner of the project.

Dash Faction uses some open-source libraries. Their licenses can be found in the *licenses* directory.
