
			Instrucciones de Instalación para Window Maker



PLATAFORMAS SOPORTADAS
======================
(obs: Alguien me mencionó que lo compiló en...)

- Intel GNU/Linux Conectiva 5.9 (beta)
- Intel GNU/Linux Slackware
- Intel GNU/Linux Debian
- Intel GNU/Linux other distributions
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
- IBM S/390 running Linux
- iBook running Darwin
- Windows NT with Cygwin/XFree86
- Sony PlayStation2 running Linux

Todas las marcas registradas están registradas por sus dueños. (duh)

Los parches que lo hagan funcionar en otras plataformas son bienvenidos.


REQUERIMIENTOS:
==============

El siguiente software se necesita para usar Window Maker:

- X11R6.x	
	Window Maker puede ser compilado en versiones más antiguas de X, 
	como X11R5 (Solaris) o X11R4 (OpenWindows) pero no funcionará 100%
	correctamente.
	En tales servidores no habrá íconos de aplicación y tendrá problemas
	usando el dock. Actualizar las bibliotecas cliente (Xlib, Xt, etc) 
	ayudará si no puede actualizar el servidor.

Lo siguiente se necesita para compilar Window Maker:

- Requerimientos básicos y obvios
	Si compila Window Maker, asegúrese de tener gcc (u algún otro
	compilador ANSI C) y los archivos header de X instalados. Especialmente
	para los usuarios de Linux principiantes: debe instalar todos los paquetes
	de desarrollo de X y la suite gcc. De lo contrario no será capaz de 
	compilar ningún programa X, incluyendo Window Maker.

- libPropList
	Esta biblioteca se puede encontrar en ftp://ftp.windowmaker.org/pub/libs
	o en ftp://ftp.gnome.org/pub/libPropList
	Instálela antes de compilar Window Maker.

- autoconf, automake y libtool
	Estas herramientas NO son necesarias, pero SI tiene una o más de ellas
	instaladas, asegúrese de tener TODO lo siguiente con estas versiones
	 exactas:
	 	
		autoconf 2.13
		automake 1.4
		libtool 1.3
	
	Si tiene una versión distinta, desactívelas temporalemte renombrándolas
	a otras cosa o desinstalándolas desde su sistema. Si no desarrolla
	 software no las necesita, así que puede desinstalarla sin peligro.

- lex (o flex) y yacc (o bison)
	Estas son usadas por libPropList. lex en realidad no se necesita ya que
	su archivo de salida está incluido, pero o yo o automake es tonto,
	causando que el script configure para libPropList simplemente 
	aborte sin ningún motivo si lex no es encontrado.

OPCIONAL:
=========
Estas bibliotecas no son necesarias para hacer que Window Maker funcione,
 pero están soportadas en caso de que quiera usarlas. Los números de versión
 son aquellos que yo tengo (y por lo tanto, garantizo que funciona), pero 
 otras versiones podría funcionar también.

- libXPM 4.7 o más actual.
	Versiones antiguas pueden no funcionar!!!
	Disponible en ftp://sunsite.unc.edu/pub/Linux/libs/X/

	Hay soporte nativo para archivos XPM, pero no cargará imagenes de
	algunos formatos poco comunes.
	
-libpng 0.96 o más actual y zlib
	Para soporte de imágenes PNG.
	http://www.cdrom.com/pub/png/

- libtiff 3.4 o más actual.
	Para soporte de imaen TIFF.
	Puede obtenerla en ftp://ftp.sgi.com/graphics/tiff

- libjpeg 6.0.1 o más actual
	Para soporte de imagen JPEG
	http://www.ijg.org/

- libgif 2.2 o libungif
	Para soporte de imagen GIF.
	ftp://prtr-13.ucsc.edu/pub/libungif/

- libHermes 1.3.2 o más actual
	Para conversión a pixel más rápida en la biblioteca wraster.
	(se usa solo en algunas conversiones - para visuales Color Verdadero)
	http://www.clanlib.org/hermes/

- GNU xgettext
	Si quiere usar mensajes traducidos, necesitará GNU gettext.
	Otras versiones de gettext no son compatibles y no funcionarán.
	Obtenga la versión GNU desde ftp://ftp.gnu.org

OPCIONES DE CONFIGURACIÓN:
=========================
Estas opciones pueden ser pasadas al script configure para activar/desactivar
algunas opciones de Window Maker. Ejemplo:

./configure --enable-kde --enable-gnome

configurará Window Maker para que sea compilado con soporte para KDE y GNOME.

