/*
 * Hoofdmenu-uitwerking voor WindowMaker
 *
 * Opmaak is:
 *
 * <Titel> [SHORTCUT <Sneltoets>] <Commando> <Parameters>
 *
 * <Titel> is elke string te gebruiken als titel. Moet tussen " staan als 't
 * 	spaties heeft
 * 
 * SHORTCUT geeft 'n sneltoets op voor dat item. <Sneltoets> heeft
 * dezelfde opmaak als de sneltoetsopties in 't
 * $HOME/GNUstep/Defaults/WindowMaker bestand, zoals RootMenuKey of MiniaturizeKey.
 *
 * U kunt geen sneltoets opgeven voor 'n MENU- of OPEN_MENU-item.
 * 
 * <Command> één van de geldige commando's: 
 *	MENU - begint (sub)menubepaling
 *	END  - beëindigt (sub)menubepaling
 *	OPEN_MENU - opent 'n menu uit 'n bestand, pipe of map(pen)inhoud
 *		    en gaat eventueel elk vooraf met 'n commando.
 *	WORKSPACE_MENU - voegt 'n submenu toe voor werkruimtehandelingen. Slechts één
 *		    workspace_menu is toegestaan. 		
 *	EXEC <programma> - voert 'n extern programma uit
 *	SHEXEC <commando> - voert 'n shellcommando uit (zoals gimp > /dev/null)
 *	EXIT - sluit de vensterbeheerder af
 *	RESTART [<vensterbeheerder>] - herstart WindowMaker of start 'n andere
 *			vensterbeheerder
 *	REFRESH - vernieuwt 't bureaublad
 *	ARRANGE_ICONS - herschikt de iconen in de werkruimte
 *	SHUTDOWN - doodt alle cliënten (en sluit de X Window-sessie af)
 *	SHOW_ALL - plaatst alle vensters in de werkruimte terug
 *	HIDE_OTHERS - verbergt alle vensters in de werkruimte, behalve die
 *		focus heeft (of de laatste die focus had)
 *	SAVE_SESSION - slaat de huidige staat van 't bureaublad op, inbegrepen
 *		       alle draaiende programma's, al hun hints (afmetingen,
 *		       positie op scherm, werkruimte waarin ze leven, 't dok
 *		       of clip van waaruit ze werden opgestart, en indien
 *		       geminiaturiseerd, opgerold of verborgen). Slaat tevens de huidige
 *		       werkruimte van de gebruiker op. Alles zal worden hersteld bij elke
 *		       start van windowmaker tot 'n andere SAVE_SESSION of
 *		       CLEAR_SESSION wordt gebruikt. Als SaveSessionOnExit = Yes; in
 *		       WindowMaker-domeinbestand, dan wordt opslaan automatisch
 *		       gedaan bij elke windowmaker-afsluiting, en overschrijft 'n
 *		       SAVE_SESSION of CLEAR_SESSION (zie beneden).
 *	CLEAR_SESSION - wist 'n eerder opgeslagen sessie. Dit zal geen
 *		       effect hebben als SaveSessionOnExit is True.
 *	INFO - toont 't Infopaneel
 *
 * OPEN_MENU-opmaak:
 *   1. Bestandsmenubehandeling.
 *	// opent bestand.menu dat 'n geldig menubestand moet bevatten en voegt
 *	// 't in op huidige plaats
 *	OPEN_MENU bestand.menu
 *   2. Pipe-menubehandeling.
 *	// opent commando en gebruikt z'n stdout om menu aan te maken.
 *	// Commando-uitvoer moet 'n geldige menubeschrijving zijn.
 *	// De ruimte tussen "|" en commando zelf is optioneel.
 *      // Gebruik "||" in plaats van "|" als u 't menu altijd wilt bijwerken
 *	// bij openen. 't Zou traag kunnen zijn.
 *	OPEN_MENU | commando
 *      OPEN_MENU || commando
 *   3. Mapbehandeling.
 *	// Opent één of meer mappen en maakt 'n menu aan met daarin alle
 *	// submappen en uitvoerbare bestanden alfabetisch
 *	// gesorteerd.
 *	OPEN_MENU /een/map [/een/andere/map ...]
 *   4. Mapbehandeling met commando.
 *	// Opent één of meer mappen en maakt menu aan met daarin alle
 *	// submappen en leesbare bestanden alfabetisch gesorteerd,
 *	// elk van hen voorafgegaan met commando.
 *	OPEN_MENU [opties] /een/map [/een/andere/map ...] WITH commando -opties
 *		Opties:
 * 			-noext 	haal alles eraf, wat na de laatste punt in de
 *				bestandsnaam komt
 *
 * <Parameters> is 't programma om uit te voeren.
 *
 * ** Commandoregelopties in EXEC:
 * %s - wordt vervangen door huidige selectie
 * %a(titel[,aanwijzing]) - opent 'n invoerveld met de opgegeven titel en de
 *			optionele aanwijzing	en wordt vervangen door wat u intypt
 * %w - wordt vervangen door XID voor 't huidig gefocust venster
 * %W - wordt vervangen door 't nummer van de huidige werkruimte
 * 
 * U kunt speciale karakters (zoals % en ") uitschakelen met 't \-teken:
 * vb.: xterm -T "\"Hallo Wereld\""
 *
 * U kunt ook ontsnappingstekens gebruiken, zoals \n
 *
 * Elke MENU-declaratie moet één gekoppelde END-declaratie op 't eind hebben.
 *
 * Voorbeeld:
 *
 * "Test" MENU
 *	"XTerm" EXEC xterm
 *		// maakt 'n submenu met de inhoud van /usr/openwin/bin
 *	"XView-progr" OPEN_MENU "/usr/openwin/bin"
 *		// enige X11-programma's in verschillende mappen
 *	"X11-progr" OPEN_MENU /usr/X11/bin $HOME/bin/X11
 *		// stel enige achtergrondafbeeldingen in
 *	"Achtergrond" OPEN_MENU -noext $HOME/afbeeldingen /usr/share/images WITH wmsetbg -u -t
 *		// voegt 't stijl.menu in, in dit item
 *	"Stijl" OPEN_MENU stijl.menu
 * "Test" END
 */

