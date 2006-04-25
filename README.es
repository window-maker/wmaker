


		       		GNU Window Maker
		       	       X11 Window Manager

	  	   	   <http://windowmaker.info>
		 	 <ftp://ftp.windowmaker.info>

				     por

		      	     Alfredo K. Kojima 

			 	 Dan Pascu

				    ]d


			       Web/FTP Master

			       Phillip Smith



		¡Felicitaciones! Ha adquirido un dispositivo
	excelentísimo que le proporcionará miles de años de uso sin
	problemas, si no fuera porque indudablemente lo destruirá a
	través de alguna maniobra estúpida típica de consumidor. Por
	eso le pedimos por EL AMOR DE DIOS LEA ESTE MANUAL DEL
	PROPIETARIO CUIDADOSAMENTE ANTES DE DESEMPAQUETAR EL
	DISPOSITIVO. ¿YA LO DESEMPAQUETÓ, NO?  LO DESEMPAQUETÓ Y LO
	ENCHUFÓ Y LO ENCENDIÓ Y TOQUETEÓ LAS PERILLAS, Y AHORA SU
	CHICO, EL MISMO CHICO QUE UNA VEZ METIÓ UNA SALCHICHA EN SU
	VIDEOCASETERA Y ACCIONÓ "AVANCE RÁPIDO", ESTE CHICO
	TAMBIÉN ESTÁ TUOQUETEANDO LAS PERILLAS, ¿CIERTO? Y RECIÉN
	AHORA ESTÁ COMENZANDO A LEER LAS INSTRUCCIONES, ¿¿¿CIERTO???
	NOSOTROS PODRÍAMOS SIMPLEMENTE ROMPER ESTOS DISPOSITIVOS EN LA
	FÁBRICA ANTES DE DESPACHARLOS, ¿SABE?
				-- Dave	Barry, "¡Lea Esto Primero!"


Descripción
===========

Window Maker es el gestor de ventanas GNU para el Sistema de Ventanas X. Fue
diseñado para emular la apariencia y funcionalidad de parte del GUI de NEXTSTEP(mr).
Procura ser relativamente rápido y pequeño, rico en características, fácil de configurar
y usar, con una simple y elegante apariencia sacada desde NEXTSTEP(mr).

Window Maker fue diseñado teniendo en mente la integración con GNUstep y por eso
es el gestor de ventanas "oficial". Es también parte del proyecto GNU (www.gnu.org)
. Lea mas sobre GNUstep más adelante en este archivo.


Pistas (información dada por las aplicaciones para integrarse bien con el gestor de
ventanas) para Motif(tm), OPEN LOOK(tm), KDE y GNOME también son soportados. 
Entonces puede reemplazar cualquiera de los gestores de ventana para estos entornos
con Window Maker manteniendo la mayoría, si no todo, de la funcionalidad del
gestor de ventanas nativo.

Window Maker antes se llamaba WindowMaker.

Window Maker no tiene relación con Windowmaker, el software para
hacer ventanas y puertas.


Documentación
=============

Lea antes de preguntar.

* Los archivos README distribuidos por todas partes del árbol de fuentes 
contienen información relacionada al contenido en los directorios. 

* INSTALL tiene instrucciones de instalación y algunos consejos cuando tenga
algún problema. Significa que debería leerlo antes de la instalación.
No fue escrito solo para ocupar espacio en el paquete...

* FAQ: Preguntas Frecuentes. LEALO!!! FAQ.I18N es para
preguntas relacionadas con la internacionalización.

* NEWS: lista los cambios visibles por el usuario desde la versión anterior. 
Léalo si está actualizando.

* MIRRORS: algunos lugares alternativos donde puede obtener Window Maker, 
incluyendo paquetes de Window Maker específicos para ciertas plataformas.

* BUGFORM: uselo para enviar reportes de errores. Por favor uselo.

* ChangeLog: ¿que cambió desde la versión anterior?

* BUGS: lista de errores conocidos.

*** Tutorial

Hay un tutorial mantenido por Georges Tarbouriech en:

http://www.linuxfocus.org/~georges.t/

*** Guía del Usuario

La Guía del Usuario de Window Maker puede ser bajada desde el ftp oficial
o por sitios web.
Puede también ser vista en formato HTML en:

http://people.delphi.com/crc3419/WMUserGuide/index.htm

La Guía del Usuario explica como usar Window Maker, los archivos de configuración 
y opciones.

*** man pages

Tipee "man wmaker" en el prompt del shell para obtener ayuda general sobre Window Maker.


Directorios y Archivos en el Árbol de Fuentes
=============================================

* Install es un script para configurar y compilar Window Maker de una forma
fácil (no es que la forma normal sea difícil, pero...).

* AUTORES: los créditos

* TODO: planes para el futuro.

