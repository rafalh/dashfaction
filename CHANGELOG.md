DashFaction Changelog
=====================

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
- add 'show_enemy_bullets' command for toggling enemy bullets visibility (configuration is persisted) - it was always enabled in previous versions
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
