/*
 * Definiowanie Menu G³ównego dla WindowMakera
 * Fonty w standardzie ISO8895-2
 *
 * Sk³adnia jest nastêpuj±ca:
 *
 * <Tytu³> [SHORTCUT <Skrut>] <Komenda> <Paramery>
 *
 * <Tytu³> Tytu³ mo¿e byæ dowolnym ci±giem znaków. Je¶li bêd± w nim wystêpowaæ
 *         spacje umie¶c go w cudzys³owie np. "Tytu³ ze spacj±"
 * 
 * SHORTCUT Definiowanie skrótu.
 * <Skrót> Nazwa rodzaju skrótu umieszczonego w pliku
 *         $HOME/GNUstep/Defaults/WindowMaker, tak jak RootMenuKey 
 *         lub MiniaturizeKey.
 *
 * Skróty mog± wystêpowaæ w sekcji MENU lub OPEN_MENU .
 * 
 * <Komenda> jedna z dostêpnych komend: 
 *	MENU - rozpoczêcie definicji (pod)menu 
 *	END  - zakoñczenie definicji (pod)menu 
 *	OPEN_MENU - generowanie podmenu na podstawie podanego katalogu,
 *              umieszczaj±c w nim pliki wykonywalne i podkatalogi.
 *	WORKSPACE_MENU - Dodanie podmenu zawieraj±cego aktywne pulpity. Tylko
 *		             jedno workspace_menu jest potrzebne. 		
 *	EXEC <program> - wykonanie jakiegokolwiek programu
 *	EXIT - wyj¶cie z menad¿era okien
 *	RESTART [<window manager>] - restart WindowMakera albo start innego
 *			                     manad¿era okien
 *	REFRESH - od¶wierzenie ekranu
 *	ARRANGE_ICONS - uporz±dkowanie ikon na pulpicie
 *	SHUTDOWN - zabicie wszystkich procesów (i wyj¶cie z X window)
 *	SHOW_ALL - pokazanie wszystkich ukrytych programów
 *  HIDE_OTHERS - schowanie aktywnych okien pulpitu, oprócz aktywnego
 *  SAVE_SESSION - zapamietanie aktualnego stanu desktpou, z wszystkimi
 *                 uruchomionymi programami, i z wszystkimi ich stanami
 *                 geometrycznymi, pozycji na ekranie, umieszczone na
 *                 odpowiednim pulpicie, ukryte lub uaktywnione.
 *                 Wszystkie te ustawiemia bed± aktywne, dopóki nie
 *                 zostan± u¿yte komendy SAVE_SESSION i CLEAR_SESSION.
 *                 Je¿eli SaveSessionOnExit = Yes; w pliku konfiguracyjnym
 *                 WindowMakera, wtedy zapamiêtywanie wszystkich ustawieñ
 *                 jest dokonywanie po ka¿dym wyj¶ciu, niezale¿nie od
 *                 komend SAVE_SESSION czy CLEAR_SESSION .
 *  CLEAR_SESSION - Czyszczenie poprzednio zapamiêtanych sesji. Nie ponosi to
 *                  ¿adnych zmian w pliku SaveSessionOnExit .
 *  INFO - Wy¶wietlenie informacji o WindowMakerze
 *
 * <Parametry> zalezne od uruchamianego programu.
 *
 * ** Opcje w lini komend EXEC:
 * %s - znak jest zastepowany przez text znajdujacy sie w ,,schowku''
 * %a(tytu³[,komunikat]) - otwiera dodatkowe okno o tytule tytu³, komunikacie
 *                         komunikat i czeka na podanie parametrów, które 
 *                         zostan± wstawione zamiast %a. Niestety nie udalo mi
 *                         siê uzyskaæ polskich fontów w tej pocji :( 
 * %w - znak jest zastepowany przez XID aktywnego okna
 * %W - znak jest zastepowany przez numer aktywnego pulpitu
 * 
 * Aby u¿ywaæ specjalnych znaków ( takich jak % czy " ) nale¿y poprzedzic je znakiem \
 * np. :xterm -T "\"Witaj ¦wiecie\""
 *
 * Mo¿na u¿ywac znaków specjalnych, takich jak \n
 *
 * Sekcja MENU musi byæ zakoñczona sekcja END, pod t± sama nazw±.
 *
 * Przyk³ad:
 *
 * "Test" MENU
 *  "XTerm" EXEC xterm
 *      // stworzenie podmenu z plikami w podkatalogu /usr/openwin/bin
 *  "XView apps" OPEN_MENU "/usr/openwin/bin"
 *      // umieszcza w jednym podmenu pliki z róznych podkatalogów
 *  "X11 apps" OPEN_MENU /usr/X11/bin $HOME/bin/X11
 *      // ustawienie t³a
 *  "Background" OPEN_MENU -noext $HOME/images /usr/share/images WITH wmsetbg -u *      // wstawienie menu z pliku style.menu
 *      // wstawienie menu z pliku style.menu
 *  "Style" OPEN_MENU style.menu
 * "Test" END
 *
 * Je¿eli zamiast polskich fontów s± jakie¶ krzaczki nale¿y wyedetowaæ pliki
 * $HOME/GNUstep/Defaults/WMGLOBAL i $HOME/GNUstep/Defaults/WindowMaker,  
 * lub wej¶æ w menu Konfiguracja.
 * Aby uzyskaæ polskie znaki nale¿y uzupe³niæ definicje fontów.
 * np. zamieniæ
 *
 * SystemFont = "-*-helvetica-medium-r-normal-*-%d-100-*-*-*-*-*-*";
 *
 * na
 *
 * SystemFont = "-*-helvetica-medium-r-normal-*-%d-100-*-*-*-*-iso8859-2";
 *
 * i wszêdzie tam gdzie wystêpuje podobna definicja.
 */
                  