* contrib/ tiene algunos parches aportados que no están soportados por Window Maker
  porque entran en conflicto con la filosofía de diseño de los desarrolladores o por
  alguna otra razón.

* util/ tiene varios programas utilitarios.

* WPrefs.app/ es el programa de configuración.

* src/wconfig.h posee opciones de compilación que puede cambiar para 
  seleccionar algunas opciones/caracteristicas y otras cosas.

* WINGs/ biblioteca widget para imitación de NEXTSTEP.

* wrlib/ biblioteca de procesamiento de imagen.

* po/ posee catálogos de mensajes  que son las versiones traducidas de los mensajes
mostrados por Window Maker.

* docklib-x.x.tar.gz es una biblioteca para escribir dockapps.

SOCORRO!!!
==========

Hay una lista de correo para discutir sobre Window Maker en
wm-user@windowmaker.info. Para suscribirse, envie un mensaje que contenga:

	subscribe
en el tema del mensaje a wm-user-request@windowmaker.info

Si tiene algun problema, pregunte aquí (después de leer los docs, por supuesto). Es
más probable que las personas de la lista sepan contestar sus preguntas
que nosotros. Para reportes de errores use el BUGFORM.

Si tiene un problema con una versión precompilada de Window Maker 
(rpm, deb etc), primero pregunte a la persona que hizo el paquete.

NOTA IMPORTANTE: cuando pida ayuda (en la lista de correo o a los desarrolladores,
directamente) *siempre* envie información sobre el sistema que está usando. Puede
usar la sección de información del sistema al final del BUGFORM como una guía. 
Otra cosa: por favor no envie correo HTML.

También hay un canal de IRC #windowmaker en openprojects. Únase aquí, 
conecte su cliente de irc a irc.openprojects.net, irc.linux.com o algún otro
servidor de esa red.

GNUstep
=======

GNUStep es un completo sistema de desarrollo orientado a objetos, basado en la 
especificación OpenStep liberada por NeXT(tm) (ahora Apple(tm) y Sun(tm)). Ello
proveerá todo lo que se necesita para producir aplicaciones multiplataforma, 
orientadas a objetos, gráficas (y no gráficas); suministrando dentro de otras cosas,
bibliotecas base del sistema, una estructura de alto nivel para aplicaciones GUI que
usan un modelo de imagenes de tipo Display PostScript(tm) (DGS), objetos para acceso
a bases de datos relacionales, objetos distribuidos y un entorno de desarrollo gráfico,
con herramientas como un modelador de interfaces, un sistema para administración del 
proyecto (central de proyecto) y otras herramientas.

El sistema de desarrollo de GNUStep será usado para crear un entorno de usuario,
con todo lo necesario para una completa interface gráfica de usuario, tal como 
un visualizador de archivos, editores de texto y otras aplicaciones. Note que el
entorno de usuario (o "entorno de escritorio") es solo un pequeña parte de todo
el proyecto GNUStep y por lo tanto no "compite" con otros proyectos como KDE o GNOME,
simplemente porque son cosas completamente diferentes.

Para más información sobre el proyecto GNUStep, visite: http://www.gnustep.org y
http://gnustep.current.nu


Ejecutando multiples instancias de Window Maker
===============================================

No es una buena idea eejcutar más de una instancia de Window Maker desde
el mismo usuario (ya que wmaker usará los mismos archivos de configuración)
al mismo tiempo. Podría obtener un comportamiento inesperado cuando Window
Maker actualiza sus archivos de configuración.

Si de verdad desea hacer esto, intente ejecutar Window Maker con la opción
de linea de comando --static ya que así no actualizará o cambiará ninguno de los
archivos de configuración.

Soporte para Sonido
===================

El sonido es soportado por los sistemas Linux y FreeBSD con el uso de 
un módulo distribuido separadamente llamado WSoundServer. Hay también 
una herramienta de configuracion gráfica para definir sus sonidos llamada
WSoundPref.
Puede bajar esto en:
http://shadowmere.student.utwente.nl/

Note que debe compilar Window Maker con el parámetro --enable-sound
y definir la opción DisableSound a NO.


Ajuste de Rendimiento.
=====================
Si quiere disminuir el uso de memoria por parte de Window Maker y mejorar el 
rendimiento, manteniendo una linda apariencia y buena funcionalidad, siga los
items de abajo:

- use texturas sólidas para todo, principalmente barras de título y menúes.
  Si quiere un escritorio de aspecto lindo, use el estilo Tradicional.
- Apague NewStyle y Superfluous
- No una muchos atajos al menú y mantenga solo los items esenciales en el menú.
- Active DisableClip
- edite wconfig.h y desactive el NUMLOCK_HACK y lo mismo con las características
  que no use (tenga en mente que algunos de los #defines podrían no funcionar,
  ya que ellos no están completamente soportados). Asegúrese de mantener siempre
  Numlock y ScrollLock apagados.
