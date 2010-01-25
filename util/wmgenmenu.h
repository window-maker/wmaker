/* Copyright (C) 2010 Carlos R. Mafra */

/*
 * If the program should run from inside a terminal it has
 * to finish with a space followed by '!', e.g.
 * "mutt !"
 */

static char *terminals[MAX_NR_APPS] = {
	"mrxvt -rv -shade 00 -vb +sb +sm -tr -sl 2000 -trt -itabbg black -hb -tabfg yellow -fn 10x20 -cr yellow",
	"xterm -bg black -fg white +sb +sm -fn 10x20 -sl 4000 -cr yellow",
	"konsole"
};

char *file_managers[MAX_NR_APPS] = {
	"dolphin", "thunar", "rox", "GWorkspace",
	"xftree", "konqueror", "nautilus --no-desktop", "fsviewer", "xfe"
};

char *Mathematiks[MAX_NR_APPS] = {
	"xmaxima", "maple" , "scilab" "maxima !", "bc !",
	"kcalc", "xcalc", "mathematica", "math"
};

char *Astronomie[MAX_NR_APPS] = {
	"xplns", "stellarium"
};

char *Graphics[MAX_NR_APPS] = {
	"gimp", "sodipodi", "killustrator", "krayon", "kpovmodeler",
	"bitmap", "xpaint", "xfig", "kpaint", "blender", "ksnapshot",
	"gphoto", "dia", "compupic", "gqview", "kview", "pixie",
	"display", "ee", "xv", "eog", "qiv !"
};

char *Multimedia[MAX_NR_APPS] = {
	"audacious2", "kaffeine", "audacity", "xmms", "k9copy", "acidrip",
	"avidemux2_gtk", "gqmpeg", "freeamp", "realplay",
	"kmid", "kmidi", "gtcd", "grip", "aviplay", "gtv", "gvlc", "sinek",
	"xine", "aktion", "gcd", "xawtv", "xcdroast", "xplaycd"
};

char *internet[MAX_NR_APPS] = {
	"chromium", "google-chrome", "firefox",
	"galeon", "skipstone", "konqueror",
	"dillo", "epiphany", "opera", "midori", "seamonkey",
	"kazehakase", "links !", "lynx !"
};

char *email[MAX_NR_APPS] = {
	"thunderbird", "mutt !", "GNUMail", "evolution",
	"kleopatra", "sylpheed", "spruce", "kmail", "exmh",
	"pine !", "elm !"
};

char *Sound[MAX_NR_APPS] = {
	"soundkonverter", "krecord", "grecord", "alsamixer !", "sndconfig !",
	"aumix !", "gmix"
};

char *Editors[MAX_NR_APPS] = {
	"xjed", "jed !", "emacs", "xemacs", "gvim", "vi !", "vim !", "gedit",
	"kedit", "xedit", "kwrite", "kate", "pico !", "nano !", "joe !"
};

char *Comics[MAX_NR_APPS] = {
	"omnia_data", "comix", "qcomicbook"
};

char *Viewers[MAX_NR_APPS] = {
	"evince", "kghostview", "gv", "ggv", "xdvi", "kdvi", "xpdf",
	"acroread", "gless"
};

char *Utilities[MAX_NR_APPS] = {
	"gdlinux", "k3b", "gtkfind", "gdict", "gpsdrive", "wfcmgr", "switch",
	"kaddressbook", "kab", "kfind", "oclock", "rclock", "xclock", "kppp"
};

char *Video[MAX_NR_APPS] = {
	"kaffeine", "gnomemeeting"
};

char *Chat[MAX_NR_APPS] = {
	"pidgin", "skype", "gizmo", "kopete", "xchat", "kvirc", "BitchX !",
	"epic !", "epic4 !", "irssi !", "tinyirc !", "ksirc", "gtalk",
	"gnome-icu", "licq", "amsn"
};

char *P2P[MAX_NR_APPS] = {
	"amule", "gftp", "smb4k", "ktorrent", "bittorrent-gui",
	"!ftp", "!sftp", "pavuk", "gtm","!gnut", "gtk-gnutella", "gnutmeg"
};

char *Games[MAX_NR_APPS] = {
	"fgfs", "tremulous", "xboard", "gnome-chess", "quake2", "quake3",
	"q3ut2", "sof", "rune", "tribes2", "unreal", "descent3", "myth2",
	"rt2", "heretic2", "kohan", "xqf"
};

char *Office[MAX_NR_APPS] = {
	"oowriter", "oocalc", "ooconfigimport", "oodraw", "ooffice",
	"ooimpress", "oomath", "ooregcomp", "abiword", "kword",
	"smath", "swriterkpresenter", "lyx", "klyx", "gnucash", "gnumeric",
	"kspread", "kchart","gnomecal", "gnomecard", "korganizer"
};

char *development[MAX_NR_APPS] = {
	"gitk", "gitview", "qgit", "git-gui", "glimmer", "glade", "kdevelop",
	"designer", "kbabel", "idle", "ghex", "hexedit !", "memprof", "tclsh !",
	"gdb !", "xxgdb", "xev !"
};

char *System[MAX_NR_APPS] = {
	"iotop -d 4 --only !", "keybconf", "gtop", "top !", "kpm", "gw", "gnomecc", "gkrellm",
	"tksysv", "ksysv", "gnome-ppp", "iostat -p -k 5 !"
};

char *OpenSUSE[MAX_NR_APPS] = {
	"yast2", "yast !", "systemsettings", "umtsmon"
};

char *Mandriva[MAX_NR_APPS] = {
	"draknetcenter", "rpmdrake", "harddrake", "drakconf",
	 "MandrakeUpdate", "Xdrakres"
};

char *WindowMaker[MAX_NR_APPS] = {
	"wmnet -d 100000 -Weth0", "wmpower", "wmlaptop2", "wmwifi -s", "wmifinfo",
	"wmWeather", "wmstickynotes", "wmmixer++ -w", "wmWeather -m -s EDDB",
	"wmcpuload", "wmcpufreq", "wmclockmon", "wmnd", "wmCalclock -S",
	"wmtime", "wmdate", "wmmon", "wmsysmon", "wmSMPmon", "wmifs",
	"wmnd", "wmbutton", "wmxmms", "wmpower", "wmagnify"
};

char *other_wm[MAX_WMS] = {
		"icewm", "kwin", "twm", "fluxbox", "blackbox", "ion"
};
