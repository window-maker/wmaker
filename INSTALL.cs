        Instrukce pro instalaci okenního mana¾era Window Maker

PODPOROVANÉ PLATFORMY
=====================
("Podporované" znamená, ¾e u¾ to na dané platformì nìkdo zprovoznil...)

- Intel GNU/Linux Conectiva 5.9 (beta)
- Intel GNU/Linux Slackware
- Intel GNU/Linux Debian
- Intel GNU/Linux ostatní distribuce
- Sparc GNU/Linux RedHat 5.1
- PowerPC GNU/MkLinux
- Alpha GNU/Linux RedHat 5.1
- FreeBSD
- NetBSD
- OpenBSD
- BSDI 4.0
- Solaris 2.5.1, 2.5.2
- Solaris 2.6.0
- Solaris 2.7beta
- SCO Unix
- SGI Irix 5.x, 6.5
- OSF/1
- HP-UX
- AIX 3.2.5
- AIX 4.1.4 / IBM PowerPC
- AIX 4.3.1 / IBM CC compiler
- AIX 4.3.2 / IBM PowerPC
- AIX 5.3
- DEC Alpha/Digital UNIX 4.x
- XFree86 / OS/2
- Corel NetWinder
- SunOS 4.x
- PowerMac / Power MachTen 4.1.1 over MacOS
- Amiga 3000 running RedHat Linux 5.1 (Rawhide)
- IBM S/390 s Linuxem
- iBook s OS Darwin

Patche pro rozbìhání na jiných platformách jsou vítané.

PO®ADAVKY
=========

Následující software je potøebný/nezbytný pro bìh Window Makeru:

- X11R6.x
        Window Maker mù¾e být zkompilován na star¹ích verzích X, jako
        je X11R5 (Solaris) nebo X11R4 (OpenWindows), ale nebude to pracovat
        100% spolehlivì. Na tìchto systémech nebudou fungovat ikony
        aplikací a budete mít problémy s pou¾íváním doku. Upgrade
        u¾ivatelských knihoven (Xlib, Xt atd.) zlep¹í tyto problémy pokud
        nemù¾ete aktualizovat vá¹ X server.

Následující software je potøebný ke zkompilování Window Makeru:

- Zákládní bì¾né vìci
        Kdy¾ budete komplivat Window Maker, tak se ujistìte, ¾e máte gcc
        (nebo jiný ANCI C kompilátor) a nainstalované hlavièkové soubory
        pro X. Hlavnì pro zaèáteèníky v Linuxu: musíte nainstalovat v¹echny
        X-devel balíky a gcc. Jinak nebudete schopni zkompilovat ¾ádný
        program s grafickým rozhraním, tedy ani Window Maker.

- autoconf, automake a libtool
        Tyto nástroje NEJSOU POTØEBA, ale kdy¾ máte jeden nebo dva nebo
        v¹echny tøi nainstalované, tak se UJISTÌTE, ¾e máte pøesnì
        následující VERZE:
                autoconf 2.54
                automake 1.4
                libtool 1.4.2
        Máte-li odli¹nou verzi, tak ji doèasnì pøejmenujte, nebo ji rovnou
        odinstalujte z va¹eho systému. Pokud nebudete programovat, tak ji
        stejnì nebudete potøebovat, tak¾e ji mù¾ete bezpeènì odinstalovat.

Pozn. libProblist u¾ není potøeba ke zkompilování Window Makeru, proto¾e
        libProblist byl pøímo zabudován do WINGs. Z toho dùvodu u¾ není
        potøeba ani lex (flex) a yacc (nebo bison).


NEPOVINNÉ
=========

Tyto knihovny nejsou nutné pro bìh Windo Makeru, ale jsou podporované, kdy¾
je budete chtít pou¾ít. Tyto verze jsou pou¾ívané vývojovým týmem, který
garantuje, ¾e budou fungovat. Ostatní verze by mìli fungovat také.

- libXPM 4.7 nebo novìj¹í
        Star¹í verze nemusí fungovat!!!
        Dostupné na ftp://sunsite.unc.edu/pub/Linux/libs/X/

        Je zde zabudovaná podpora pro soubory XPM, ale nenaète obrázky s
        nestandardním formátem kódování.

- libpng 0.96 nebo novìj¹í a zlib
        Pro podporu PNG obrázkù
        http://www.cdrom.com/pub/png/

- libtiff 3.4 nebo novìj¹í
        Pro podporu TIFF obrázkù
        Dostupné na ftp://ftp.sgi.com/graphics/tiff