#include "wmmacros"
#define ULUB_EDYTOR vi 
/* Je¶li nie lubisz edytora vi zmieñ na swój ulubiony edytor */
#define ULUB_TERM xterm
/* A tutaj ustaw swój ulubiony terminal */

"WindowMaker" MENU
	"Informacja" MENU
		"Informacja o WMaker..." INFO_PANEL
		"Legalno¶æ..."           LEGAL_PANEL
		"Konsola Systemu"        EXEC xconsole
		"Obci±¿enie Systemu"     EXEC xosview || xload
		"Lista Procesów"         EXEC ULUB_TERM -T "Lista Procesów" -e top
		"Przegl±darka Manuali"   EXEC xman
	"Informacja" END
	
	"Konfiguracja" MENU	
		"Edycja menu"       EXEC ULUB_TERM -T "Edycja menu" -e ULUB_EDYTOR $HOME/GNUstep/Library/WindowMaker/menu
		"Ustawienie fontów" EXEC ULUB_TERM -T "Ustawienie fontów" -e ULUB_EDYTOR $HOME/GNUstep/Defaults/WMGLOBAL
		"Konfiguracja"      EXEC ULUB_TERM -T "Konfiguracja" -e ULUB_EDYTOR $HOME/GNUstep/Defaults/WindowMaker
	"Konfiguracja" END
	
	"Uruchom..." EXEC %a(Uruchom,Wpisz komende do uruchomienia:)
	"Terminal"   EXEC ULUB_TERM -T "Mój ulubiony terminal" -sb 
	"Edytor"     EXEC ULUB_TERM -T "Moj ulubiony edytor" -e ULUB_EDYTOR %a(Edytor,Podaj plik do edycji:)
	"Pulpity"    WORKSPACE_MENU
	
	"Aplikacje" MENU
		"Grafika" MENU
			"Gimp"        EXEC gimp >/dev/null
			"XV"          EXEC xv
			"XFig"        EXEC xfig
			"XPaint"      EXEC xpaint
			"Gnuplot"     EXEC ULUB_TERM -T "GNU plot" -e gnuplot
			"Edytor ikon" EXEC bitmap
		"Grafika" END
		"Tekst" MENU
			"LyX"                 EXEC lyx
  			"Ghostview"           EXEC gv %a(GhostView,Wprowadz nazwe pliku *.ps *.pdf *.no:)
  			"XDvi"                EXEC xdvi %a(XDvi,Wprowadz nazwe pliku *.dvi:)
			"Acrobat"             EXEC /usr/local/Acrobat3/bin/acroread %a(Acrobat,Wprowadz nazwe pliku *.pdf:)
			"Xpdf"                EXEC xpdf %a(Xpdf,Wprowadz nazwe pliku *.pdf:)
			"Arkusz kalkulacyjny" EXEC xspread
		"Tekst" END
		"X File Manager"     EXEC xfm
		"OffiX Files"        EXEC files
		"TkDesk"             EXEC tkdesk
		"Midnight Commander" EXEC ULUB_TERM -T "Midnight Commander" -e mc
		"X Gnu debbuger"     EXEC xxgdb
		"Xwpe"               EXEC xwpe
	"Aplikacje" END
	
	"Internet" MENU
		"Przegl±darki" MENU
			"Netscape" EXEC netscape 
			"Arena"    EXEC arena
			"Lynx"     EXEC ULUB_TERM -e lynx %a(Lynx,Podaj URL:)
		"Przegl±darki" END
		"Programy pocztowe" MENU
			"Pine" EXEC ULUB_TERM -T "Program pocztowy Pine" -e pine 
			"Elm"  EXEC ULUB_TERM -T "Program pocztowy Elm" -e elm
			"Xmh"  EXEC xmh
		"Programy pocztowe" END
		"Emulator terminala" MENU
			"Minicom" EXEC xminicom
			"Seyon"   EXEC seyon
		"Emulator terminala" END
		"Telnet"     EXEC ULUB_TERM -e telnet %a(Telnet,Podaj nazwe hosta:)
		"Ssh"        EXEC ULUB_TERM -e ssh %a(Ssh,Podaj nazwe hosta:)
		"Ftp"        EXEC ULUB_TERM -e ftp %a(Ftp,Podaj nazwe hosta:)
		"Irc"        EXEC ULUB_TERM -e irc %a(Irc,Podaj swoj pseudonim:)
		"Ping"       EXEC ULUB_TERM -e ping %a(Ping,Podaj nazwe hosta:)
		"Talk"       EXEC ULUB_TERM -e talk %a(Talk,Podaj nazwe uzytkownika, z ktorym chcesz nawiazac polaczenie:)
	"Internet" END

	"Editory" MENU
		"XFte"    EXEC xfte
		"XEmacs"  EXEC xemacs || emacs
		"XJed"    EXEC xjed 
		"NEdit"   EXEC nedit
		"Xedit"   EXEC xedit
		"Editres" EXEC editres
		"VI"      EXEC ULUB_TERM -e vi
	"Editory" END
	
	"D¼wiêk" MENU
		"CDPlay"  EXEC workbone
		"Xmcd"    EXEC xmcd 2> /dev/null
		"Xplaycd" EXEC xplaycd
		"Xmixer"  EXEC xmixer
	"D¼wiêk" END
	
    "Gry" MENU
    	"Maze"      EXEC maze
    	"Karty "    EXEC spider
    	"Londownik" EXEC xlander
    	"Szachy "   EXEC xboard
    	"Xeyes"     EXEC xeyes -geometry 51x23
    	"Xmahjongg" EXEC xmahjongg
    	"Xlogo"     EXEC xlogo
    	"Xroach"    EXEC xroach
    	"Xtetris"   EXEC xtetris -color
    	"Xvier"     EXEC xvier
    	"Xgas"      EXEC xgas
    	"Xkobo"     EXEC xkobo
    	"xboing"    EXEC xboing -sound
    	"XBill"     EXEC xbill
    "Gry" END
	
	"U¿ytki" MENU
		"Kalkulator"          EXEC xcalc
		"Zegarek"             EXEC xclock
		"Opcje Okna"          EXEC xprop | xmessage -center -title 'xprop' -file -
		"Przegl±darka Fontów" EXEC xfontsel
		"Szk³o Powiêkszaj±ce" EXEC xmag
		"Mapa Kolorów"        EXEC xcmap
		"XKill"               EXEC xkill
		"Clipboard"           EXEC xclipboard
	"U¿ytki" END

	"Selekcyjne" MENU
		"Kopia"                  EXEC echo '%s' | wxcopy
		"Poczta do ..."          EXEC ULUB_TERM -name mail -T "Pine" -e pine %s
		"Serfuj do ..."          EXEC netscape %s
		"Pobierz Manual ..."     EXEC MANUAL_SEARCH(%s)
		"Po³±cz siê z ..."       EXEC telnet %s
		"Pobierz plik z FTP ..." EXEC ftp %s
	"Selekcyjne" END

	"Ekran" MENU
		"Ukryj Pozosta³e"         HIDE_OTHERS
		"Poka¿ wszystko"          SHOW_ALL
		"Uporz±dkowanie icon"     ARRANGE_ICONS
		"Odswie¿"                 REFRESH
		"Zablokuj"                EXEC xlock -allowroot -usefirst
		"Zachowaj Sesje"          SAVE_SESSION
		"Wyczy¶æ zachowan± sesje" CLEAR_SESSION
	"Ekran" END

	"Wygl±d" MENU
		"Tematy"          OPEN_MENU -noext THEMES_DIR $HOME/GNUstep/Library/WindowMaker/Themes WITH setstyle
		"Style"           OPEN_MENU -noext STYLES_DIR $HOME/GNUstep/Library/WindowMaker/Styles WITH setstyle
		"Ustawienia ikon" OPEN_MENU -noext ICON_SETS_DIR $HOME/GNUstep/Library/WindowMaker/IconSets WITH seticons
		"T³o" MENU
			"Jednolite" MENU
               	"Czarny"            WS_BACK '(solid, black)'
               	"Niebieski"         WS_BACK '(solid, "#505075")'
				"Indigo"            WS_BACK '(solid, "#243e6c")'
				"G³êboko Niebieski" WS_BACK '(solid, "#224477")'
               	"Fioletowy"         WS_BACK '(solid, "#554466")'
               	"Pszeniczny"        WS_BACK '(solid, "wheat4")'
               	"Ciemno Szary"      WS_BACK '(solid, "#333340")'
               	"Winny"             WS_BACK '(solid, "#400020")'
			"Jednolite" END
			"Cieniowane" MENU
				"Zachód S³oñca"         WS_BACK '(mvgradient, deepskyblue4, black, deepskyblue4, tomato4)'
				"Niebo"                 WS_BACK '(vgradient, blue4, white)'
    			"Cieniowany Niebieski"  WS_BACK '(vgradient, "#7080a5", "#101020")'
				"Cieniowane Indigo"     WS_BACK '(vgradient, "#746ebc", "#242e4c")'
			   	"Cieniowany Fioletowy"  WS_BACK '(vgradient, "#654c66", "#151426")'
    			"Cieniowany Pszeniczny" WS_BACK '(vgradient, "#a09060", "#302010")'
    			"Cieniowany Szary"      WS_BACK '(vgradient, "#636380", "#131318")'
    			"Cieniowany Winnny"     WS_BACK '(vgradient, "#600040", "#180010")'
			"Cieniowane" END
			"Obrazki" OPEN_MENU -noext BACKGROUNDS_DIR $HOME/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t
		"T³o" END
		"Zaoamiêtanie Tematu"        EXEC getstyle -t $HOME/GNUstep/Library/WindowMaker/Themes/"%a(Nazwa tematu,Wpisz nazwe pliku:)"
		"Zapamiêtanie Ustawieñ Ikon" EXEC geticonset $HOME/GNUstep/Library/WindowMaker/IconSets/"%a(Ustawienia ikon,wpisz nazwe pliku:)"
	"Wygl±d" END

	"Wyj¶cie" MENU
		"Prze³adowanie"    RESTART
		"Start BlackBox"   RESTART blackbox
		"Start kwm"        RESTART kwm
		"Start IceWM"      RESTART icewm
		"Wyj¶cie..."       EXIT
		"Zabicie sesji..." SHUTDOWN
	"Wyj¶cie" END
"WindowMaker" END
