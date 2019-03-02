Dash Faction
============

About
-----
Dash Faction is Red Faction modification designed to fix multiple game bugs, improve compatibility with modern
hardware and software, extend functionality and improve graphics quality.
Works fine with Windows 10 after the Anniversary Update in contrast to other highly popular modification
named Pure Faction (version 3.0d).

Features:
- Levels Auto-Downloader (uses factionfiles.com just like Pure Faction)
- Spectate Mode
- wide-screen fixes
- windowed and stretched display mode
- better graphics quality (anti-aliasing, true-color textures, anisotropic filtering, higher scanner view resolution, disabled LOD models)
- improved security (enabled Data Execution Prevention, multiple Buffer Overflows fixes)
- removed limit of packfiles
- improved Scoreboard with Kills/Deaths column
- information about killer's weapon in chat
- option to disable level ambient sounds
- ui.vpp cheats prevention
- multiple fixes for high FPS (limited to 150 for now)
- country-specific edition support (properly handles game directory structure for German and French edition)
- other stability fixes

New commands:
- maxfps <limit> - sets maximal FPS
- hud - shows and hides HUD
- unban_last - unbans last banned player
- spectate <player> - starts Spectate Mode
- inputmode - switched between default and DirectInput mouse handling
- ms <value> - mouse sensitivity
- vli - toggles volumetric lightining

Compatibility
-------------
Dash Faction is compatible with Red Faction 1.20 North America (NA).
If your game version is 1.00 or 1.10 you have to update it to 1.20 first.
If your edition is not NA or you are using Steam version, you have to replace RF.exe file with one from
1.20 NA version (it can be found on FactionFiles.com). Launcher will ask you to do it if it detects
unsupported EXE file.

Application should work on Windows XP SP3 and newer (tested mostly on Windows 10).

Usage
-----
1. Install Visual C++ Redistributable for Visual Studio 2017 (x86) from:
https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads (select *vc_redist.x86.exe* in *Visual Studio 2017* section).

2. Unpack Dash Faction files to any folder (there is no requirement to put it in Red Faction folder).

3. Run DashFactionLauncher.exe.

4. On first run select Options and check if everything is OK (especially make sure game executable path is valid).

5. Close settings and click "Launch Game" to start modded Red Faction game.

Problems
--------
If antivirus detects Dash Faction as a malicious program try to disable Reputation Services. It can help
because some antiviruses flags new not popular files as danger just because they are not popular among users
and Dash Faction is certainly not very popular :)
I had some problems with Avast silently removing launcher exe and had to add Dash Faction to exclusions of
Behaviour Shield.

If game crashes try to disable features in Options window - especially try disabling MSAA.

License
-------
Most of Dash Faction source code is licensed under Mozilla Public License 2.0. It is available in the GitHub repository. See LICENSE.txt.

Only Pure Faction support code is not open sourced because it would make PF anti-cheat features basically useless.
It consists of *pf.cpp* and *pf.h* files in main module. It is going to be linked statically during a release process of Dash Faction by owner of this project.

Dash Faction uses some open source libraries. Their licenses can be found in *licenses* directory.
