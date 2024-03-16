Dash Faction Changelog
======================

Version 1.8.1 (not released yet)
--------------------------------
- Fix possible crash when alt-tabbing in fullscreen mode before game is fully initialized
- Simplify installation detection in setup and launcher
- Fix writing corrupted save files when autosaving some levels
- Add German and French RF 1.10 detection to setup
- Text improvements to setup
- Don't copy over the changelog during setup
- Add `gamma` command
- Fix holes in driller cockpit on a widescreen display
- Change default 'Use' key to E
- Fix "Play (camera)" editor button for level filenames with spaces
- Fix a patch for a possible buffer overflow in "Play" editor button caused by a long filepath
- Fix cull radius for particle emitters with growing particles
- Skip textures with too long names (32+ characters) in editor to avoid buffer overflow
- Speed up shadows rendering in levels with multiple detailed movers
- Add `debug particle_emitter` subcommand (shows emitter cull radius)
- Fix crash reporter URL
- Fix buffer-overflow when importing mesh with more than 8000 faces in the editor
- Fix various issues when server switches to a new level before player finishes downloading the previous one
- Adjust letterbox effects in cutscenes and after death for wide screens
- Add support for bik files (Bink videos) in mods
- Hide player riot shield third person model in cutscenes
- Fix player third person weapon model animations after loading the game from a save file

Version 1.8.0 (released 2022-09-17)
-----------------------------------
- Add autosave after a level transition
- Disable big HUD for resolutions lower than 1024x768 to prevent crashes
- Fix "Wrong player ID" warnings when multiplayer quick save/load is used (client-side fix)
- Restructure Options window
- Add option to disable beep sound when new player joins multiplayer game and window is not focused
- Ignore browsers when calculating player count for info requests
- Fix Message event crash on dedicated server
- Prevent browsers from voting
- Add `swap_grenade_controls` command
- Don't overwrite existing levels in `download_level` command
- Add `download_level_force` command that overwrites existing levels
- Allow the person who started a vote to cancel it
- Allow setting max players server setting to 1
- Change launcher window title
- Allow autocomplete for `map` command
- Add version and date to `/info` server chat command
- Enable `map` command in RCON
- Change update checker URL
- Fix particle damage on dedicated servers
- Add `.` command to make finding commands easier
- Fix blurry fonts in the launcher on HiDPI monitors
- Add `fps_counter` command
- Make editor window resizeable
- Add `debug_event_msg` command that allows to track event messages in console
- Do not panic when character animation cannot be loaded, instead fall back to the first animation - fixes
  "Too many animations" errors occuring in specific conditions in some run maps (note that this problem was
  not occuring in the stock game because it allowed memory corruption to happen)
- Log critical error message
- Do not show glock with silencer in 3rd person view if current primary weapon is not a glock (RF bug)
- Change screenshot filename template (it now includes date, time and level filename)
- Fix multiple out-of-bounds writes in save game code (fixes crash when saving the game in some custom levels)
- Do not use TTF fonts in game menu if VF fonts are modded (should fix some unofficial game localizations)
- Change transparency sorting algorithm to fix flamethrower particles rendering in rooms with liquid and/or
  semi-transparent details
- Optimize finding a new room for moving objects
- Allow start menu shortcut to be skipped in setup program
- Simplify installation name in Windows control panel (no longer includes "version [version]")
- Add icon for control panel entry
- Restore original Red Faction Launcher on uninstall if replaced with Dash Faction Launcher link
- Add game installation path autofill for German GOG RF to setup program
- Remove unnecessary "Unknown tables.vpp version" error from setup program (the launcher already displays an error for mismatched tables.vpp files)
- Fix possible OOB write in waypoint list read code
- Add server verification in update checker using Ed25519 signature
- Fix opacity of Display_Fullscreen_Image being affected by previous drawing operations
- Restore blackout from a save file if it was killing the player or ending the game
- Fix crash when skipping a cutscene that stared when blackout was in progress
- Remove limit of restored objects during a level transition
- Fix possible buffer overflow if transitioned object mesh name is longer than 31 characters
- Change default FPS limit to 120 to comply with the stock game
- Fix loading a save game when player entity is out of level bounds
- Fix possible freeze when burning entity is destroyed

