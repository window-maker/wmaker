                Èasto kladené otázky na podporu místního nastavení
                ==================================================

Pokud vám nefunguje podpora I18N, tak zkontrolujte tato nastavení:

        - systémová promìnná LANG musí být nastavená tak, aby odpovídala va¹emu
          místnímu nastavení (èe¹tina = czech), va¹e místní nastavení musí být
          podporovatelné va¹ím operaèním systémem nebo X emulací. V¹echny
          podporovatelné lokalizace zobrazíte pøíkazem "locale -a". Mìli byste
          také zkontrolovat, zda-li je va¹e místní nastavení podporované
          X emulací. Soubor: "/usr/X11R6/lib/X11/locale/locale.alias" by mìl
          obsahovat podobnou sekvenci (podpora pro èe¹tinu):

                cs                              cs_CZ.ISO8859-2
                cs_CS                           cs_CZ.ISO8859-2
                cs_CS.ISO8859-2                 cs_CZ.ISO8859-2
                cs_CZ                           cs_CZ.ISO8859-2
                cs_CZ.iso88592                  cs_CZ.ISO8859-2
                cz                              cz_CZ.ISO8859-2
                cz_CZ                           cz_CZ.ISO8859-2

        - zkontrolujte pou¾ití správných fontù pro va¹e místní nastavení.
          Pokud nepou¾íváte správné fonty s kódováním, které je nastaveno v
          Xlib nebo libc, tak se mù¾ou dít "dost divné vìci". Zkuste explicitnì
          zadat kování do promìnné LANG: LANG=cs_CS.ISO8859-2 nebo
          LANG=cs_CZ.iso88592 a znovu zkontrolujte:
          "/usr/X11R6/lib/X11/locale/locale.alias"

        - pokud vá¹ operaèní systém nepodporuje místní nastavení (locales), nebo
          pokud vá¹ OS nepodporuje místní nastavení pro vá¹ jazyk, mù¾ete
          pou¾ít emulaci místního nastavení X Window Systému. Zprovoznit tuto
          emulaci je mo¾né spu¹tìním ./configure s volbou "--witn-x-locale".
          Pokud pou¾íváte nìjaký komerèní systém jako je napøíklad IRIX, AIX,
          Solaris, ...,tak asi X emulaci nebudete potøebovat. Ov¹em pokud je
          va¹ím operaèním systémem Linux, NetBSD nebo jiný u¾asný, volnì
          ¹iøitelný operaèní systém, tak je mo¾né, ¾e va¹e místní nastavení
          zatím není podporované. Potom pou¾ijte volbu "--witn-x-locale".

          Pozn: Aby jste mohli pou¾ívat X emulaci místních nastavení, tak va¹e
                Xlib musí být zkompilované s touto podporou. Xlib v RedHat 5.0
                tak zkompilované nejsou (RH4.x jsou OK). Pøekompilované Xlib
                s podporou pro emulací místních nastavení pro RH5.0 jsou
                dostupné na adrese:

                ftp://ftp.linux.or.jp/pub/RPM/glibc

        - fonty, které pou¾íváte by mìli být podporované va¹ím místním nastavením.
          Jestli¾e va¹e nastavení fontù v souboru ~/GNUstep/Defaults/WindowMaker
          vypadá takto:

          WindowTitleFont = "-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*";
          MenuTitleFont = "-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*";  ,

          nemù¾ete zobrazovat znaky jiné ne¾ iso8859-x ve fontu helvetica.
          Jednoduchý zpùsob, jak zobrazovat znaky rùzných jazykù, je nastavit v¹echny
          fonty takto:

                "-*-*-medium-r-normal-*-14-*-*-*-*-*-*-*"

          Také je nutné zmìnit nastavení fontù v souborech stylù v adresáøi:
          ~/Library/WindowMaker/Style.

        - pokud si nejste jisti, zda-li má systémová promìnná LC_TYPE správnou
          hodnotu, tak ji nenastavujte.