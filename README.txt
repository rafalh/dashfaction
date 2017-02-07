Dash Faction 1.0 by rafalh
==========================

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
- better graphics quality (anti-aliasing, anisotropic filtering, disabled LOD models)
- improved security (enabled Data Execution Prevention, multiple Buffer Overflows fixes)
- removed limit of packfiles
- information about killer's weapon in chat
- LazyBan (baning and kicking players using only part of nickname)
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

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
USE IT AT YOUR OWN RISK.

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
Install Visual C++ Redistributable for Visual Studio 2015 (x86) from:
https://www.microsoft.com/pl-pl/download/details.aspx?id=48145 (select vc_redist.x86.exe).
Unpack Dash Faction files to any folder (there is no requirement to put it in Red Faction folder).
Run DashFactionLauncher.exe.
On first run select Options and check if everything is OK (especially make sure game executable path is valid).
Close settings and click "Launch Game" to start modded Red Faction game.

Changelog
---------
Version 1.0.1 (released 07-02-2017):
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

Version 1.0.0 (released 24-01-2017):
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

Version 0.61:
- added widescreen fixes
- enabled anisotropic filtering
- enabled DEP to improve security
- implemented passing of launcher command-line arguments to game process

Version 0.60:
- use exclusive full screen mode (it was using windowed-stretched mode before)
- check RF.exe checksum before starting (detects if RF.exe has compatible version)
- added experimental Multisample Anti-aliasing (to enable start launcher from console with '-msaa' argument)
- improved crashlog
- added icon

Version 0.52:
- use working directory as fallback for finding RF.exe

Version 0.51:
- first public release