- Active DisableAnimations. Puede también #undefine ANIMATIONS en wconfig.h
- quite las entradas por defecto IconPath y PixmapPath para contener solo las
  rutas que en verdad tiene en su sistema.
- no use imágenes grandes en el fondo raíz.
- quite el soporte para los formatos de imagen que no use.
- para reducir el uso de la memoria, desactive el caché de ícono, definiendo 
  la variable de entorno RIMAGE_CACHE a 0. Si quiere aumentar el rendimiento 
  a expensas del uso de la memoria, defina este valor a un valor igual al 
  número de íconos distintos que use.

Control del Mouse por Teclado
=============================

Muchas personas preguntan sobre agregar control por teclado al mouse, como
en fvwm, pero Window Maker no tendrá tal característica. La extensión XKB 
soporta simulación de mouse desde el teclado, de una manera mucho más poderosa
que cualquier otra simulación hecha por un administrador de ventanas.

Para activarlo, presione la combinación de teclas Control+Shift+NumLock o 
Shift+NumLock. Debiera escuchar el beep del parlante. Para desactivarlo,
haga lo mismo.

Para controlar el mouse el teclado numérico se usa así:
- 4 (flecha izquierda), 7 (Home), 8 (flecha arriba), 9 (PgUP), 6 (flecha derecha),
  3 (PgDn), 2 (flecha abajo) y 1 (Fin) mueve el mouse a la correspondiente 
  dirección;
- sosteniendo una de las teclas de arriba y luego sosteniendo la tecla 5 moverá
el puntero más rápido;
- / seleccionará el primer botón del mouse (botón izquierdo);
- * seleccionará el segundo botón del mouse (botón del medio);
- - seleccionará el tercer botón del mouse (botón derecho);
- 5 hará un click con el botón actualmente seleccionado del mouse;
- + hará un doble click con el botón actualmente seleccionado;
- 0 (Ins) cliqueará y mantendrá el botón seleccionado actualmente;
- . (Del) liberará el botón seleccionado actualmente que fue anteriormente
  cliqueado con la tecla 0 (Ins).

Los valores anteriores de las teclas funcionarán en un servidor X XFree86 3.2
(X11R6.1) pero su alcance puede variar.

Como hacer un gdb backtrace
===========================

Backtraces pueden ayudarnos a arreglar errores que hicieron que Window Maker falle.
Si encuentra un bug que hace fallar a Window Maker, por favor envie un backtrace con su
reporte de error.

Para hacer un backtrace útil, necesita un archivo core con información de depuración
producida por Window Maker cuando falló. Debería haber sido instalado sin stripping también.

Para compilar wmaker con información de depuración:

./configure
make CFLAGS=-g

Si obtiene el cuadro de diálogo que le dice que wmaker falló y le 
pregunta que hacer, respóndale "Abortar y dejar un archivo core"

script
cd src
gdb .libs/wmaker path_to_the_core_file

Luego, en el prompt gdb escriba "bt". Salga de gdb escribiendo "quit"
y luego, en el prompt del shell, scriba "exit"

El archivo llamado typescript contendrá el backtrace.

Derechos de Autor y Descargo de Responsabilidad
===============================================

Window Maker está registrado por Alfredo K. Kojima y está licensiado por la 
Licensia Pública General GNU. Lea el archivo COPYING para leer la licensia
completa.

Los íconos que son distribuidos con este programa y fueron hechos por Marco
van Hylckama Vlieg, están licenciados por la Licencia Pública General GNU. 
Lea el archivo COPYING para leer la licencia completa.

Los íconos listados en COPYING.WTFPL y son distribuidos en este programa
fueron hechos por Banlu Kemiyatorn (]d), están licenciados por la 
"do What The Fuck you want to Public License". Lea el archivo COPYING.WTFPL
para leer la licencia completa.

NeXT, OpenStep y NEXTSTEP son marcas registradas de NeXT Computer, Inc.
Todas las otras marcas registradas son propiedad de sus respectivos dueños.

Los autores se reservan el derecho de hacer cambios en el software sin previo
aviso.

Autores
=======

Alfredo K. Kojima <kojima@windowmaker.info>
Dan Pascu <dan@windowmaker.info>
]d <id@windowmaker.info>

Por favor no nos haga preguntas antes de leer la documentación (especialmente 
la FAQ, este archivo y los archivos INSTALL) y sobre cosas "cool" que ve en
las capturas de pantalla del escritorio de las personas.

El archivo AUTHORS contiene una lista de las personas que han contribuido 
con el proyecto. El nombre de las personas que han ayudado con localization 
(traducción) se puede encontrar en po/README y Window Maker/README

Si tiene algún comentario, arreglos y reportes de errores (complete BUGFORMs)
y enviémelos a developers@windowmaker.info



traducido por Efraín Maximiliano Palermo <max_drake2001@yahoo.com.ar>
