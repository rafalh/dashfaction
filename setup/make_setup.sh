#!/bin/sh
inno_setup_dir="$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6"
wine "$inno_setup_dir/ISCC.exe" setup.iss