Para obtener una lista de otras opciones, ejecute ./configure --help

--with-libs-from
	especifica rutas adicionales por donde se debe buscar bibliotecas.
	El -L flag debe preceder a cada ruta, tal como:
	
	--with-libs-from="-L/opt/libs -L/usr/local/lib"

--with-incs-from
	especifica rutas adicionales para la búsqueda de archivos header.	
	El parámetro -I debe preceder a cada ruta, tal como:
	
	--with-incs-from="-I/opt/headers -I/usr/local/include"

--enable-single-icon
	activa el agrupamiento de todos los appicons (iconos de aplicaciones)
	del WM_CLASS+WM_INSTANCE en uno solo. Esta característica no es soportada
	por todos los desarrolladores. Si tiene algún problema con ella, contacte
	a su autor:	Christopher Seawood <cls@seawood.org>

--disable-shm
	desactiva el uso de la extensión MIT de memoria compartida. Esto 
	ralentizará la generación de texturas un poco, pero en algunos casos 
	parecerá necesario debido a un error que se manifiesta como íconos y
	texturas desordenadas.

--disable-motif 
	desactiva el soporte para el gestor de ventanas mwm.

--enable-openlook
	activa el soporte para el gestor de ventanas OPEN LOOK(tm)

--enable-gnome
	activa el soporte para el gestor de ventanas GNOME.

--enable-kde
	activa el soporte para el gestor de ventanas kde/kwm.

--enable-lite
	quita cosas que ya están soportadas en los entornos de escritorio, 
	tal como KDE y Gnome. Desactiva cosas como: lista de ventanas, 
	menú de aplicaciones raíz, selección de ventanas múltiple. Note 
	que no podrá salir más desde dentro de Window Maker; tendrá que usar 
	kill con la señal SIGTERM o salir de KDE. No aconsejo activarlo.

--enable-modelock
	Soporte para bloqueo de estado de lenguaje XKB. Si no sabe que es esto
	probablemente no lo necesite.

--enable-sound	
	activa el soporte del módulo para efectos sonoros.

--disable-xpm   
	desactiva el uso de la biblioteca XPM aunque este disponible en su
	 sistema.

--disable-png	
	desactiva el uso de la biblioteca PNG.

--disable-tiff
	desactiva el uso de la biblioteca TIFF.

--disable-gif
	desactiva el uso de la biblioteca GIF.

--disable-jpeg
	desactiva el uso de la biblioteca JPEG.

--disable-shape
	desactiva la forma de ventanas (para oclock, xeyes etc.)

NOTAS ESPECÍFICAS A LA PLATAFORMA:
=================================

- máquinas DEC/Alpha

#>>>>>>>>>>>
From: Anton Ivanov <aivanov@eu.level3.net>
To: Marcelo E. Magallon <mmagallo@debian.org>
Subject: Re: El paquete WindowMaker funciona bien en Alpha?

> Hola,
> 
>  Estuve leyendo algunos documentos en el tarball de WindowMaker,
>  y encontré esto:
> 
>  | - máquinas DEC/Alpha
>  |         podría necesitar pasar el parámetro --disable-shm a configure,
>  |         así ./configure --disable-shm
> 
>  alguien está teniendo problemas con WindowMaker en Alpha?  Puede alguien
>  por favor probar esto?  Debería el parámetro ser pasado al compilar 
>  en Alpha?

Descargo de responsabilidad: alphas bajo mi mando nunca han ejecutado X y are
least likely to suddenly start running it anytime soon.

	Alpha suele tener alguna ridícula poca cantidad de memoria compartida
configurada.

Así que muchísimas aplicaciones suelen to barf. Concretamente - cdrecord, mysql server, etc.
	
	Verifique donde está el suyo en este momento y súbalo a un valor más adecuado catting la entrada
	adecuada de /proc o cambiando el /usr/src/linux/include/asm/shmparam.h.

	De lo contrario la memoria compartida en alpha debiera ser completamente funcional y no veo razón
para desactivarla. Las mías están aumentadas a 32 o más en muchas máquinas.
	
	Y si recuerdo correctamente los comentarios en aquél archivo están en realidad mal.
	El valor no está en bytes, pero si en palabra de tamaño máquina. Para alpha *8.
	
	Como dije - no ejecuto X en ellas asi que apliqué a todas #include "stdisclaimer.h".
#<<<<<<<<<<<<

- SCO Unix - ejecute configure así
	CFLAGS="-belf -DANSICPP" ./configure 

