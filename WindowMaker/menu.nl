//
// Basis Menu definite voor WindowMaker
//
// De syntax is:
//
// <Titel> <Commando> <Parameters>
//
// <Titel> is een string die zal gebruikt worden als titel. 
//  Er moeten "'s rondstaan als er spaties in zijn
//
// <Commando> een van de volgende geldige commandos: 
//	MENU - start een (sub)menu definitie
//	END  - beeindigd een (sub)menu definitie
//	EXEC <programma> - voert een extern programma uit
//	EXIT - afsluiten windowmanager
//	SHEXEC <command> - executes a shell command (like gimp > /dev/null)
//	RESTART [<windowmanager>] - herstarts WindowMaker of start een andere
//			windowmanager of
//	REFRESH - herteken het bureaublad
//	ARRANGE_ICONS - herschik de iconen op de werkplaats
//	SHUTDOWN - beeindig alle programmas (en sluit de X window sessie)
//	WORKSPACE_MENU - voeg een submenu toe voor het werkplaatsmenu
//	SHOW_ALL - toon alle windows op de werkplaats
//	HIDE_OTHERS - verstop alle windows op de werkplaats, behalve de 
//         het gefocuste window (of het laatst gefocuste)
//		focused one (or the last one that received focus)
//
// <Parameters> is het programma uit te voeren.
//
// Elk MENU vermelding moet een equivalente END vermelding hebben op het einde. 
// Zie voorbeeld:
#include <wmmacros>

"Applicaties" MENU
	"Info" MENU
		"Info Panel..." INFO_PANEL
		"Legal" LEGAL_PANEL
		"Xosview" EXEC xosview
		"Top"	EXEC xterm -e top
		"Handleidingszoeker" EXEC xman
	"Info" END
	"XTerm" SHEXEC xterm -sb || color-xterm -sb || xterm -sb
	"XJed"	EXEC xjed
	"Werkplaatsen" WORKSPACE_MENU
	"Applicaties" MENU
		"Grafische toepassingen" MENU
			"Gimp" EXEC gimp
			"XV" EXEC xv
			"XPaint" EXEC xpaint
			"XFig" EXEC xfig
		"Grafische toepassingen" END
		"X File Manager" EXEC xfm
		"OffiX Files" EXEC files
		"LyX" EXEC lyx
		"Netscape" EXEC netscape
  		"Ghostview" EXEC ghostview
		"Acrobat" EXEC /usr/local/Acrobat3/bin/acroread
  		"TkDesk" EXEC tkdesk
	"Applicaties" END
	"Editors" MENU
		"XEmacs" EXEC xemacs
		"XJed" EXEC xjed
		"NEdit" EXEC nedit
		"Xedit" EXEC xedit
		"VI" EXEC xterm -e vi
	"Editors" END
	"Diverse" MENU
		"Xmcd" SHEXEC xmcd 2> /dev/null
		"Xplaycd" EXEC xplaycd
		"Xmixer" EXEC xmixer
	"Diverse" END
	"Utils" MENU
		"Calculator" EXEC xcalc
		"Font Chooser" EXEC xfontsel
		"Magnify" EXEC xmag
		"Colormap" EXEC xcmap
		"XKill" EXEC xkill
		"ASClock" EXEC asclock
	"Utils" END
	"Werkplaats" MENU
		"Achtergrond" MENU
			"-" 	EXEC CLEARROOT
			"Zwart" WS_BACK '(solid, black)'
			"Blauw"  WS_BACK '(solid, "#505075")'
			"Purpel"  WS_BACK '(solid, "#554466")'
			"Vlas Geel"  WS_BACK '(solid, wheat4)'
			"Donker Grijs"  WS_BACK '(solid, "#333340")'
			"Bordeaux Rood"  WS_BACK '(solid, "#400020")'
		"Achtergrond" END
#if (DEPTH>=8)
// Configureer enkel gradient themas voor newbies en luie mensen
// Je moet herstarten na een gradient gekozen te hebben
#include <gradients.menu>
#endif
		"Verstop andere" HIDE_OTHERS
		"Toon alle" SHOW_ALL
		"Herschik iconen" ARRANGE_ICONS
		"Ververs" REFRESH
		"Blokkeren" EXEC xlock -allowroot -usefirst
		"Opslaan werkplaats" EXEC SAVE_WORKSPACE
	"Werkplaats" END
	"Exit"	MENU
		"Herstart" RESTART
		"Start AfterStep" RESTART afterstep
		"Afsluiten..."  EXIT
		"Afsluiten sessie..." SHUTDOWN
	"Exit" END
"Applicaties" END