- libjpeg 6.0.1 nebo novìj¹í
        Pro podporu JPEG obrázkù
        http://www.ijg.org/

- libgif 2.2 nebo libungif
        Pro podporu GIF obrázkù
        Dostupné na ftp://prtr-13.ucsc.edu/pub/libungif/

-GNU xgettxt
        Kdy¾ chcete pou¾ívat èe¹tinu nebo jiné jazyky kromì angliètiny,
        tak potøebujete GNU xgettext.
        Ostatní verze nejsou kompatibilní a nebudou fungovat
        GNU verze je dostupná na ftp://ftp.gnu.org


KONFIGURAÈNÍ NASTAVENÍ
======================

Tyto volby mohou být pøedané konfiguraènímu skriptu jako argumenty za
úèelem povolení/zakázání urèité vlastnosti Window Makera.

Pøíklad:

./configura --enable-kde --enable-gnome

nakonfiguruje Window Maker s podporou KDE a GNOME

Seznam ostatních mo¾ností dostanete spu¹tìním ./configure --help

--with-libs-from
        specifikuje, které dal¹í adresáøe s knihovnami se mají 
        prohledávat. Øetìzec -L musí pøedcházet ka¾dému adresáøi,
        napø:
        --with-libs-from="-L/opt/libs -L/usr/local/lib"

--with-incs-from
        urèuje, které dal¹í adresáøe s hlavièkovými soubory se mají
        prohledat. Øetìzec -I musí pøedcházet ka¾dému adresáøi,
        napø:
        --with-incs-from="-I/opt/headers -I/usr/local/include"

--enable-single-icon
        umo¾òuje skrýt v¹echny ikony aplikací WM_CLASS+WM_INSTANCE
        do jedné jediné. Tato vlastnost není podporovaná ze strany vývojáøù.
        Kdy¾ budete mít s touto funkcí problémy, tak se obra»te na jejího
        autora: Christopher Seawood <cls@seawood.org>

--disable-shm
        zaká¾e pou¾ívání zdílené pamìti MIT. To trochu zpomalí generování
        textur, ale v nìkterých pøípadech se to jeví jako nezbytnost z dùvodu
        chyby, která zpùsobuje ¹patné zobrazování ikon a textur.

--disable-motif
        zaká¾e podporu pokynù pro okenní mana¾er mwm.

--enable-lite
        odstraní nástroje a funkce, které jsou u¾ dostupné v desktopových
        prosøedích KDE a GNOME. Odstraní se: seznam oken, menu aplikací,
        výbìr více oken. Uvìdomte si, ¾e takto u¾ nebudete schopni ukonèit
        samotný Window Maker pomocí aplikaèního menu. Budete muset zabít Window
        Maker z textové konzole nebo ho ukonèit z KDE nebo GNOME.
        Nedoporuèuje se povolit tuto vlastnost.

--enable-modelock
        podpora pro XKB nastavení jazyka. Kdy¾ nevíte, o co jde, tak to
        pravdìpodobnì nebudete potøebovat.

--enable-sound
        povolí podporu modulu zvukových efektù.

--disable-xpm
        zaká¾e podporu knihovny XPM, pokud je dostupná na va¹em systému.

--disable-png
        zaká¾e podporu knihovny PNG.

--disable-tiff
        zaká¾e podporu knihovny TIFF.

--disable-gif
        zaká¾e podporu knihovny GIF.

--disable-jpeg
        zaká¾e podporu knihovny JPEG.

--disable-shape
        zaká¾e tvarovaná okna (pro oclock, xeyes, atd.).

PO®ADAVKY PRO SPECIFICKÉ PLATFORMY
==================================

-DEC/Alpha

        Následující mail není pøelo¾en, ale hovoøí se v nìm, ¾e pravdìpodobnì
        není potøeba konfigurovat Window Maker na Alphì s volbou --disable-shm.

-------------------------------------------------------------------------------
        From: Anton Ivanov <aivanov@eu.level3.net>
        To: Marcelo E. Magallon <mmagallo@debian.org>
        Subject: Re: Is the WindowMaker package working ok on Alpha?

> Hi,
> 
>  I was reading some docs on the WindowMaker tarball, and found this:
> 
>  | - DEC/Alpha machines
>  |         You might need to pass the --disable-shm flag to configure,
>  |         like ./configure --disable-shm
> 
>  is anyone having problems with WindowMaker on Alpha?  Can someone
>  please test this?  Should the flag be passed when building on Alpha?

