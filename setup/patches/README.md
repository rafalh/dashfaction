Building
--------
To build all patches needed by setup program create following directory structure:

    official/
        rf120_na.exe
        rf120_gr.exe
        rf120_fr.exe
    source/
        RF-1.20-fr.exe
        RF-1.20-gr.exe
        RF-1.21-steam.exe
        RF-1.21.exe
        tables-gog-gr.vpp
    target/
        RF-1.20-na.exe
        tables.vpp

Files in `official` directory are official Red Faction 1.20 patches.

Files in `source` directory are source files for minibsdiff tool (files that needs to be patched).

Files in `target` directory are target files for minibsdiff tool (files from compatible version of the game).

After preparing all files build needed patches by running:

    make

Patches will be created in `output` directory.