Version 1.7.0 (released 2021-06-05)
-----------------------------------
- Change maximal value of Max Kills dedicated server config to 999
- Support Team DM scores above 255
- Added additional textures from base game to level editor texture browser (contribution from Goober)
- Display proper server name in the scoreboard when joining by link
- Level auto-downloader has new UI and does its job without leaving the server
- Use SSL for communication with FactionFiles in level auto-downloader
- Add `pow2_tex` command for forcing usage of power of two textures
- Add support for redirecting dedicated server standard output stream to a file in win32-console mode
- Add server-side part of Pure Faction client verification
- Add `$DF Anticheat Level` option in dedicated server config
- Send Kills/Deaths statistics to players joining an already started gameplay
- Send player statistics (kills, deaths, accuracy) in private message after round ends in Dash Faction server
- Update fpgun skin when switching players in spectate mode
- Update spectate mode UI
- Do not render the level twice when Message Log is open
- Add `spectate_mode_minimal_ui` command for disabling most of spectate mode texts
- Add welcome message function for dedicated servers
- Fix 3D sounds ignoring master volume during first frame after creation if EAX is enabled
- Fix restarting muted ambient sounds
- Change editor hotkey for lightmap+lighting calculation to Ctrl+Shift+L because old hotkey was conflicting with
  other function
- Fix handling conflicting Activated By property when editing multiple triggers in editor
- Fix copying bool properties in some events
- Fix move sound not being muted if entity is created hidden
- Fix console being rendered in wrong place on endgame screen
- Fix memory corruption when transitioning to 5th level in a sequence and the level has no entry in ponr.tbl
- Fix music being unstoppable by events after entering the game menu
- Calculate mesh static lighting when object mesh is switched to corpse
- Make sure GR/FR game localizations have priority over EN files in case someone messed his RF directory
- Fix persona messages being displayed too low in Big HUD mode in vehicles
- Fix rfl file extension association with Dash Editor not working properly for filenames containing space characters
- Add `mute_all_players` and `mute_player` commands for hiding chat messages from specific players
- Add server config option to give max ammo to the player when a weapon item is picked up
- Append dedicated server config name to the log file name so logs from multiple servers won't mix
- Change log entry time format
- Fix clients randomly losing connection to the server (RF bug)
- Fix 2D sound regression when EAX is enabled
- Remove limit of loaded skeletons (*.rfa) - fixes crash when loading L1S1 and then pdm-deathmatch-run-v2
- Remove memory corruption when too many skeletons/animations are added to a single character
- Optimize skeleton unlinking when character is destroyed
- Fix rendering menu background if its height is lower than 480 (for example AQuest Speedrun Edition mod)

Version 1.6.1 (released 2021-02-20)
-----------------------------------
- Improve tbl parsing error message
- Bring back team icons in team scores HUD (left-bottom screen corner) in Team DM multiplayer game (1.5.0 regression)
- Fixed rarely occuring client-side crash in multiplayer (regression from 1.6.1-beta)
- Fix possible game freeze when scrolling the message log on a wide screen
- Change launcher header image to one provided by Goober
- Add `$DF UPnP Enabled` option in dedicated server config that allows to disable UPnP
- Fix hit sound packet sometimes being sent from the client
- Do not play hit sounds if player damages themselves
- Fix possible network issues if multiple clients sharing one IP address connect to a server in a very short period of
  time (RF bug)
- Fix drawing main menu background on some graphics cards/drivers (1.6.1-rc1 regression)
- Change the key that crashes a frozen game process to Alt and change the minimal hold time to 1 second
- Add `$DF Max FOV` option in dedicated server config that allows to limit FOV used by clients for level rendering
- Add more textures from base game to the level editor texture browser (contribution from Goober)

