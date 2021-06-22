Dash Faction
============

About
-----
Dash Faction is Red Faction game modification designed to fix original game bugs, improve compatibility with modern
hardware and software, extend functionality and improve graphics quality and engine performance.

Features:
* levels auto-downloader (uses factionfiles.com just like Pure Faction)
* spectate mode (first person or free camera)
* wide-screen support
* windowed and stretched display mode
* better graphics quality (anti-aliasing, higher textures color depth, anisotropic filtering, higher scanner view and mirrors resolution, LOD models improvements and more)
* improved security (enabled Data Execution Prevention, multiple Buffer Overflows fixes)
* removed/raised multiple game limits
* improved Scoreboard with Kills/Deaths column
* information about killer's weapon in chat
* option to disable level ambient sounds
* ui.vpp cheats prevention
* multiple fixes for high FPS (it is currently limited to 240), especially fix for the infamous sub exploding bug
* country-specific edition support (properly handles game directory structure for German and French edition)
* improved game performance
* votes support for dedicated servers
* hit-sounds support for dedicated servers
* other usability and stability fixes

See [CHANGELOG file](docs/CHANGELOG.md) for a detailed list of all features.

You can also check out [examples of graphical improvements compared to the base game](docs/graphics_comparison).

Client commands:
* `maxfps value` - sets maximal FPS
* `hud` - shows and hides HUD
* `bighud` - toggles HUD between big and normal size
* `spectate [player]` - starts spectate mode (first person or free camera)
* `inputmode` - switches between default and DirectInput mouse input handling
* `ms value` - mouse sensitivity
* `vli` - toggles volumetric lightining
* `fullscreen` - enters fullscreen mode
* `windowed` - enters windowed mode
* `antialiasing` - toggles anti-aliasting
* `nearest_texture_filtering` - toggles nearest neighbor texture filtering
* `damage_screen_flash` - toggles screen flash effect when player is getting hit
* `mesh_static_lighting` - toggles static lighting for clutters and items
* `reticle_scale scale` - sets reticle scale
* `findlevel rfl_name_fragment` - finds level using rfl name fragment
* `download_level rfl_name` - downloads level from FactionFiles.com
* `linear_pitch` - toggles linear pitch angle
* `skip_cutscene_bind control` - sets binding for cutscene skip action
* `levelsounds` - sets volume scale for level ambient sounds
* `swap_assault_rifle_controls` - swaps primary and alternate fire controls for Assault Rifle weapon
* `show_enemy_bullets` - toggles visibility of enemy bullets in multiplayer
* `kill_messages` - toggles printing of kill messages in the chatbox and the game console
* `mute_all_players` - toggles processing of chat messages from other players
* `mute_player` - toggles processing of chat messages from a specific player

Server commands:
* `unban_last` - unbans last banned player
* `map_ext` - extend round
* `map_rest` - restart current level
* `map_next` - load next level
* `map_prev` - load previous level
* `kill_limit value` - sets kill limit
* `time_limit value` - sets time limit
* `geomod_limit value` - sets geomod limit
* `capture_limit value` - sets capture limit

Compatibility
-------------
Dash Faction is compatible with Red Faction 1.20 North America (NA).
If your game version is 1.00 or 1.10 you have to update it to 1.20 first.
If your edition is not NA or you are using Steam version, you have to replace RF.exe file with one from
1.20 NA version (it can be found on FactionFiles.com). Launcher will ask you to do it if it detects
unsupported EXE file. Dash Faction official installer does all required changes to the installation automatically.

Supported operating systems: Windows 7 and newer.

Dash Faction also works on Linux when launched through Wine. Latest Ubuntu LTS and latest version of vanilla Wine from stable branch is recommended.

Usage
-----
1. Unpack Dash Faction files to some folder (there is no requirement to put it in the Red Faction folder).

2. Run `DashFactionLauncher.exe`.

3. When running launcher for the first time select "Options" button and check if everything is OK (especially make sure
   game executable path is valid).

4. Close options and click "Launch Game" button to start Dash Faction.

Advanced usage
--------------
You can provide additional command line arguments to `DashFactionLauncher.exe` application. They are always forwarded
to the Red Faction process. For example to start a dedicated server use `-dedicated` argument just like in the original
game.

Dash Faction specific arguments:

* `-game` - launch the game immediately without displaying the launcher window
* `-editor` - launch the level editor immediately without displaying the launcher window
* `-win32-console` - use a native Win32 console in the dedicated server mode
* `-exe-path` - override the launched executable file path (RF.exe or RED.exe) - useful for running multiple dedicated
  servers using separate RF directories

Problems
--------
If your anti-virus software detects Dash Faction as a malicious program add it to a whitelist or try to disable
reputation based heuristics in the anti-virus options. It can help because some anti-virus programs flags new
not popular files as danger just because they are not popular among users.
If you do not trust this Dash Faction author you can review its code and compile it yourself - keep in mind it is
open-source software.

During video capture in OBS please disable MSAA in Options - it does not perform well together.

In case of problems you can ask for help on Faction Files Discord server (click Discord Support Channel button in
launcher main window).

Additional server configuration
-------------------------------
Dedicated server specific settings are configured in `dedicated_server.txt` file.
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
    // Enable hit-sounds
    $DF Hitsounds: true
        // Sound used for hit notification
        +Sound ID: 29
        // max sound packets per second - keep it low to save bandwidth
        +Rate Limit: 10
    // Replace all "Shotgun" items with "rail gun" items when loading RFLs
    $DF Item Replacement: "Shotgun" "rail gun"
    // If enabled players are given full ammo when picking up weapon items, can be useful with Weapons Stay standard option
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
    // If enabled a private message with player statistics is sent after each round.
    //$DF Send Player Stats Message: true
    // Send a chat message to players when they join the server ($PLAYER is replaced by player name)
    //$DF Welcome Message: "Hello $PLAYER!"


License
-------
Most of Dash Faction source code is licensed under Mozilla Public License 2.0. It is available in the GitHub repository.
See [LICENSE.txt](LICENSE.txt).

Only Pure Faction anti-cheat support code is not open source because it would make PF anti-cheat features basically useless.
It consists of few files in *game_patch/purefaction* directory. It is going to be linked statically during a release process
of Dash Faction by owner of the project.

Dash Faction uses some open source libraries. Their licenses can be found in *licenses* directory.
