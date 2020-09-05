#!/bin/sh
inno_setup_dir="$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 5"
wine "$inno_setup_dir/ISCC.exe" setup.iss