Version 1.6.1-rc1 (released 2021-02-13)
---------------------------------------
- Fix background image being scrolled too fast in multiplayer menu
- Make sure CTF flag does not spin after level change if it was in dropped state in the previous level
- Make main menu background image scroll animation smooth on high resolution displays
- Add `fpgun_fov_scale` command that allows adjustment of fpgun (view model) FOV
- Remove UID limit (50k) in editor
- Fix shape and script name being overwritten if editing properties of multiple triggers that have those fields set
  differently
- Do not verify mod vpp checksum if `Require Client Mod` option is disabled in server config
- Fix near plane clipping issues happening when FOV is big
- Increase max FOV to 160
- Fix clicking "Fav" checkbox in the server list

Version 1.6.1-beta (released 2021-01-29)
----------------------------------------
- Fix items not respawning in DF-hosted server (1.6.0 regression)
- Fix ambient sound volume not being affected by `levelsounds` command if EAX sounds are disabled (1.6.0 regression)
- Add `screenshot` command
- Add `d_profiler` command for Release builds (single player only)
- Fix vote not ending when level ends in hosted server (1.6.0 regression)
- Fix CTF flag sometimes not returning to the base automatically (RF bug)
- Fix spectate camera staying in fixed mode after level change
- Fix editor crash when building geometry after lightmap resolution for a face was set to Undefined (RF bug)
- Improve crash reporting for Dash Editor and launcher
- Do not print redundant new lines in `-win32-console` server mode and optimize it a little
- Fix unwanted server-side weapon fire when switching from a continous weapon or a weapon with burst fire
- Prevent shooting and reloading while switching weapon server-side
- Increase decals limits
- Add server-side CPS limit
- Separate maxfps cvar for dedicated server and client
- Clear console input field when Control+C is pressed
- Do not add repeated commands to the console history
- Rename option "Disable LOD models" to "High model details" - it now scales LOD distances by 10, scale can be updated
  using `lod_distance_scale` command
- Add `fov` command which allows to override horizontal FOV value
- Fix crash that is occuring when cutscene path node is copied in the level editor (RF bug)
- Fix out of ammo sounds being played when ammo got out of sync client-side
- Add missing `consolebutton2A.tga` texture which is used by "Console Button02" clutter object
- Fix chat input box rendering in big HUD mode when a very long message is being entered
- Improve scope weapons zoom in spectate mode

Version 1.6.0 (released 2021-01-07)
-----------------------------------
- Fix sounds being often skipped if volume in game options is set very low (1.6.0-rc1 regression)
- Fix rockets not making damage after hitting a detail brush (fix included in 1.6.0-beta did not always work)
- Add `frametime_graph` command
- Small code improvements

Version 1.6.0-rc1 (released 2020-12-31)
---------------------------------------
- Fix rocket launcher IR scanner in spectate mode (regression)
- Fix 3D sounds having wrong volume for a very short time (regression)
- Disable unsupported color depths in launcher options
- Open RF wiki instead of hlp file when F1 is pressed in editor
- Add more tools in editor menu
- Change editor hotkey for create packfile function to Ctrl+Shift+S
- Do not render CTF flag being hold by a spectated player
- Add more checks for buffer overflow errors when parsing tbl files
- Add a warning message when starting game with a mod that has a space in the name
- Add missing textures: `mtl_archway04-mip1.tga`, `pls_airlockmat01-mip2.tga`
- Add more textures from base game to the level editor texture browser
- Fix buffer overflow occuring when using Play Level command in level editor if level path and/or Dash Faction path is
  long
- Improve performance of sound function `ds_get_channel`
- Fix `level_sounds` command not affecting ambient sounds (1.6.0-beta regression)
- Refactor sounds allocation to avoid frequent allocation and freeing of DirectSound buffers when channels pool is
  full - should improve performance
- Add `kill_messages` command that allows to disable printing information about kills in the chatbox and the game console
- Fix hud msg never disappearing in spectate mode
- Fix Brush Cliping Tool window causing speed reduction for level editor
- Other stability fixes

