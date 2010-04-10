/* Copyright (C) 2010 Carlos R. Mafra */

/*
 * If the program should run from inside a terminal it has
 * to end with a space followed by '!', e.g.  "mutt !"
 */

char *Terminals[MAX_NR_APPS][2] = {
	{ "xterm", "xterm -bg black -fg white +sb +sm -fn 10x20 -sl 4000 -cr yellow" },
	{ "mrxvt", "mrxvt -rv -shade 00 -vb +sb +sm -tr -sl 2000 -trt -itabbg black -hb -tabfg yellow -fn 10x20 -cr yellow" },
	{ "Konsole", "konsole" },
	{ NULL, NULL }
};

char *File_managers[MAX_NR_APPS][2] = {
	{ "Dolphin", "dolphin" },
	{ "Thunar", "thunar" },
	{ "ROX filer", "rox" },
	{ "GWorkspace", "GWorkspace" },
	{ "Midnight Commander", "mc !" },
	{ "XFTree", "xftree" },
	{ "Konqueror", "konqueror" },
	{ "Nautilus", "nautilus --no-desktop" },
	{ "FSViewer", "fsviewer" },
	{ "Xfe", "xfe" },
	{ NULL, NULL }
};

char *Mathematics[MAX_NR_APPS][2] = {
	{ "Xmaxima", "xmaxima" },
	{ "Maxima", "maxima !" },
	{ "Maple", "maple" },
	{ "Scilab", "scilab" },
	{ "bc", "bc !" },
	{ "KCalc", "kcalc" },
	{ "XCalc", "xcalc" },
	{ "Mathematica", "mathematica" },
	{ "Math", "math" }, /* what's this? */
	{ "Free42", "free42" },
	{ "X48", "x48" },
	{ NULL, NULL }
};

char *Astronomy[MAX_NR_APPS][2] = {
	{ "Xplns", "xplns" },
	{ "Stellarium", "stellarium" },
	{ NULL, NULL }
};

char *Graphics[MAX_NR_APPS][2] = {
	{ "GIMP", "gimp" },
	{ "Sodipodi", "sodipodi" },
	{ "Inkscape", "inkscape" },
	{ "KIllustrator", "killustrator" },
	{ "Krayon", "krayon" },
	{ "KPovModeler", "kpovmodeler" },
	{ "XBitmap", "bitmap" },
	{ "XPaint", "xpaint" },
	{ "XFig", "xfig" },
	{ "KPaint", "kpaint" },
	{ "Blender", "blender" },
	{ "KSnapshot", "ksnapshot" },
	{ "GPhoto", "gphoto" },
	{ "Dia", "dia" },
	{ "CompuPic", "compupic" },
	{ "GQview", "gqview" },
	{ "Geeqie", "geeqie" },
	{ "KView", "kview" },
	{ "Pixie", "pixie" },
	{ "ImageMagick Display", "display" },
	{ "XV", "xv" },
	{ "Eye of GNOME", "eog" },
	{ "Quick Image Viewer", "qiv" },
	{ NULL, NULL },
};

char *Multimedia[MAX_NR_APPS][2] = {
	{ "Audacious", "audacious2" },
	{ "Kaffeine", "kaffeine", },
	{ "Audacity", "audacity" },
	{ "XMMS", "xmms" },
	{ "K9Copy", "k9copy" },
	{ "AcidRip", "acidrip" },
	{ "Avidemux", "avidemux2_gtk" },
	{ "GQmpeg", "gqmpeg" },
	{ "Freeamp", "freeamp" },
	{ "RealPlayer", "realplay" },
	{ "KMid", "kmid" },
	{ "Kmidi", "kmidi" },
	{ "Gtcd", "gtcd" },
	{ "Grip", "grip" },
	{ "AVIplay", "aviplay" },
	{ "Gtv", "gtv" },
	{ "VLC", "vlc" },
	{ "Sinek", "sinek" },
	{ "xine", "xine" },
	{ "aKtion", "aktion" },
	{ "Gcd", "gcd" },
	{ "XawTV", "xawtv" },
	{ "X-CD-Roast", "xcdroast" },
	{ "XPlayCD", "xplaycd" },
	{ NULL, NULL}
};

char *Internet[MAX_NR_APPS][2] = {
	{ "Chromium", "chromium" },
	{ "Chromium", "chromium-browser" },
	{ "Google Chrome", "google-chrome" },
	{ "Mozilla Firefox", "firefox" },
	{ "Galeon", "galeon" },
	{ "SkipStone", "skipstone" },
	{ "Konqueror", "konqueror" },
	{ "Dillo", "dillo" },
	{ "Epiphany", "epiphany" },
	{ "Opera", "opera" },
	{ "Midori", "midori" },
	{ "Mozilla SeaMonkey", "seamonkey" },
	{ "Kazehakase", "kazehakase" },
	{ "Links", "links !" },
	{ "Lynx", "lynx !" },
	{ "W3M", "w3m !" },
	{ NULL, NULL }
};