-SunOS, Solaris
	Si tiene gcc instalado, ejecute configure como:
	CPP_PATH="gcc -E -x c" ./configure
	Sun cpp le falta algunas características que necesita Window Maker y
	 puede causarle problemas al analizar sintacticamente los archivos config.
	 También podría necesitar usar el --with-libs-from y --with-incs-from para
	 suministrarle el directorio donde libtiff se aloja.
	Alguien me envió un mensaje diciéndome que también debe hacer que /usr/local/lib 
	sea la primera ruta en LD_LIBRARY_PATH para que funcione.
	
	Si tiene un Ultra Creator 3D o alguna otra máquina con gráficos high-end,
	asegúrese de iniciar el servidor X con el valor visual por defecto a 24bpp o
	 podría experimentar problemas con colores destrozados.
	 Este es un error de wmaker y será reparado.
	
- GNU/Linux en general
	Asegúrese de tener /usr/local/lib en /etc/ld.so.conf y de ejecutar 
	ldconfig después de instalar.
	Desinstale cualquier versión empaquetada de Window Maker antes de 
	instalar una nueva versión.
	
- RedHat GNU/Linux
	Los sistemas RedHat tienen varios problemas molestos. Si lo usa,
	asegúrese de seguir los pasos de más abajo o Window Maker no funcionará:

	* si instaló el Window Maker que viene con RedHat, desinstálelo antes de
	actualizar;
	
	*asegúrese que no tiene las variables de entorno LANG y LINGUAS establecidas a
	 en_RN;
	
	*asegúrese de tener /usr/local/bin en su variable de entorno PATH;

	* asegúrese de tener /usr/local/lib en /etc/ld.so.conf antes de ejecutar 
	ldconfig;
	
	*si tiene problemas que mencionan un menaje de error con --no-reexec 
	desinstale libtool-1.2b e instale libtool-1.3 en su lugar. libtool-1.3 
	se puede encontrar en ftp.gnu.org. También lea la sección TROUBLESHOOTING
	 (PROBLEMAS);
	
	* si instaló el paquete Window Maker desde RedHat y está instalando una
	nueva versión de WM a mano (compilandolo usted mismo), desinstale antes el 
	paquete desde RedHat.
	 
	*asegúrese de tener un enlace simbólico desde /usr/include/X11 hacia 
	/usr/X11R6/include/X11 (si no, tipee ln -s /usr/X11R6/include/X11 /usr/include/X11)
	
	* asegúrese de tener /lib/cpp apuntando al programa cpp.
	
	Si tiene alguna duda en cuanto a hacer algunas de las cosas de arriba, 
	por favor no dude en contactar el soporte para usuarios de RedHat. Ellos
	 responderán amablemente a todas sus preguntas en lo que respecta a su sistema.
	 Ellos también conocen mucho más acerca de su propio sistema que nosotros 
	 (nosotros no usamos RedHat).
	
- PowerPC MkLinux
	Necesitará tener la última version de Xpmac. Las versiones más antiguas
	parecen tener errores que producen el cuelgue del sistema.
	
- Debian GNU/Linux
	Si quiere soporte JPEG y TIFF, asegúrese de tener libtiff-dev y
	 libjpeg-dev instalados.

- SuSE GNU/Linux
	Si instaló el paquete Window Maker desde SuSE, 
	desinstálelo antes de instentar compilar wmaker o podría
	tener problemas.
	

- MetroX (version desconocida)
	MetroX tiene un error que corrompe pixmaps que se establecen como 
	fondos de ventana. Si usa MetroX y tuvo problemas raros con texturas, no use 
	texturas en las barras de títulos. O use un servidor X distinto.

INSTALACIÓN:
=============

Compilando Window Maker
------------------
	Para un comienzo rápido, tipee lo siguiente en el prompt del shell:

	./configure
	make

luego, regístrese como root y tipee:

	make install
	ldconfig

o si quiere remover los símbolos de depuración desde los binarios y hacerlos más
 pequeños, puede tipear:

	make install-strip
	ldconfig

Esto compilará e instalará Window Maker con los parámetros por defecto.

