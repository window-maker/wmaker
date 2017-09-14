#include "wmmacros"

"Achtergrond" MENU
        "Effen" MENU
                "Zwart" WS_BACK '(solid, black)'
                "Blauw"  WS_BACK '(solid, "#505075")'
                "Indigo" WS_BACK '(solid, "#243e6c")'
                "Marineblauw" WS_BACK '(solid, "#224477")'
                "Diepblauw" WS_BACK '(solid, "#180090")'
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
        "Afbeeldingen" MENU
            "Getegeld" OPEN_MENU  BACKGROUNDS_DIR USER_BACKGROUNDS_DIR WITH wmsetbg -u -t
            "Geschaald" OPEN_MENU  BACKGROUNDS_DIR USER_BACKGROUNDS_DIR WITH wmsetbg -u -s
            "Gecentreerd" OPEN_MENU  BACKGROUNDS_DIR USER_BACKGROUNDS_DIR WITH wmsetbg -u -e
            "Gemaximaliseerd" OPEN_MENU  BACKGROUNDS_DIR USER_BACKGROUNDS_DIR WITH wmsetbg -u -a
            "Opgevuld" OPEN_MENU  BACKGROUNDS_DIR USER_BACKGROUNDS_DIR WITH wmsetbg -u -f
        "Afbeeldingen" END
"Achtergrond" END
