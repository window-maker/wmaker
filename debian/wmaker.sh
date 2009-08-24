#!/bin/sh

set -e

[ -n "$GNUSTEP_USER_ROOT" ] || export GNUSTEP_USER_ROOT="$HOME/GNUstep"
gs_base="$GNUSTEP_USER_ROOT"
gs_defaults="$gs_base/Defaults"
gs_system_defaults=/etc/GNUstep/Defaults
wm_base="$gs_base/Library/WindowMaker"
wm_backgrounds="$wm_base/Backgrounds"
wm_iconsets="$wm_base/IconSets"
wm_soundsets="$wm_base/SoundSets"
wm_pixmaps="$wm_base/Pixmaps"
gs_icons="$gs_base/Library/Icons"
wm_style="$wm_base/Style"
wm_styles="$wm_base/Styles"
wm_sounds="$wm_base/Sounds"
wm_themes="$wm_base/Themes"
WindowMaker=/usr/lib/WindowMaker/WindowMaker
convertfonts=/usr/lib/WindowMaker/convertfonts

make_dir_if_needed ()
{
    if [ ! -d "$1" ] ; then
        install -m 0755 -d "$1"
    fi
}

rename_dir_if_possible ()
{
    if [ ! -d "$2" ] ; then
       if [ -d "$1" ] ; then
          mv "$1" "$2"
       fi
    fi
}

copy_defaults_if_needed ()
{
    file="$gs_defaults/$1"
    system_file="$gs_system_defaults/$1"
    if [ ! -f "$file" ] ; then
        install -m 0644 "$system_file" "$file"
    fi
}

make_dir_if_needed     "$gs_defaults"
make_dir_if_needed     "$wm_base"
make_dir_if_needed     "$wm_backgrounds"
make_dir_if_needed     "$wm_iconsets"
make_dir_if_needed     "$wm_soundsets"
make_dir_if_needed     "$wm_pixmaps"
make_dir_if_needed     "$gs_icons"
make_dir_if_needed     "$wm_sounds"
rename_dir_if_possible "$wm_style"        "$wm_styles"
make_dir_if_needed     "$wm_styles"
make_dir_if_needed     "$wm_themes"

copy_defaults_if_needed WindowMaker
copy_defaults_if_needed WMRootMenu
copy_defaults_if_needed WMState
#copy_defaults_if_needed WMWindowAttributes

if [ -x $convertfonts -a ! -e "$wm_base/.fonts_converted" ] ; then
# --keep-xlfd is used in order to preserve the original information
    $convertfonts --keep-xlfd "$gs_defaults/WindowMaker"
    if [ -f "$gs_defaults/WMGLOBAL" ] ; then
        $convertfonts --keep-xlfd "$gs_defaults/WMGLOBAL"
    fi
    find "$wm_styles" -type f -print0 -mindepth 1 -maxdepth 1 |
    xargs -0 -r -n 1 $convertfonts --keep-xlfd
    touch "$wm_base/.fonts_converted"
fi

if [ -n "$1" -a -x "$WindowMaker$1" ] ; then
    WindowMaker="$WindowMaker$1"
    shift
fi

exec "$WindowMaker" "$@"