Si quiere personalizar algunas opciones de compilación, puede hacer lo siguiente.
	
	1. (opcional)Mire en la sección OPCIONES DE CONFIGURACIÓN para ver las
	 opciones disponibles. También ejecute:
 
	./configure --help
	
	para obtener un listado completo de otras opciones que están disponibles.

	2. Ejecute configure con las opciones que quiera. Por ejemplo, si quiere 
	usar la opción --enable-kde, tipee:
	
	./configure --enable-kde
	
	3. (opcional) Edite src/wconfig.h con su editor de texto favorito y
	 echele un vistazo por algunas opciones que podría querer cambiar.
	 
	4. Compile. Solo tipee:

	make
	
	5. Regístrese como root (si no puede hacerlo, lea la sección "No tengo la contraseña de root" :-()
	 e instale Window Maker en su sistema:
		su root
		make install


Configuración específica del usuario
------------------------------------

Estas instrucciones no necesitan ser seguidas al actualizar Window Maker 
desde una versión más antigua, a menos que se indique de otra forma en el archivo
 NEWS.

Todo usuario en su sistema que desee ejecutar Window Maker debe hacer lo siguiente:

	1. Instale los archivos de configuración de Window Maker en su directorio home.
Tipee:
	wmaker.inst

	wmaker.inst instalará los archivos de configuración de Window Maker y
configurará X para que automáticamente lance Window Maker al inicio.

	Eso es todo!
	
	Puede tipear "man wmaker" para obtener algo de ayuda general para la 
configuración y otras cosas.
Lea la Guia de Usuario para una explicación más a fondo de Window Maker.

Podría querer dar una mirada a la FAQ también.

Instalando el paquete extras
----------------------------

Desempaquete WindowMaker-extra-<número-de-versión>.tar.gz en /usr/local/share

Puede obtener el archivo en ftp://ftp.windowmmaker.org. Este archivo es optativo
 y solo tiene unos pocos íconos, y temas. Busque el último <número-de-versión> 
 disponible.
 También hay un WindowMaker-extra.readme que le enseña donde vería ir ese paquete.


No tengo la contraseña de root :(
--------------------------------

Si no puede obtener privilegios de superusuario (no puede ser root) puede 
instalar wmaker en su propio directorio home. Para esto, proporcione la 
opción --prefix al ejecutar configure en el paso 2 de compilando Window Maker. 
También necesitará proporcionar la opción --with-appspath, para especificar la ruta
para WPrefs.app. Ejemplo:

./configure --prefix=/home/jshmoe --with-appspath=/home/jshmoe/GNUstep/Apps

Luego haga /home/jshmoe/bin para que se lo incluya en su ruta de búsqueda, agregue
/home/jshmoe/lib a su variable de entorno LD_LIBRARY_PATH y ejecute bin/wmaker.inst

Por supuesto, /home/jshmoe se supone que va a ser reemplazado con la ruta a su 
directorio home real.


ACTUALIZANDO
============
Si está actualizando desde una versión antigua de Window Maker:

	1. Configure y compile Window Maker como siempre.
	2. Instale Window Maker (pero no ejecute wmaker.inst)
	3. Lea el archivo NEWS y actualice sus archivos de configuración, 
	   si es necesario.

PROBLEMAS
=========

Cuando tenga alguno problemas durante la configuración (al ejecutar configure),
tal como no poder usar una biblioteca de formato gráfico que piensa tener instalada,
mire en el archivo config.log para obtener ideas sobre el problema.

== Error al cargar las fuentes, siempre que existan.

Intente recompilar sin el soporte NLS.

== Error al configurar

ltconfig: unrecognized option `--no-reexec'
Try `ltconfig --help' for more information.
configure: error: libtool configure failed

quite la opción --no-reexec desde aclocal.m4 y libPropList/aclocal.m4 y
 reconfigure.
También asegúrese que las versiones de autoconf y automake que tenga instaladas son:

autoconf 2.13
automake 1.4
libtool 1.3

Note que ella no debe ser libtool 1.2b, debe ser libtool 1.3, 
desde los sitios GNU.

== No puedo encontrar proplist.h o libPropList.something

Baje e instale libPropList desde los lugares ya citados en algún lugar 
de este archivo.

== configure no detecta libtiff, u otras bibliotecas gráficas.

Elimine config.cache, luego vuelva a ejecutar configure añadiendo las siguientes
 opciones a configure (entre sus otras opciones):

--with-libs-from="-L/usr/local/lib"
--with-incs-from="-I/usr/local/include -I/usr/local/include/tiff"

Sustituya las rutas donde están localizadas sus bibliotecas gráficas y sus 
correspondientes archivos header. Puede colocar rutas múltiples en cualquiera de 
estas opciones, como se muestra en el ejemplo de --with-incs-from . Solo coloque
un espacio entre ellas.

== configure no detecta libXpm. 

* Verifique si tiene un enlace simbólico desde libXpm.so.4.9 a libXpm.so

== Segmentation fault al inicio.

* Verifique si la versión de libXPM que tiene es por lo menos la 4.7

* Verifique si tiene una versión actualizada de ~/GNUstep/Defaults/WindowMaker

Si no está seguro, intente renombrar ~/GNUstep a ~/GNUtmp y luego ejecute wmaker.inst

== "...: your machine is misconfigured. gethostname() returned (none)"

* el hostname de su máquina está definido a algo inválido, como comenzar con
	un paréntesis. Haga un man hostname para obtener información acerca de como definirlo.


== El menú raíz contiene solo 2 entradas. ("XTerm" y "Exit...")

* Window Maker no está encontrando cpp (el preprocesador de C). Si su 
cpp no está ubicado en /lib/cpp, edite src/config.h y corrija la ruta en 
CPP_PATH.

== checking lex output file root... configure: error: cannot find output from true; giving up

* Lea la sección REQUERIMIENTOS de este archivo.


LOCALES/INTERNACIONALIZACIÓN
============================

Window Maker tiene soporte de idioma nacional. Para activar este soporte, 
debe compilar Window Maker con algunos parámetros adicionales.

0 - Debe tener el paquete GNU gettext instalado. Puede obtenerse en
ftp://prep.ai.mit.edu/pub/gnu/gettext-nnn.tar.gz

Los pasos 1 al 3 pueden saltearse si usa el script Install.

1 - Debe seleccionar los idiomas que quiere soportar. Defina el
LINGUAS a la lista de locales que quiera. El Inglés siempre está
soportado. Ejemplo:

setenv LINGUAS "pt ja de"
en csh

o

export LINGUAS;LINGUAS="pt ja de"
en sh

La lista de locales soportados se pueden encontrar en po/README.
El Inglés es el idioma pr defecto.

Lea po/README si desea traducir y mantener archivos locale para otros 
idiomas.

2 - Adicionalmente, si su idioma usa caracteres multi-byte, tal como
Japonés o Coreano, debe definir la opción MultiByteText a YES
en ~/GNUstep/Defaults/WMGLOBAL

3 - Configure, compile e instale Window Maker normalmente.

4 - Para seleccionar un locale particular en tiempo de ejecución debe definir la
 variable de entorno LANG al locale que quiera. Por ejemplo, si quiere definir
el locale portugués, debe ejecutar

setenv LANG pt

en csh o

export LANG; LANG=pt

en Bourne sh y similares

Nota: Si tiene definida la variable de entorno LC_CTYPE, debe
indefinirla antes de ejecutar wmaker.

Window Maker busca los archivos de definición de menú en el siguiente orden:
(para portugués brasileño, en este caso):

menu.pt_BR
menu.pt
menu

5 - Si elige un idioma que usa caracteres multi-byte, debe configurar 
las fuentes adecuadamente. Lea la página del manual para XCreateFontSet 
para obtener más detalles sobre esto. Debe cambiar el archivo ~/G/D/Windowmaker
para las fuentes usadas en barras de título, menús y otras cosas. Para las fuentes
usadas en ventanas de diálogo, cambie ~/G/D/WMGLOBAL. El %d en los nombres de las
fuentes no debe ser quitado. Puede también usar el script wsetfont proporcionado para
esta tarea. Lea el mismo script para obtener instrucciones.

Por ejemplo, puede especificar lo siguiente en ~/G/D/WindowMaker:

WindowTitleFont = "-*-helvetica-bold-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
MenuTitleFont = "-*-helvetica-bold-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
MenuTextFont = "-*-helvetica-medium-r-normal-*-12-*,-*-*-medium-r-normal-*-14-*";
IconTitleFont = "-*-helvetica-medium-r-normal-*-8-*,-*-*-medium-r-normal-*-12-*";
ClipTitleFont = "-*-helvetica-bold-r-normal-*-10-*,-*-*-medium-r-normal-*-12-*";
DisplayFont = "-*-helvetica-medium-r-normal-*-12-*,-*-*-medium-r-normal-*-12-*";

y en ~/G/D/WMGLOBAL:

SystemFont = "-*-*-medium-r-normal-*-%d-*-*-*-*-*-*-*";
BoldSystemFont = "-*-*-medium-r-normal-*-%d-*-*-*-*-*-*-*";

Las 2 fuentes de arriba son usadas únicamente por aplicaciones que usan WINGs
(WindowMaker y WPrefs.app)

El script wsetfont que se proporciona le permitirá cambiar las definiciones de fuentes
 de una manera fácil. De una mirada al script para detalles de uso.

traducido por Efraín Maximiliano Palermo <max_drake2001@yahoo.com.ar>