Disclaimer: alphas under my command have never run X and are least likely to 
suddenly start running it anytime soon.

        Alpha used to have some ridiculously low amount of shared memory
configured. 
So quite a lot of apps used to barf. Namely - cdrecord, mysql server, etc.

        Check where is yours at the moment and raise it to a more appropriate
value by either catting to the appropriate /proc entry or changing the
/usr/src/linux/include/asm/shmparam.h.

        Otherwise the shared memory on alpha should be fully functional and I
see no reason to disable it. Mine are bumped up to 32 or more on most
machines.

        And If I recall correctly the comments in that file are actually
wrong. Value is not bytes, but in machine size word. For alpha *8.

        As I said - I do not run X on them so all #include "stdisclaimer.h"
apply.

-------------------------------------------------------------------------------

- SCO Unix - configure spus»te takto:
        CFLAGS="-belf -DANSICPP" ./configure

- SunOS, Solaris
        Kdy¾ máte naistalovaný gcc, tak configure spus»te takto:
        CPP_PATH="gcc -E -x c" ./configure
        Preprocesoru cpp od Sunu chybìjí nìkteré vlastnosti, které Window Maker
        potøebuje, a to mù¾e zpùsobit problémy bìhem parsování konfiguraèních
        souborù. Je mo¾né, ¾e budete muset pou¾ít --with-libs-from a
        --with-incs-from k nahrazení adresáøe s libtiff.
        Nìkdo doporuèuje pou¾ít /usr/local/lib jako první cestu v LD_LIBRARY_PATH.

        Kdy¾ máte Ultra Creator 3D nebo jinou high-end grafickou kartu, tak se
        ujistìte, ¾e X server startuje s 24 bitovou barevnou hloubkou, jinak
        mù¾ete mít problémy s pomícháním barev. Toto je chyba Window Makeru,
        která bude odstranìna.

- GNU/Linux obecnì
        Ujistìte se, ¾e soubor /etc/ld.so.conf obsahuje øádek "/usr/local/lib".
        Odinstalujte jakoukoliv star¹í verzi balíèku Window Makeru pøed instalací
        novìj¹í verze.
        Nezapomeòte spustit ldconfig po instalaci Window Makeru.

- RedHat GNU/Linux
        RedHat má nìkolik obtì¾ujících chyb. Pokud ho pou¾íváte dodr¾ujte nìkolik
        následujících krokù, jinak vám Window Maker nebude fungovat.

        * pokud máte nainstalovaný Window Maker z distribuce RedHat, tak ho pøed
        upgradem odinstalujte

        * ujistìte se, ¾e nemáte systémové promìnné LANG a LINGUAS nastavené
        na en_RN

        * ujistìte se, ¾e va¹e systémová promìnná PATH obsahuje polo¾ku
        /usr/local/bin

        * ujistìte se, ¾e soubor /etc/ld.so.conf obsahuje øádek "/usr/local/lib"
        pøed tím, ne¾ spustíte ldconfig (na konci instalace)

        * pokud máte problémy, které se projevují chybovou hlá¹kou, která
        obsahuje text --no-reexec, tak odinstalujte libtool-1.2b a nainstalujte
        si novìj¹í verzi libtool-1.3. libtool-1.3 mù¾ete nalézt na adrese:
        ftp.gnu.org  .    Pøeètìte si také odstavec ØE©ENÍ PROBLÉMÚ.

        * pokud máte nainstalovaný balíèek Window Maker od RedHatu a nyní
        instalujete novou verzi "ruènì" (kompilováním zdrojových kódù), tak
        nejprve odinstalujte stávající balíèek.

        * ujistìte se, ¾e se na va¹em systému nachází symbolický link z
        /usr/include/X11 na /usr/X11R6/include/X11. Pokud tomu tak není, tak
        na pøíkazovou øádku napi¹te:

         ln -s /usr/X11R6/include/X11 /usr/include/X11

        * ujistìte se, ¾e máte symbolický link /lib/cpp ukazující na cpp
        program

        Pokud pochybujete o nìjakém z pøedcházejích krokù, tak neváhejte
        kontaktovat u¾ivatelskou podporu distribuce RedHat. Oni vám laskavì
        zodpoví v¹echny va¹e dotazy zohledòující vá¹ systém. Oni vìdí
        mnohem více o va¹em (jejich) systému ne¾ my (my nepou¾íváme
        RedHat).

- PowerPC MkLinux
        Budete potøebovat poslední verzi Xpma. Star¹í verze pravdìpodobnì 
        obsahují chybu, která zpùsobuje zamrznutí systému.