Version 1.6.0-beta (released 2020-12-12)
----------------------------------------
- fix alpha draw order issues in envirosuit1.vcm, Vet1.v3m, coffeesmokedtbl2.v3m, coffeesmokedtblAlt.v3m meshes
- add support for bitmaps with alpha channel in Display_Fullscreen_Image event
- fix level editor not using all space for rendering when used with a big monitor
- fix wrong aspect ratio in level editor when used with a wide screen
- use Hor+ FOV scaling method in level editor perspective view
- make level editor perspective view FOV depend on view aspect ratio
- add Save, Pack, Lighting, Play, Play (camera) buttons in top bar of main editor window
- remove "Packfile saved successfully!" message after packfile generation in the level editor
- do not reduce editor FPS when editor dialog box is open
- fix changing properties of multiple respawn points in level editor
- add multiplayer settings in trigger properties window in level editor
- load V3M meshes from `user_maps\meshes` and `red\meshes` in the level editor
- fix lights randomly stopping working after copying in the level editor
- fix rendering of VBM textures from `user_maps\textures` in the level editor
- fix level editor crash when selecting decal texture from 'All' folder
- no longer require "-sound" argument when launching level editor with sound support
- allow opening a level in the level editor by using `-level` command line argument
- add option to associate `rfl` file extension with Dash Faction level editor in the setup program
- fix level editor crash randomly occuring when opening cutscene properties
- add icon for EAX Effect objects in the level editor (icon provided by Goober)
- add more textures from base game to the level editor texture browser
- fix packing a level that contains a particle emitter with a default texture in the level editor
- fix path node connections sometimes being rendered incorrectly in the level editor
- use dynamic allocation when pool is full for weapons, debris, corpses, decal polygons, faces, face vertices,
  bounding boxes, vertices, meshes (it was listed as a feature of Dash Faction 1.5.0 but the patch was actually
  broken)
- support starting the launcher from a symlink
- fix objects having invalid radius after Switch_Model event
- allow spectating in servers with Force Respawn enabled before player spawns for the first time
- fix crash when using Switch_Model event on object with collisions disabled
- fix replacing collision spheres when using Switch_Model event
- recalculate mesh static lighting after Switch_Model event
- optimize logging
- add an option to override RF launcher by a symbolic link in the setup program
- fix rockets not making damage after hitting a detail brush
- allow "debug network" command usage in multiplayer
- fix rotation interpolation (Y axis) in multiplayer when it goes from 360 to 0 degrees
- add links to Red Faction Wiki in RED Help menu replacing old "Help Topics" menu item that did not work on modern
  Windows versions because of the lack of support for WinHelp format
- fix F4 key (Maximize active viewport) in RED for screens larger than 1024x768
- fix shooting through transparent sections of DDS textures
- change default server update rate to 20
- add 'update_rate' command server-side and client-side
- fix object interpolation in multi-player being played too fast causing a possible jitter
- replace editor icon
- add P hotkey for packfile generation in editor
- add Ctrl+L hotkey for lightmaps and lighting calculation in editor
- add more items in editor main menu
- add option in setup program to enable Windows Visual Styles for the level editor
- performance optimizations
- fix memory corruption when saving game after 4 level transitions and current level does not have an entry in ponr.tbl
  (fixes crash in Bearskinner SP campaign)
- use DirectSound3D position and velocity parameters for 3D sounds instead of emulating 3D by panning (enables doppler effects and allows proper 3D experience in setups with more than 2 sound channels e.g. 5.1)
- improve compatibility with dsoal (allows EAX effects emulation)
- fix entity staying in crouched state in a liquid if it was crouching when entering the liquid
- use Direct3D 9 rendering API
- optimize dynamic textures for Direct3D 9
- send operating system version when checking for updates in the launcher

