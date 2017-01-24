Dash Faction 1.0 by rafalh
==========================

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
Close settings by clicking OK and click "Launch Game" to start modded Red Faction game.

About
-----
This is Red Faction modification inspired by Pure Faction. I made it some time ago for fun and because
Pure Faction was making problems for me sometimes.
My application works fine with updated Windows 10 in contrast to Pure Faction 3.0d.

Features:
- levels autodownloader (uses factionfiles.com just like Pure Faction)
- added information about killer's weapon in chat
- fixed HUD and FOV for wide screens
- lazyban (you can ban using only part of nickname)
- removed limit of packfiles
- gamma restored when switching window
- disabled LOD models
- ui.vpp cheats prevention
- fixed crash when making screenshots with high resolution
- anisotropic texture filtering (better quality)
- multiple fixes for high FPS
- improved security
- other stability fixes
- country-specific edition support (properly handles game directory structure)

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

Changelog
---------
Version 1.0.0 (not yet released):
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