- Debian GNU/Linux
        Pokud chcete podporu JPEG a TIFF, tak se ujistìte, ¾e máte nainstalovány
        balíèky libtiff-dev a libjpeg-dev.

- SuSE GNU/Linux
        Pokud máte ji¾ nainstalován balièek Window Maker od SuSE, tak ho
        odstraòte ne¾ se pustíte do kompilace Window Makeru. kdy¾ tak
        neuèiníte, budete mít problémy.

- MetroX (neznámá verze)
        Metrox obsahuje chybu, která zapøièiòuje, ¾e obrázky, které jsou
        nastaveny jako pozadí, jsou po¹kozeny. Pokud pou¾íváte Metrox a
        máte podivné problémy s texturami, tak nepou¾ívejte textury v
        titulcích nebo pou¾ívejte jiný X server.

INSTALACE
=========

Nejjednodu¹¹í zpùsob, jak nainstalovat Window Maker, je spustit skript
Install nebo Install.cs (ten s vámi komunikuje èesky). Oba skripty
vás jednodu¹e provedou celou instalací.

        su
        Install.cs

Poznámka: tato metoda neposkytuje v¹echny mo¾nosti konfigurace a pokud
          se setkáte s nìjakými problémy, tak stejnì budete muset
          pou¾ít následující zpùsob kompilace.

Kompilace Window Makeru
-----------------------
        Pro osvìdèený zpùsob zadejte následující pøíkazy v shellu:

        ./configure
        make

pak se pøihla¹te jako root a zadejte:

        make install
        ldconfig

pokud nejste programátor a nebudete ladit Window Maker, tak se mù¾ete zbavit
ladících symbolù v binárních souborech a zmen¹it jejich velikost:

        make install-strip
        ldconfig

Takto zkompilujete a nainstalujete Window Maker se standartními parametry.

Pokud chcete upravit nìkteré vlastnosti, tak musíte uèinit následující:

        1. (volitelné) Podívejte se na èást MO®NOSTI KONFIGURACE, kde jsou
        uvedené mo¾nosti konfigurace. Nebo spus»te:

        ./configure --help

        tak dostanete kompletní seznam v¹ech dostupných mo¾ností konfigurace.

        2. Spus»te ./configure s mo¾ností, kterou jste si vybrali. Napøíklad,
        pokud chcete pou¾ít mo¾nost --enable-kde, tak zadejte:

        ./configure --enable-kde

        3. (volitelné) Otevøete soubor ./src/wconfig.h va¹ím oblíbeným editorem
        a upravte nìkteré mo¾nosti, které si pøejete zmìnit.

        4. Kompilace. Zadejte pouze:

        make

        5. Pøihlaste se jako root (pokud tak nemù¾etet uèinit, tak si pøeètìte
        èást "Nemám rootovské helso :-(") a nainstalujte Window Maker na vá¹
        systém:

        su root
        make install

Nastavení specifické pro u¾ivatele
----------------------------------

Tyto instrukce nejsou povinné, pokud upgradujete Window Maker ze star¹í
verze na novìj¹í a pokud není uvedeno jinak v souboru NEWS.

Ka¾dý u¾ivatel na va¹em systému, který si pøeje pou¾ívat Window Maker musí
udìlat následující:

        1. Nainsatlovat konfiguraèní soubory Window Makeru do jeho domovského
        adresáøe:

        wmaker.inst

        wmaker.inst nainstaluje konfiguraèní soubory a nastaví X server tak,
        aby automaticky spou¹tìl Window Maker pøi jejich spu¹tìní.

        To je v¹echno, pøátelé.

        Informace o konfiguraci a spoustì ostatních vìcí vám poskytne:

        man wmaker

Pro hlub¹í úvod do Window Makeru si pøeètìte U¾ivatelskou pøíruèku (User Guide).

Mìli byste se také podívat na FAQ (Frequently Asked Questions = èasto kladené 
otázky), pokud budete mít nìjaké problémy/potí¾e jak pøi samotné kompilaci,
tak s u¾íváním Window Makeru.

Instalování speciálního balíèku
-------------------------------

Rozbalte WindowMaker-extra-<èíslo_verze>.tar.gz v adresáøi /usr/local/share

Tento soubor si mù¾ete sehnat na adrese: ftp://ftp.windowmaker.org. Instalace
tohoto souboru vùbec není nutná. tento balíèek obsahuje nìkolik ikon a témat.
Hledejte poslední dostupnou verzi. V balíèku také naleznete soubor
WindowMaker-extra.readme, který vám poradí, co máte udìlat.

