/* Copyright (C) 2010 Carlos R. Mafra */

/*
 * If the program should run from inside a terminal it has
 * to end with a space followed by '!', e.g.  "mutt !"
 */

char *Terminals[MAX_NR_APPS][2] = {
	{ N_("xterm"), "xterm -bg black -fg white +sb +sm -fn 10x20 -sl 4000 -cr yellow" },
	{ N_("mrxvt"), "mrxvt -rv -shade 00 -vb +sb +sm -tr -sl 2000 -trt -itabbg black -hb -tabfg yellow -fn 10x20 -cr yellow" },
	{ N_("Konsole"), "konsole" },
	{ NULL, NULL }
};

char *File_managers[MAX_NR_APPS][2] = {
	{ N_("Dolphin"), "dolphin" },
	{ N_("Thunar"), "thunar" },
	{ N_("ROX filer"), "rox" },
	{ N_("GWorkspace"), "GWorkspace" },
	{ N_("Midnight Commander"), "mc !" },
	{ N_("XFTree"), "xftree" },
	{ N_("Konqueror"), "konqueror" },
	{ N_("Nautilus"), "nautilus --no-desktop" },
	{ N_("FSViewer"), "fsviewer" },
	{ N_("Xfe"), "xfe" },
	{ NULL, NULL }
};

char *Mathematics[MAX_NR_APPS][2] = {
	{ N_("Xmaxima"), "xmaxima" },
	{ N_("Maxima"), "maxima !" },
	{ N_("Maple"), "maple" },
	{ N_("Scilab"), "scilab" },
	{ N_("bc"), "bc !" },
	{ N_("KCalc"), "kcalc" },
	{ N_("XCalc"), "xcalc" },
	{ N_("Mathematica"), "mathematica" },
	{ N_("Math"), "math" },		/* command-line Mathematica */
	{ N_("Free42"), "free42" },
	{ N_("X48"), "x48" },
	{ NULL, NULL }
};

char *Astronomy[MAX_NR_APPS][2] = {
	{ N_("Xplns"), "xplns" },
	{ N_("Stellarium"), "stellarium" },
	{ NULL, NULL }
};

char *Graphics[MAX_NR_APPS][2] = {
	{ N_("GIMP"), "gimp" },
	{ N_("Sodipodi"), "sodipodi" },
	{ N_("Inkscape"), "inkscape" },
	{ N_("KIllustrator"), "killustrator" },
	{ N_("Krayon"), "krayon" },
	{ N_("KPovModeler"), "kpovmodeler" },
	{ N_("XBitmap"), "bitmap" },
	{ N_("XPaint"), "xpaint" },
	{ N_("XFig"), "xfig" },
	{ N_("KPaint"), "kpaint" },
	{ N_("Blender"), "blender" },
	{ N_("KSnapshot"), "ksnapshot" },
	{ N_("GPhoto"), "gphoto" },
	{ N_("DigiKam"), "digikam" },
	{ N_("Dia"), "dia" },
	{ N_("CompuPic"), "compupic" },
	{ N_("GQview"), "gqview" },
	{ N_("Geeqie"), "geeqie" },
	{ N_("KView"), "kview" },
	{ N_("Pixie"), "pixie" },
	{ N_("ImageMagick Display"), "display" },
	{ N_("XV"), "xv" },
	{ N_("Eye of GNOME"), "eog" },
	{ N_("Quick Image Viewer"), "qiv" },
	{ NULL, NULL },
};

char *Multimedia[MAX_NR_APPS][2] = {
	{ N_("Audacious"), "audacious2" },
	{ N_("Kaffeine"), "kaffeine", },
	{ N_("Audacity"), "audacity" },
	{ N_("Amarok"), "amarok" },
	{ N_("XMMS"), "xmms" },
	{ N_("K9Copy"), "k9copy" },
	{ N_("HandBrake"), "HandBrakeGUI" },
	{ N_("OGMRip"), "ogmrip" },
	{ N_("DVBcut"), "dvbcut" },
	{ N_("AcidRip"), "acidrip" },
	{ N_("Avidemux"), "avidemux2_gtk" },
	{ N_("GQmpeg"), "gqmpeg" },
	{ N_("Freeamp"), "freeamp" },
	{ N_("RealPlayer"), "realplay" },
	{ N_("Mediathek"), "Mediathek.sh" },
	{ N_("KMid"), "kmid" },
	{ N_("Kmidi"), "kmidi" },
	{ N_("Gtcd"), "gtcd" },
	{ N_("Grip"), "grip" },
	{ N_("AVIplay"), "aviplay" },
	{ N_("Gtv"), "gtv" },
	{ N_("VLC"), "vlc" },
	{ N_("Sinek"), "sinek" },
	{ N_("xine"), "xine" },
	{ N_("aKtion"), "aktion" },
	{ N_("Gcd"), "gcd" },
	{ N_("XawTV"), "xawtv" },
	{ N_("X-CD-Roast"), "xcdroast" },
	{ N_("XPlayCD"), "xplaycd" },
	{ NULL, NULL}
};