Version 1.5.0 (released 2020-09-05)
-----------------------------------
- increase object limit from 1024 to 65536
- remove additional limits on number of allocated bullet, item, debris and corpse objects
- enable and fix items and clutters static lighting calculation
- enable Set_Liquid_Depth implementation - check out Geothermal Plant in single player and see what changed
- add DDS texture format support (useful for DXTn compressed textures)
- rewrite crash reporting system UI
- improve sound handling when active sounds pool is full
- allow custom meshes in Switch_Model event
- add 'rf' protocol handler (URLs has format: rf://IP:PORT)
- add setup program which installs Dash Faction and patch Red Faction if needed
- add '/save' and '/load' chat commands and quick save/load controls handling for saving and restoring player position
  in multiplayer (useful for runmap servers, can be enabled in dedicated_server.txt)
- add 'Big HUD' option and 'bighud' command for making the HUD larger
- add textures for big HUD contributed by Goober
- add 'reticle_scale' command for changing reticle size
- make texts in main menu and in HUD sharp by using TrueType fonts
- log error when RFA cannot be loaded
- improve render to texture performance (e.g. mirrors and scanners)
- improve 2D drawing performance
- improve level loading time
- fix performance degradation occurring if no level with fog effect was played in current game session, for some levels
  it can double the FPS (RF bug)
- add 'damage_screen_flash' command for toggling damage screen flash effect
- render held corpse in monitors/mirrors
- add better support for non-US keyboard layouts
- add support for CTRL+V (paste from clipboard) in dialog boxes
- use SHA1 for files verification in launcher
- add support for RF.exe in version 1.20 NA with 4GB flag enabled
- stretch image header in launcher so it is properly displayed when system DPI is changed
- add option for switching game language (English, German or French)
- improve hiding HUD by 'hud' command
- add server-size validation of ammo when handling weapon fire
- use 24-bit depth buffer if supported even if video card is not Nvidia (fixes Z-fighting on Intel and AMD cards)
- use dynamic light detail texture for all cards if multi-texturing is available (improves graphics quality on Intel
  cards)
- not treat every chat message starting with 'server' as a server command
- add GOG install directory auto-detection
- add server-side support for late joiners aware triggers (improves Pure Faction compatibility)
- load .tbl files in user_maps (default files cannot be overridden) - allows inclusion of HUD messages in custom maps
  (*_text.tbl)
- increase maximal number of simultaneously playing sounds to 64
- use port 7755 for server hosting when 'Force port' option is disabled
- add UPnP automatic port forwarding when hosting a server
- add 'download_level' command which invokes level downloader
- improve algorithm for selecting texture D3D format (should reduce VRAM usage)
- add 'Rate' setting in Options window (allows specifying network connection speed)
- add 'Adapter' setting in Options window (allows selection of graphics card)
- insert server name in window title when hosting dedicated server
- display joining player IP address in dedicated server console
- add dedicated server option for forcing player character
- add '-exe-path' launcher command line argument that allows running multiple dedicated servers using separate RF
  directories
- add support for nearest neighbor texture filtering ('nearest_texture_filtering' command)
- try setting gamma using Direct3D in full-screen mode before falling back to WinAPI (can fix gamma issues)
- fix sending crash reports
- fix non-working face scrolling before first geomod use in custom levels (RF bug)
- fix memory and VRAM leak when quick saving (RF bug)
- fix VRAM leak when playing Bink video (RF bug)
- fix AI flee animation regression
- fix crash caused by AI state not being properly updated during level load for entites that are taken from the
  previous level (RF bug)
- fix crash caused by corpse pose pointing to not loaded entity action animation for corpses that are taken from the
  previous level (RF bug)
- fix buffer overflow on level load when too many objects are being preserved from the previous level; fixes crash in
  Spaceport Redux campaign (RF bug)
- fix crash when executing camera2 command in main menu (RF bug)
- fix crash when item cannot be created during RFL load in multi-player (RF bug)
- fix entity collisions with objects near absolute zero position (RF bug)
- fix bullets collisions with big objects, especially with high FPS (RF bug)
- fix Message Log window rendering if resolution ratio is not 4:3 (RF bug)
- fix flamethrower graphical issues when playing with high FPS (RF bug)
- reduce maximal particle emitter spawn rate to 60 particles per frame in order to improve graphics and performance
  (framerate dependent particle emitters like flamethrower weren't designed with 240 FPS in mind so they look bad with
  too many overlapping particles)
- fix rendering of liquids and details with alpha in Railgun scanner (RF bug)
- fix entity/fpgun ambient color being improperly calculated when 'True color textures' option is active
- fix invalid lightmap sometimes being used for rendering of details with alpha; fixes glass color being affected by
  shooting in glass_house.rfl (RF bug)
- fix items not being respawned after ~25 days of server uptime (RF bug)
- fix players' pings being improperly calculated after ~25 days of server uptime (RF bug)
- fix compatibility of Crash Handler module with Windows XP and Vista
- fix infinite loop when failed to load Bink video (RF bug)
- fix memory leak when trying to load non-existing Bink video (RF bug)
- fix crash caused by explosion near dying player-controlled entity (RF bug)
- fix unwanted fire after weapon switch from weapon with impact delay (RF bug)
- fix wall-peeking (RF bug)
- fix weapon being auto-switched to previous one after respawn even when auto-switch is disabled (RF bug)
- fix swap_assault_rifle_controls cvar issues with stopping the fire and shooting from vehicles
- fix team scores in TDM
- fix auto-detection of Steam based Red Faction installation directory
- fix crash if camera cannot be restored to first-person mode after cutscene (RF bug)
- fix handling of alpha-only (A8) texture format when 32-bit textures are used
- fix switch weapon UI resetting fire wait timer without checking old value (RF bug)
- fix crash when particle emitter allocation fails during entity ignition (RF bug)
- fix possible buffer overflow in console input buffer (RF bug)
- fix possible server side crash when kicking a player using rcon (RF bug)
- fix alpha draw order issues in Hendrix, Generator_Small01, Generator_Small02, LavaTester01, Weapon_RiotShield meshes
  (RF bug)
- fix corona lights being visible through clutters, items, corpses (RF bug)
- fix possible 'buzzing' sound in multi-player game if some players are in water (RF bug)
- fix dedicated server crash when loading level that uses directional light (RF bug)
- fix heap corruption when loading a bitmap with corrupted header (RF bug)
- fix stack corruption when loading a corrupted packfile (RF bug)

Version 1.4.1 (released 2019-12-07)
-----------------------------------
- add 'skip_cutscene_bind' command allowing to change the control used for skipping cutscenes (by default Multiplayer
  Stats - TAB)
- fix crash after exiting administration with Gryphon (regression)
- fix crash when joining Pure Faction servers (regression)

Version 1.4.0 (released 2019-12-04)
-----------------------------------
- add free camera spectate mode support
- add vote system (server-side)
- rework fix for sticking to the ground when jumping - now it is based on PF code (by Trotskie) - should fix free-fall
  animation bug in multi-player and occasionally sticking to the ground in some levels
- add 'map_ext' command for extending round time (server-side)
- increase precision of output from 'ms' command
- add 'show_invisible_faces' subcommand in 'debug' command
- make sure log file is not spammed by errors happening every frame
- allow cursor movement outside of game window when in menu
- slightly optimize screenshot capturing
- add optional hit-sounds in dedicated server
- add 'map_rest', 'map_next', 'map_prev' commands
- add optional Win32 native console support in dedicated server mode
- remove stripping of '%' character in chat messages
- make sure required registry values are always created on game launch
- improve FPS limit accuracy
- add item replacements support in dedicated_server.txt
- add default player weapon class and ammo override support in dedicated_server.txt
- add 'pctf' level name prefix support (server-side)
- reset fpgun animation when player dies
- sync kills/deaths statistics when joining to Pure Faction servers
- reduce launcher size and allow building it on Linux after replacing MFC by Win32++ library
- skip processing of non-damaging particles on a dedicated server (optimization)
- improve particles simulation performance
- delete projectiles when they reach bounding box of the level (should fix playing space levels with huge amount of
  people)
- stop distant sounds when sound pool is full and a new sound is being started (should fix sounds sometimes stoping
  working when playing multiplayer with a lot of players)
- sort items and clutter by mesh to improve performance
- change cutscene skip control to Multiplayer Stats to avoid unintended skipping
- add control name in cutscene skip hint message
- increase entity simulation max distance (fixes far entities not being animated even when moving)
- add support for players_request PF packet (used by online server browser)
- fix VRAM leak on level load caused by font textures (RF bug)
- fix occasionally getting stuck on jump pads on high FPS
- fix 'debug' command not flushing geometry cache when disabling flags
- fix dedicated server crash when loading level with screens
- fix submarine exploding bug if L5S3 level was loaded using 'level' command
- fix killed glass restoration from a save file
- fix very long startup on Windows XP in some hardware configurations
- fix crash when saving/loading custom levels with a lot of objects
- fix debris object pool (used for ejected shells) easly getting exhausted when local player is dead
- fix loading images with color palette in 32-bit mode
- fix default player weapon being incorrect in listen server after playing on a modded server
- fix crash during texture load if it allocation in RAM fails
- fix RAM leak if texture creation by D3D fails
- fix cutscene cinema bars when MSAA is enabled (regression)
- fix improper alpha sorting for Invulnerability Power-up items (RF bug)
- fix flee animation on steep ground for AI controlled characters on high FPS

Version 1.3.0 (released 2019-05-07)
-----------------------------------
- enable mip-mapping for textures bigger than 256x256
- change maximal FPS limit to 240
- speed up server list refreshing
- put not responding servers at the bottom of server list
- add linear pitch option (stops mouse movement from slowing down when looking up and down)
- fix bullet collision testing if mover is in the line of the shot
- add mod selector in launcher
- add "Keep launcher open" option
- increase resolution of monitors and mirrors
- change screen-shake emulated FPS to 150 to match PF default FPS
- fix invalid player entity model when undercover (RF bug)
- fix decal fade out effect (RF bug)
- open server list menu instead of main menu when leaving multiplayer game (affects level download too)
- optimize Flamethower rendering
- optimize particles and HUD rendering by reducing number of draw-calls (both in game and level editor)
- optimize rendering of railgun and rocket launcher scanners by using different back buffer capture technique
- optimize glare rendering
- use hardware vertex processing in game and level editor (optimization)
- reduce number of draw-calls when rendering lines in level editor (optimization)
- properly handle Team property in Spawnpoint object in TeamDM game (server-side) - improves PF compatibility
- client-side/solo/teleport triggers handling (server-side) - improves PF compatibility
- handle trigger team property (server-side) - improves PF compatibility
- fix 'Orion bug' - default 'miner1' entity spawning periodically on clients (server-side)
- fix beeping every frame if chat input buffer is full
- apply proper chat input limits client-side so PF server does not kick the sender
- preserve password case when processing rcon_request command (server-side)
- fix possible crash when loading a corrupted save file
- block creation of corrupted save files when cutscene starts
- allow skipping cutscenes by pressing jump key
- fix time synchronization on high FPS in cutscenes
- fix glares being visible through characters in some cutscenes
- add 'findlevel' command as alias for 'findmap'
- add 'map' command as alias for 'level'
- allow 'level' command outside of multiplayer game and remove now redundant 'levelsp' command
- add 'show_enemy_bullets' command for toggling enemy bullets visibility (configuration is persisted) - it was always
  enabled in previous versions
- add persisting of volumetric lights (glares) configuration changed by 'vli' command
- add 'fullscreen' and 'windowed' commands
- fix "Buffer overrun" message being displed in RED (level editor) when too many objects were selected
- optimize displaying selected objects in log view in RED (level editor)
- add 'debug' command for enabling RF built-in debugging features
- add 'playercount' command
- fix multiple security vulnerabilities
- stability improvements
- a lot of code was rewritten into modern C++

Version 1.2.1 (released 2017-08-15)
-----------------------------------

- added high resolution scopes
- fixed camera shake dependence on FPS (e.g. for Assault Rifle)
- fixed Level Sounds option not working in most levels
- fixed Stretched Window Mode not using full screen if choosen framebuffer resolution was lower than screen
  resolution
- fixed Bink videos not working when True Color textures are enabled
- added Drag&Drop support in Level Editor for opening RFL files
- added auto-completition with TAB key for console commands: level, kick, ban, rcon
- improved crash reporting functionality
- adjust game configuration (default weapon, clip size, max ammo) based on packets from server (improves gameplay
  on highly modified servers like Afaction)
- fixed game crash when level geometry is highly modified by GeoMod and has too many vertices
- speed up game startup
- other stability improvements

Version 1.2.0 (released 2017-05-10)
-----------------------------------

- added running and jumping animations in Spectate Mode
- added Rocket Launcher scanner rendering in Spectate Mode
- fixed Spectate Mode target entity being rendered (visible when player looks down)
- fixed keyboard layout for QWERTZ and AZERTY keyboards (automatic detection)
- added option to disable level ambient sounds (in launcher or by levelsounds command)
- added Game Type name to Scoreboard
- added fast show/hide animations to Scoreboard (configurable in launcher)
- added 'findmap' command
- fixed improper rendering after alt-tab in full screen (especially visible as glass rendered only from one side)
- fixed cursor being not visible in menu when changing level
- hidden client IP addresses in server packets
- fixed compatibility with old CPUs without SSE support
- removed error dialog box when launcher cannot check for updates
- changed chatbox color alpha component
- added small rendering optimizations
- other minor fixes

Version 1.1.0 (released 2017-02-27)
-----------------------------------

- improved loading of 32 bit textures (improves quality of lightmaps and shadows)
- launch DashFaction from level editor (editor must be started from DF launcher)
- improved Scanner resolution (Rail Gun, Rocket Launcher, Fusion Launcher)
- added alive/dead indicators in Scoreboard
- added truncating text in Scoreboard if it does not fit
- added Kills/Deaths counters in Scoreboard
- fixed console not being visible when waiting for level change in Multiplayer (RF bug/feature)
- changed font size for "Time Left" timer in Multi Player
- hackfixed submarine exploding in Single Player level L5S3 on high FPS (RF bug)
- fixed bouncing on low FPS
- added support to setting custom screen resolution in Launcher Options (useful for Windowed mode)
- changed default tracker to rfgt.factionfiles.com
- stability fixes

Version 1.0.1 (released 2017-02-07)
-----------------------------------

- fixed monitors/mirrors in MSAA mode
- fixed aspect ratio in windowed mode if window aspect ratio is different than screen aspect ratio
- fixed left/top screen edges not rendering properly in MSAA mode
- fixed right/bottom screen edges being always black due to bug in RF code
- fixed water waves animation on high FPS (with help from Saber)
- fixed invalid frame time calculation visible only on very high FPS
- player name, character and weapons preferences are no longer overwritten when loading game from save
- fixed exiting Bink videos by Escape
- other stability fixes
- launcher icon has higher resolution

Version 1.0.0 (released 2017-01-24)
-----------------------------------

- Spectate Mode
- launcher GUI
- DirectInput support
- windowed and stretched mode support
- support for country specific game edition (German and French)
- added level author and filename information on scoreboard
- fixed jump, hit-screen and toggle console animations on high FPS
- added checking for new Dash Faction version in launcher
- fixed multiple Buffer Overflows in RF code (security)
- added "ms" command for changing mouse sensitivity
- added "vli" command for disabling volumetric lightining
- changed default sorting on Server List to player count

Version 0.61
------------

- added widescreen fixes
- enabled anisotropic filtering
- enabled DEP to improve security
- implemented passing of launcher command-line arguments to game process

Version 0.60
------------

- use exclusive full screen mode (it was using windowed-stretched mode before)
- check RF.exe checksum before starting (detects if RF.exe has compatible version)
- added experimental Multisample Anti-aliasing (to enable start launcher from console with '-msaa' argument)
- improved crashlog
- added icon

Version 0.52
------------

- use working directory as fallback for finding RF.exe

Version 0.51
------------

- first public release