char *Email[MAX_NR_APPS][2] = {
	{ "Mozilla Thunderbird", "thunderbird" },
	{ "Mutt", "mutt !" },
	{ "GNUMail", "GNUMail" },
	{ "Evolution", "evolution" },
	{ "Kleopatra", "kleopatra" },
	{ "Sylpheed", "sylpheed" },
	{ "Spruce", "spruce" },
	{ "KMail", "kmail" },
	{ "Exmh", "exmh" },
	{ "Pine", "pine !" },
	{ "ELM", "elm !" },
	{ "Alpine", "alpine !" },
	{ NULL, NULL }
};

char *Sound[MAX_NR_APPS][2] = {
	{ "soundKonverter", "soundkonverter" },
	{ "Krecord", "krecord" },
	{ "Grecord", "grecord" },
	{ "ALSA mixer", "alsamixer !" },
	{ "Sound configuration", "sndconfig !" },
	{ "aumix", "aumix !" },
	{ "Gmix", "gmix" },
	{ NULL, NULL }
};

char *Editors[MAX_NR_APPS][2] = {
	{ "XJed", "xjed" },
	{ "Jed", "jed !" },
	{ "Emacs", "emacs" },
	{ "XEmacs", "xemacs" },
	{ "gVIM", "gvim" },
	{ "vi", "vi !" },
	{ "VIM", "vim !" },
	{ "gedit", "gedit" },
	{ "KEdit", "kedit" },
	{ "XEdit", "xedit" },
	{ "KWrite", "kwrite" },
	{ "Kate", "kate" },
	{ "Pico", "pico !" },
	{ "Nano", "nano !" },
	{ "Joe", "joe !" },
	{ NULL, NULL }
};

char *Comics[MAX_NR_APPS][2] = {
	{ "Omnia data", "omnia_data" },
	{ "Comix", "comix" },
	{ "QComicBook", "qcomicbook" },
	{ NULL, NULL }
};

char *Viewers[MAX_NR_APPS][2] = {
	{ "Evince", "evince" },
	{ "KGhostView", "kghostview" },
	{ "gv", "gv" },
	{ "GGv", "ggv" },
	{ "Xdvi", "xdvi" },
	{ "KDVI", "kdvi" },
	{ "Xpdf", "xpdf" },
	{ "Adobe Reader", "acroread" },
	{ "Gless", "gless" },
	{ NULL, NULL }
};

char *Utilities[MAX_NR_APPS][2] = {
	{ "gdlinux", "gdlinux" },
	{ "K3B", "k3b" },
	{ "gtkfind", "gtkfind" },
	{ "gdict", "gdict" },
	{ "gpsdrive", "gpsdrive" },
	{ "wfcmgr", "wfcmgr" },
	{ "switch", "switch" },
	{ "kaddressbook", "kaddressbook" },
	{ "kab", "kab" },
	{ "kfind", "kfind" },
	{ "oclock", "oclock" },
	{ "rclock", "rclock" },
	{ "xclock", "xclock" },
	{ "kppp", "kppp" },
	{ NULL, NULL }
};

char *Video[MAX_NR_APPS][2] = {
	{ "kaffeine", "kaffeine" },
	{ "gnomemeeting", "gnomemeeting" },
	{ NULL, NULL }
};

char *Chat[MAX_NR_APPS][2] = {
	{ "pidgin", "pidgin" },
	{ "skype", "skype" },
	{ "gizmo", "gizmo" },
	{ "kopete", "kopete" },
	{ "xchat", "xchat" },
	{ "kvirc", "kvirc" },
	{ "BitchX", "BitchX !" },
	{ "epic", "epic !" },
	{ "epic4", "epic4 !" },
	{ "irssi", "irssi !" },
	{ "tinyirc", "tinyirc !" },
	{ "ksirc", "ksirc" },
	{ "gtalk", "gtalk" },
	{ "gnome-icu", "gnome-icu" },
	{ "licq", "licq" },
	{ "amsn", "amsn" },
	{ NULL, NULL }
};

char *P2P[MAX_NR_APPS][2] = {
	{ "amule", "amule" },
	{ "gftp", "gftp" },
	{ "smb4k", "smb4k" },
	{ "ktorrent", "ktorrent" },
	{ "bittorrent-gui", "bittorrent-gui" },
	{ "ftp", "ftp !" },
	{ "sftp", "sftp !" },
	{ "pavuk", "pavuk" },
	{ "gtm", "gtm !" },
	{ "gnut", "gnut !" },
	{ "gtk-gnutella", "gtk-gnutella" },
	{ "gnutmeg", "gnutmeg" },
	{ NULL, NULL }
};

char *Games[MAX_NR_APPS][2] = {
	{ "fgfs", "fgfs" },
	{ "tremulous", "tremulous" },
	{ "xboard", "xboard" },
	{ "gnome-chess", "gnome-chess" },
	{ "quake2", "quake2" },
	{ "quake3", "quake3" },
	{ "q3ut2", "q3ut2" },
	{ "sof", "sof" },
	{ "rune", "rune" },
	{ "tribes2", "tribes2" },
	{ "unreal", "unreal" },
	{ "descent3", "descent3" },
	{ "myth2", "myth2" },
	{ "rt2", "rt2" },
	{ "heretic2", "heretic2" },
	{ "kohan", "kohan" },
	{ "xqf", "xqf" },
	{ NULL, NULL }
};