char *Internet[MAX_NR_APPS][2] = {
	{ N_("Chromium"), "chromium" },
	{ N_("Chromium"), "chromium-browser" },
	{ N_("Google Chrome"), "google-chrome" },
	{ N_("Mozilla Firefox"), "firefox" },
	{ N_("Galeon"), "galeon" },
	{ N_("SkipStone"), "skipstone" },
	{ N_("Konqueror"), "konqueror" },
	{ N_("Dillo"), "dillo" },
	{ N_("Epiphany"), "epiphany" },
	{ N_("Opera"), "opera" },
	{ N_("Midori"), "midori" },
	{ N_("Mozilla SeaMonkey"), "seamonkey" },
	{ N_("Kazehakase"), "kazehakase" },
	{ N_("Links"), "links !" },
	{ N_("Lynx"), "lynx !" },
	{ N_("W3M"), "w3m !" },
	{ NULL, NULL }
};

char *Email[MAX_NR_APPS][2] = {
	{ N_("Mozilla Thunderbird"), "thunderbird" },
	{ N_("Mutt"), "mutt !" },
	{ N_("GNUMail"), "GNUMail" },
	{ N_("Evolution"), "evolution" },
	{ N_("Kleopatra"), "kleopatra" },
	{ N_("Sylpheed"), "sylpheed" },
	{ N_("Spruce"), "spruce" },
	{ N_("KMail"), "kmail" },
	{ N_("Exmh"), "exmh" },
	{ N_("Pine"), "pine !" },
	{ N_("ELM"), "elm !" },
	{ N_("Alpine"), "alpine !" },
	{ NULL, NULL }
};

char *Sound[MAX_NR_APPS][2] = {
	{ N_("soundKonverter"), "soundkonverter" },
	{ N_("Krecord"), "krecord" },
	{ N_("Grecord"), "grecord" },
	{ N_("ALSA mixer"), "alsamixer !" },
	{ N_("Sound configuration"), "sndconfig !" },
	{ N_("aumix"), "aumix !" },
	{ N_("Gmix"), "gmix" },
	{ NULL, NULL }
};

char *Editors[MAX_NR_APPS][2] = {
	{ N_("XJed"), "xjed" },
	{ N_("Jed"), "jed !" },
	{ N_("Emacs"), "emacs" },
	{ N_("XEmacs"), "xemacs" },
	{ N_("gVIM"), "gvim" },
	{ N_("vi"), "vi !" },
	{ N_("VIM"), "vim !" },
	{ N_("gedit"), "gedit" },
	{ N_("KEdit"), "kedit" },
	{ N_("XEdit"), "xedit" },
	{ N_("KWrite"), "kwrite" },
	{ N_("Kate"), "kate" },
	{ N_("Pico"), "pico !" },
	{ N_("Nano"), "nano !" },
	{ N_("Joe"), "joe !" },
	{ NULL, NULL }
};

char *Comics[MAX_NR_APPS][2] = {
	{ N_("Omnia data"), "omnia_data" },
	{ N_("Comix"), "comix" },
	{ N_("QComicBook"), "qcomicbook" },
	{ NULL, NULL }
};

char *Viewers[MAX_NR_APPS][2] = {
	{ N_("Evince"), "evince" },
	{ N_("KGhostView"), "kghostview" },
	{ N_("gv"), "gv" },
	{ N_("GGv"), "ggv" },
	{ N_("Xdvi"), "xdvi" },
	{ N_("KDVI"), "kdvi" },
	{ N_("Xpdf"), "xpdf" },
	{ N_("Adobe Reader"), "acroread" },
	{ N_("Gless"), "gless" },
	{ NULL, NULL }
};

char *Utilities[MAX_NR_APPS][2] = {
	{ N_("Google Desktop"), "gdlinux" },
	{ N_("K3B"), "k3b" },
	{ N_("gtkfind"), "gtkfind" },
	{ N_("gdict"), "gdict" },
	{ N_("gpsdrive"), "gpsdrive" },
	{ N_("wfcmgr"), "wfcmgr" },
	{ N_("switch"), "switch" },
	{ N_("kaddressbook"), "kaddressbook" },
	{ N_("kab"), "kab" },
	{ N_("kfind"), "kfind" },
	{ N_("oclock"), "oclock" },
	{ N_("rclock"), "rclock" },
	{ N_("xclock"), "xclock" },
	{ N_("kppp"), "kppp" },
	{ NULL, NULL }
};