#include "wmmacros"

"Programma's" MENU
	"Info" MENU
		"Infopaneel" INFO_PANEL
		"Juridische info" LEGAL_PANEL
		"Systeemconsole" EXEC xconsole
		"Systeembelasting" SHEXEC xosview || xload
		"Proceslijst" EXEC xterm -e top
		"Handleidingbrowser" EXEC xman
	"Info" END
	"Uitvoeren..." SHEXEC %a(Uitvoeren,Typ uit te voeren commando:)
	"XTerm" EXEC xterm -sb 
	"Mozilla Firefox" EXEC firefox
	"Werkruimten" WORKSPACE_MENU
	"Programma's" MENU
		"Gimp" SHEXEC gimp >/dev/null
  		"Ghostview" EXEC ghostview %a(GhostView,Voer te bekijken bestand in)
		"Xpdf" EXEC xpdf %a(Xpdf,Voer te bekijken PDF in)
		"Abiword" EXEC abiword
		"Dia" EXEC dia
		"OpenOffice.org" MENU
			"OpenOffice.org" EXEC ooffice
			"Writer" EXEC oowriter
			"Rekenblad" EXEC oocalc
			"Draw" EXEC oodraw
			"Impress" EXEC ooimpress
		"OpenOffice.org" END 

		"Tekstbewerkers" MENU
			"XEmacs" EXEC xemacs
			"Emacs" EXEC emacs
			"XJed" EXEC xjed 
			"VI" EXEC xterm -e vi
			"GVIM" EXEC gvim
			"NEdit" EXEC nedit
			"Xedit" EXEC xedit
		"Tekstbewerkers" END

		"Multimedia" MENU
			"XMMS" MENU
				"XMMS" EXEC xmms
				"XMMS afspelen/pauzeren" EXEC xmms -t
				"XMMS stoppen" EXEC xmms -s
			"XMMS" END
			"Xine videospeler" EXEC xine
			"MPlayer" EXEC mplayer
		"Multimedia" END
	"Programma's" END

	"Hulpmiddelen" MENU
		"Rekenmachine" EXEC xcalc
		"Venstereigenschappen" SHEXEC xprop | xmessage -center -title 'xprop' -file -
		"Lettertype kiezen" EXEC xfontsel
		"Vergroten" EXEC wmagnify
		"Kleurenkaart" EXEC xcmap
		"X-programma doden" EXEC xkill
	"Hulpmiddelen" END

	"Selectie" MENU
		"Kopiëren" SHEXEC echo '%s' | wxcopy
		"E-mailen naar" EXEC xterm -name mail -T "Pine" -e pine %s
		"Navigeren" EXEC netscape %s
		"Zoeken in handleiding" SHEXEC MANUAL_SEARCH(%s)
	"Selectie" END

	"Commando's" MENU
		"Andere verbergen" HIDE_OTHERS
		"Alles tonen" SHOW_ALL
		"Iconen schikken" ARRANGE_ICONS
		"Vernieuwen" REFRESH
		"Vergrendelen" EXEC xlock -allowroot -usefirst
	"Commando's" END

	"Uiterlijk" MENU
		"Thema's" OPEN_MENU -noext THEMES_DIR $HOME/GNUstep/Library/WindowMaker/Themes WITH setstyle
		"Stijlen" OPEN_MENU -noext STYLES_DIR $HOME/GNUstep/Library/WindowMaker/Styles WITH setstyle
		"Iconensets" OPEN_MENU -noext ICON_SETS_DIR $HOME/GNUstep/Library/WindowMaker/IconSets WITH seticons
		"Achtergrond" MENU
			"Effen" MENU
				"Zwart" WS_BACK '(solid, black)'
				"Blauw"  WS_BACK '(solid, "#505075")'
				"Indigo" WS_BACK '(solid, "#243e6c")'
				"Marineblauw" WS_BACK '(solid, "#224477")'
				"Purper" WS_BACK '(solid, "#554466")'
				"Tarwe"  WS_BACK '(solid, "wheat4")'
				"Donkergrijs"  WS_BACK '(solid, "#333340")'
				"Wijnrood" WS_BACK '(solid, "#400020")'
				"Effen" END
			"Kleurverloop" MENU
				"Zonsondergang" WS_BACK '(mvgradient, deepskyblue4, black, deepskyblue4, tomato4)'
				"Lucht" WS_BACK '(vgradient, blue4, white)'
    			"Blauwtinten" WS_BACK '(vgradient, "#7080a5", "#101020")'
				"Indigotinten" WS_BACK '(vgradient, "#746ebc", "#242e4c")'
			    "Purpertinten" WS_BACK '(vgradient, "#654c66", "#151426")'
    			"Tarwetinten" WS_BACK '(vgradient, "#a09060", "#302010")'
    			"Grijstinten" WS_BACK '(vgradient, "#636380", "#131318")'
    			"Wijnroodtinten" WS_BACK '(vgradient, "#600040", "#180010")'
			"Kleurverloop" END
			"Afbeeldingen" OPEN_MENU -noext BACKGROUNDS_DIR $HOME/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t
		"Achtergrond" END
		"Thema opslaan" SHEXEC getstyle -t $HOME/GNUstep/Library/WindowMaker/Themes/"%a(Themanaam,Voer bestandsnaam in:)"
		"Iconenset opslaan" SHEXEC geticonset $HOME/GNUstep/Library/WindowMaker/IconSets/"%a(Iconensetnaam,Voer bestandsnaam in:)"
		"Voorkeurenhulpmiddel" EXEC /usr/local/GNUstep/Applications/WPrefs.app/WPrefs
	"Uiterlijk" END

	"Sessie" MENU
		"Sessie opslaan" SAVE_SESSION
		"Sessie wissen" CLEAR_SESSION
		"Window Maker herstarten" RESTART
		"BlackBox starten" RESTART blackbox
		"IceWM starten" RESTART icewm
		"Afsluiten"  EXIT
	"Sessie" END
"Programma's" END