char *Office[MAX_NR_APPS][2] = {
	{ "OpenOffice.org Writer", "oowriter" },
	{ "OpenOffice.org Calc", "oocalc" },
	{ "ooconfigimport", "ooconfigimport" },
	{ "OpenOffice.org Draw", "oodraw" },
	{ "OpenOffice.org Impress", "ooimpress" },
	{ "OpenOffice.org Math", "oomath" },
	{ "OpenOffice.org", "ooffice" },
	{ "AbiWord", "abiword" },
	{ "KWord", "kword" },
	{ "smath", "smath" },
	{ "swriterkpresenter", "swriterkpresenter" },
	{ "lyx", "lyx" },
	{ "klyx", "klyx" },
	{ "gnucash", "gnucash" },
	{ "gnumeric", "gnumeric" },
	{ "kspread", "kspread" },
	{ "kchart", "kchart" },
	{ "gnomecal", "gnomecal" },
	{ "gnomecard", "gnomecard" },
	{ "korganizer", "korganizer" },
	{ NULL, NULL }
};

char *Development[MAX_NR_APPS][2] = {
	{ "gitk", "gitk" },
	{ "gitview", "gitview" },
	{ "qgit", "qgit" },
	{ "git-gui", "git-gui" },
	{ "glimmer", "glimmer" },
	{ "glade", "glade" },
	{ "kdevelop", "kdevelop" },
	{ "designer", "designer" },
	{ "kbabel", "kbabel" },
	{ "idle", "idle" },
	{ "ghex", "ghex" },
	{ "hexedit", "hexedit !" },
	{ "memprof", "memprof" },
	{ "tclsh", "tclsh !" },
	{ "gdb", "gdb !" },
	{ "xxgdb", "xxgdb" },
	{ "xev", "xev !" },
	{ NULL, NULL }
};

char *System[MAX_NR_APPS][2] = {
	{ "iotop", "iotop -d 4 --only !" },
	{ "keybconf", "keybconf" },
	{ "gtop", "gtop" },
	{ "top", "top !" },
	{ "kpm", "kpm" },
	{ "gw", "gw" },
	{ "gnomecc", "gnomecc" },
	{ "gkrellm", "gkrellm" },
	{ "tksysv", "tksysv" },
	{ "ksysv", "ksysv" },
	{ "gnome-ppp", "gnome-ppp" },
	{ "iostat", "iostat -p -k 5 !" },
	{ NULL, NULL }
};

char *OpenSUSE[MAX_NR_APPS][2] = {
	{ "yast2", "yast2" },
	{ "yast", "yast !" },
	{ "systemsettings", "systemsettings" },
	{ "umtsmon", "umtsmon" },
	{ NULL, NULL }
};

char *Mandriva[MAX_NR_APPS][2] = {
	{ "draknetcenter", "draknetcenter" },
	{ "rpmdrake", "rpmdrake" },
	{ "harddrake", "harddrake" },
	{ "drakconf", "drakconf" },
	{ "MandrakeUpdate", "MandrakeUpdate" },
	{ "Xdrakres", "Xdrakres" },
	{ NULL, NULL }
};

char *WindowMaker[MAX_NR_APPS][2] = {
	{ "wmnet", "wmnet -d 100000 -Weth0" },
	{ "wmpower", "wmpower" },
	{ "wmlaptop2", "wmlaptop2" },
	{ "wmwifi", "wmwifi -s" },
	{ "wmifinfo", "wmifinfo" },
	{ "wmWeather", "wmWeather" },
	{ "wmstickynotes", "wmstickynotes" },
	{ "wmmixer++", "wmmixer++ -w" },
	{ "wmWeather", "wmWeather -m -s EDDB" },
	{ "wmcpuload", "wmcpuload" },
	{ "wmcpufreq", "wmcpufreq" },
	{ "wmclockmon", "wmclockmon" },
	{ "wmnd", "wmnd" },
	{ "wmCalclock", "wmCalclock -S" },
	{ "wmtime", "wmtime" },
	{ "wmdate", "wmdate" },
	{ "wmmon", "wmmon" },
	{ "wmsysmon", "wmsysmon" },
	{ "wmSMPmon", "wmSMPmon" },
	{ "wmifs", "wmifs" },
	{ "wmnd", "wmnd" },
	{ "wmbutton", "wmbutton" },
	{ "wmxmms", "wmxmms" },
	{ "wmpower", "wmpower" },
	{ "wmagnify", "wmagnify" },
	{ NULL, NULL }
};

char *other_wm[MAX_WMS][2] = {
	{ "IceWM", "icewm" },
	{ "KWin", "kwin" },
	{ "twm", "twm" },
	{ "Fluxbox", "fluxbox" },
	{ "Blackbox", "blackbox" },
	{ "ion", "ion" },
	{ "MWM", "mwm" },
	{ NULL, NULL }
};