char *Video[MAX_NR_APPS][2] = {
	{ N_("Kaffeine"), "kaffeine" },
	{ N_("Ekiga"), "Ekiga" },
	{ NULL, NULL }
};

char *Chat[MAX_NR_APPS][2] = {
	{ N_("Pidgin"), "pidgin" },
	{ N_("Skype"), "skype" },
	{ N_("Gizmo"), "gizmo" },
	{ N_("Kopete"), "kopete" },
	{ N_("XChat"), "xchat" },
	{ N_("KVIrc"), "kvirc" },
	{ N_("BitchX"), "BitchX !" },
	{ N_("EPIC"), "epic !" },
	{ N_("EPIC4"), "epic4 !" },
	{ N_("Irssi"), "irssi !" },
	{ N_("TinyIRC"), "tinyirc !" },
	{ N_("Ksirc"), "ksirc" },
	{ N_("gtalk"), "gtalk" },
	{ N_("GnomeICU"), "gnome-icu" },
	{ N_("Licq"), "licq" },
	{ N_("aMSN"), "amsn" },
	{ NULL, NULL }
};

char *P2P[MAX_NR_APPS][2] = {
	{ N_("aMule"), "amule" },
	{ N_("gFTP"), "gftp" },
	{ N_("Smb4K"), "smb4k" },
	{ N_("KTorrent"), "ktorrent" },
	{ N_("BitTorrent GUI"), "bittorrent-gui" },
	{ N_("ftp"), "ftp !" },
	{ N_("sftp"), "sftp !" },
	{ N_("Pavuk"), "pavuk" },
	{ N_("gtm"), "gtm !" },
	{ N_("Gnut"), "gnut !" },
	{ N_("GTK Gnutella"), "gtk-gnutella" },
	{ N_("Gnutmeg"), "gnutmeg" },
	{ NULL, NULL }
};

char *Games[MAX_NR_APPS][2] = {
	{ N_("FlightGear Flight Simulator"), "fgfs" },
	{ N_("Tremulous"), "tremulous" },
	{ N_("XBoard"), "xboard" },
	{ N_("GNOME Chess"), "gnome-chess" },
	{ N_("Quake 2"), "quake2" },
	{ N_("Quake 3"), "quake3" },
	{ N_("Quake 3: Urban Terror 2"), "q3ut2" },
	{ N_("Soldier of Fortune"), "sof" },
	{ N_("Rune"), "rune" },
	{ N_("Tribes 2"), "tribes2" },
	{ N_("Unreal Tournament"), "unreal" },
	{ N_("Descent 3"), "descent3" },
	{ N_("Myth 2"), "myth2" },
	{ N_("Sauerbraten"), "sauerbraten" },
	{ N_("Sauerbraten"), "sauer_client" },
	{ N_("Railroad Tycoon 2"), "rt2" },
	{ N_("Heretic 2"), "heretic2" },
	{ N_("Kohan"), "kohan" },
	{ N_("XQF"), "xqf" },
	{ NULL, NULL }
};

char *Office[MAX_NR_APPS][2] = {
	{ N_("OpenOffice.org Writer"), "oowriter" },
	{ N_("OpenOffice.org Calc"), "oocalc" },
	{ N_("OpenOffice.org Draw"), "oodraw" },
	{ N_("OpenOffice.org Impress"), "ooimpress" },
	{ N_("OpenOffice.org Math"), "oomath" },
	{ N_("OpenOffice.org"), "ooffice" },
	{ N_("StarOffice Writer"), "swriter" },
	{ N_("StarOffice Calc"), "scalc" },
	{ N_("StarOffice Draw"), "sdraw" },
	{ N_("StarOffice Impress"), "simpress" },
	{ N_("StarOffice Math"), "smath" },
	{ N_("StarOffice"), "soffice" },
	{ N_("AbiWord"), "abiword" },
	{ N_("KWord"), "kword" },
	{ N_("KPresenter"), "kpresenter" },
	{ N_("KSpread"), "kspread" },
	{ N_("KChart"), "kchart" },
	{ N_("KOrganizer"), "Korganizer" },
	{ N_("LyX"), "lyx" },
	{ N_("Klyx"), "klyx" },
	{ N_("GnuCash"), "gnucash" },
	{ N_("Gnumeric"), "gnumeric" },
	{ N_("GnomeCal"), "gnomecal" },
	{ N_("GnomeCard"), "gnomecard" },
	{ NULL, NULL }
};