Nemám rootovské helso :-(
-------------------------

Pokud nemáte superu¾ivatelská privilegia (nemù¾ete se pøihlásit jako root),
tak nezoufejte, proto¾e mù¾ete nainstalovat Window Maker do va¹eho vlastního
domovského adresáøe. K tomu musíte pou¾ít volbu --prefix pøi spu¹tìní
./configure. Také musíte pou¾ít volbu --with-appspath, která specifikuje
cestu pro WPrefs.app.
Pøíklad:

./configure --prefix=/home/karel --with-appspath=/home/karel/GNUstep/Applications

Potom pøidejte adresáø /home/karel/bin do systémové promìnné PATH a adresáø
/home/karel/lib do sytémové promìnné LD_LIBRARY_PATH a nakonec spus»te
~/bin/wmaker.inst.

Samozøejmì, ¾e adresáø /home/karel bude ve va¹em pøípadì nahrazen va¹ím
vlastním domovským adresáøem.


UPGRADE
=======

Pokud upgradujete ze star¹í verze Window Makeru:

        1. Nakonfigurujte a zkompilujte Window Maker jako obvykle.
        2. Nainstalujte Window Maker (ale nespou¹tìjte wmaker.inst).
        3. Pøeètìte si soubor NEWS a aktualizujte va¹e konfiguraèní soubory,
           pokud je to nezbytné.

ØE©ENÍ PROBLÉMÚ
===============

Pokud máte nìjaké problémy bìhem konfigurace (kdy¾ bì¾í configure), jako
napøíklad neschopnost pou¾ít knihovnu grafického formátu, o kterém víte, ¾e
ho máte nainstalován, tak se podívejte do souboru config.log. Mo¾ná zde
najdete pøíèinu svých problémù.

== Error with loading fonts, even if they exist.

Zkuste kompilaci bez podpory NLS (národního prostøedí).

== Error when configuring

ltconfig: unrecognized option `--no-reexec'
Try `ltconfig --help' for more information.
configure: error: libtool configure failed

odstraòte volbu --no-reexec ze souboru aclocal.mc a spus»te znovu configure.
Také zkontrolujte, zda verze autoconfu a automaku, které máte nainsatlované
odpovídají ní¾e uvedeným hodnotám:
autoconf 2.13
automake 1.4
libtool 1.3

Poznámka: nemù¾e to být libtool 1.2b, ale musí to být libtool 1.3 ze stránek
GNU.

== configure doesn't detect libtiff, or other graphic libraries.

Sma¾te soubor config.cache, pak znovu spus»te configure s následujícími
volbami (a jinými, které pou¾íváte):
--with-libs-from="-L/usr/local/lib"
--with-incs-from="-I/usr/local/include -I/usr/local/include/tiff"
Na pøíslu¹ná místa vlo¾te cesty k va¹ím grafickým knihovnám a odpovídajícím
hlavièkovým souborùm. Mù¾ete zadat nìkolik cest do tìchto voleb, jako je to
ve vý¹e uvedeném pøíkladì --with-incs-from. Jenom je nezapomeòte oddìlit
mezerou.

== configure doesn't detect libXpm.

* Zkontrolujte, jestli máte symbolický link ze souboru libXpm.so.4.9 na
  soubor libXpm.so

== Segmentation fault on startup

* Zkontrolujte, jestli verze knihovny libXPM je vy¹¹í ne¾ 4.7

* Zkontrolujte, zda-li máte upravenou verzi ~/GNUstep/Defaults/WindowMaker
  Pokud si nejste jisti, tak zkuste pøejmenovat ~/GNUstep na ~/GNUtmp a
  spus»te wmaker.inst

== "...: your machine is misconfigured. gethostname() returned (none)"

* Jméno va¹eho poèítaèe je nastaveno nesprávnì, proto¾e zaèíná uvozovkami.
  Spus»te man hostname a zde se dozvíte, jak zmìnit jméno va¹eho poèítaèe.

== The root menu contains only 2 entries. ("XTerm" and "Exit...")

* Window Maker nemù¾e nalézt cpp (preprocesor jazyka C). Pokud se vá¹ cpp
  nenachází v /lib/cpp, tak otevøte soubor src/config.h a nastavte správnì
  cestu CPP_PATH.

== checking lex output file root... configure: error: cannot find output from true; giving up

* Pøeètìte si odstavec PO®ADAVKY na zaèátku tohoto souboru.

MÍSTNÍ NASTAVENÍ / INTERNACIONALIZACE
=====================================

Window Maker má podporu národních jazykù. Aby jste ji povolili, tak
musíte zkompilovat Window Maker s nìkolika dal¹ími parametry.
Seznam podporovaných jazykù naleznete v souboru ./po/README.

0 - Musíte mít nainstalovaný balíèek GNU gettextu. Tento balíèek
    mù¾ete nalézt na ftp://prep.ai.mit.edu/pub/gnu/gettext-nnn.tar.gz

Kroky 1,2 a 3 mù¾ete vynechat, pokud pou¾íváte skript Install.cs nebo
skript Install.

1 - Vyberte si jazyky, které budete chtít pou¾ívat. Zadejte seznam
    tìchto jazykù do systémové promìnné LINGUAS. Angliètina je
    podporována v¾dy. Pøíklady

    C - shell:

        setenv LINGUAS "pt ja de"

    Bash(sh):

        export LINGUAS
        LINGUAS="pt ja de"

    Pøeètìte si soubor po/README pokud chcete pøelo¾it Window Maker
    pro nìjaký dal¹í jazyk.

2 - Navíc, pokud vá¹ jazyk pou¾ívá multi-byte znaky, napøíklad Japon¹tina
    nebo Korej¹tina, tak musíte nastavit volbu the MultiByteText na YES
    v souboru ~/GNUstep/Defaults/WMGLOBAL

3 - Nakonfigurujte, zkompilujte a nainstalujte Window Maker jako obvykle.

4 - Národní prostøedí si mù¾ete zmìnit i za bìhu Window Makeru. Systémovou
    promìnnou LANG nastavíte na vámi zvolenou hodnotu. Napøíklad, pokud
    chcete pou¾ít portugalské prostøedí, musíte spustit:

    C - shell:

        setenv LANG pt

    Bourne shell a podobné:

        export LANG
        LANG=pt

    Nakonec musíte restartovat/spustit Window Maker.

    Poznámka: Kdy¾ máte nastavenou systémovou promìnnou LC_TYPE, tak jí
    musíte zru¹it pøed spu¹tìním Window Makeru.

    Window Maker hledá soubory s definicí menu v tomto poøadí (pro
    brazilskou portugal¹tinu):

        menu.pt_BR
        menu.pt
        menu

5 - Pokud si vyberete jazyk, který pou¾ívá multi-byte znaky, tak musíte
    pøíslu¹ným zpùsobem nastavit fonty. Pøeètìte si manuálové stránky
    o XCreateFontSet k získání vìt¹ího mno¾ství informací o daném
    problému. Musíte zmìnit v souboru ~/GNUstep/Default/WindowMaker
    nastavení fontù pro titulky, menu, atd. Fonty pro dialogy nastavíte
    v souboru ~/GNUstep/Default/WMGLOBAL. Øetìzce %d ve jménech fontù
    nemusí být odstranìné. Také mù¾ete pou¾ít skript wsetfont, který
    toto v¹e uèiní za vás. Pokud se ho rozhodnote pou¾ít, tak si k nìmu
    pøeètìte instrukce tak, ¾e spustíte wsetfont bez argumentù.

    Pøíklad èásti souboru ~/GNUstep/Default/WindowMaker:

        WindowTitleFont = "-*-helvetica-bold-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
        MenuTitleFont = "-*-helvetica-bold-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
        MenuTextFont = "-*-helvetica-medium-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
        IconTitleFont = "-*-helvetica-medium-r-normal-*-8-*,-*-*-medium-r-normal-*-12-*";
        ClipTitleFont = "-*-helvetica-bold-r-normal-*-10-*,-*-*-medium-r-normal-*-12-*";
        DisplayFont = "-*-helvetica-medium-r-normal-*-12-*,-*-*-medium-r-normal-*-12-*";

    a souboru ~/GNUstep/Default/WMGLOBAL:

        SystemFont = "-*-*-medium-r-normal-*-%d-*-*-*-*-*-*-*";
        BoldSystemFont = "-*-*-medium-r-normal-*-%d-*-*-*-*-*-*-*";

    Tyto dva fonty jsou pou¾ívány v aplikacích, které pou¾ívají WINGs (WindowMaker a
    WPrefs.app).

    Skript wsetfont vám umo¾ní nastavit fonty mnohem jednodu¹ím zpùsobem. Podívejte
    se na jeho manuálové stránku k získání bli¾¹ích informací.