char *Development[MAX_NR_APPS][2] = {
	{ N_("gitk"), "gitk" },
	{ N_("gitview"), "gitview" },
	{ N_("qgit"), "qgit" },
	{ N_("git-gui"), "git-gui" },
	{ N_("glimmer"), "glimmer" },
	{ N_("glade"), "glade" },
	{ N_("kdevelop"), "kdevelop" },
	{ N_("designer"), "designer" },
	{ N_("kbabel"), "kbabel" },
	{ N_("idle"), "idle" },
	{ N_("ghex"), "ghex" },
	{ N_("hexedit"), "hexedit !" },
	{ N_("memprof"), "memprof" },
	{ N_("tclsh"), "tclsh !" },
	{ N_("gdb"), "gdb !" },
	{ N_("xxgdb"), "xxgdb" },
	{ N_("xev"), "xev !" },
	{ NULL, NULL }
};

char *System[MAX_NR_APPS][2] = {
	{ N_("Iotop"), "iotop -d 4 --only !" },
	{ N_("Iostat"), "iostat -p -k 5 !" },
	{ N_("keybconf"), "keybconf" },
	{ N_("GNOME System Monitor"), "gtop" },
	{ N_("top"), "top !" },
	{ N_("KDE Process Monitor"), "kpm" },
	{ N_("gw"), "gw" },
	{ N_("GNOME Control Center"), "gnomecc" },
	{ N_("GKrellM"), "gkrellm" },
	{ N_("tksysv"), "tksysv" },
	{ N_("ksysv"), "ksysv" },
	{ N_("GNOME PPP"), "gnome-ppp" },
	{ NULL, NULL }
};

char *OpenSUSE[MAX_NR_APPS][2] = {
	{ N_("YaST 2"), "yast2" },
	{ N_("YaST"), "yast !" },
	{ N_("System Settings"), "systemsettings" },
	{ N_("UMTSMon"), "umtsmon" },
	{ NULL, NULL }
};

char *Mandriva[MAX_NR_APPS][2] = {
	{ N_("DrakNetCenter"), "draknetcenter" },
	{ N_("RPMDrake"), "rpmdrake" },
	{ N_("HardDrake"), "harddrake" },
	{ N_("DrakConf"), "drakconf" },
	{ N_("MandrakeUpdate"), "MandrakeUpdate" },
	{ N_("XDrakRes"), "Xdrakres" },
	{ NULL, NULL }
};

char *WindowMaker[MAX_NR_APPS][2] = {
	{ N_("Docker"), "docker -wmaker" },
	{ N_("Net"), "wmnet -d 100000 -Weth0" },
	{ N_("Power"), "wmpower" },
	{ N_("Laptop"), "wmlaptop2" },
	{ N_("WiFi"), "wmwifi -s" },
	{ N_("Interface Info"), "wmifinfo" },
	{ N_("Weather"), "wmWeather" },
	{ N_("Sticky Notes"), "wmstickynotes" },
	{ N_("Mixer"), "wmmixer++ -w" },
	{ N_("Weather"), "wmWeather -m -s EDDB" },
	{ N_("CPU Load"), "wmcpuload" },
	{ N_("CPU Freq"), "wmcpufreq" },
	{ N_("Clock Mon"), "wmclockmon" },
	{ N_("Network Devices"), "wmnd" },
	{ N_("Calendar & Clock"), "wmCalclock -S" },
	{ N_("Time"), "wmtime" },
	{ N_("Date"), "wmdate" },
	{ N_("System Monitor"), "wmmon" },
	{ N_("System Monitor"), "wmsysmon" },
	{ N_("SMP Monitor"), "wmSMPmon" },
	{ N_("Interfaces"), "wmifs" },
	{ N_("Button"), "wmbutton" },
	{ N_("xmms"), "wmxmms" },
	{ N_("Power"), "wmpower" },
	{ N_("Magnify"), "wmagnify" },
	{ NULL, NULL }
};

char *other_wm[MAX_WMS][2] = {
	{ N_("IceWM"), "icewm" },
	{ N_("KWin"), "kwin" },
	{ N_("twm"), "twm" },
	{ N_("Fluxbox"), "fluxbox" },
	{ N_("Blackbox"), "blackbox" },
	{ N_("Ion"), "ion" },
	{ N_("Motif Window Manager"), "mwm" },
	{ N_("FVWM"), "fvwm" },
	{ NULL, NULL }
};
