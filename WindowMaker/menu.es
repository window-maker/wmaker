/*
 * Definición para el menú principal de WindowMaker
 *
 * La sintáxis es:
 *
 * <Título> <Comando> <Parámetros>
 *
 * <Título> es cualquier cadena para usar como título. Debe estar encerrada entre "" si tiene 
 * 	    espacios.
 *
 * <Comando> puede ser uno de estos mandatos: 
 *	MENU - Comienza definición de (sub)menu 
 *	OPEN_MENU - abre un el contenido de menu desde fichero, tuberia or directorio(s) 
 *		    y ecentualmente puede precederle un comando.
 *	END  - finaliza una definición de (sub)menu
 *	WORKSPACE_MENU - Añade el submenu para operaciones con el escritorio
 *	EXEC <programa> - ejecuta un programa externo
 *	EXIT - sale del gestor de ventanas
 *	RESTART [<Gestor de ventanas>] - rearrancar WindowMaker o arranca otro gestor de ventanas
 *	REFRESH - refrescar el escritorio
 *	ARRANGE_ICONS - arreglar (alinear) los iconos en el escritorio
 *	SHUTDOWN - Mata todos los clientes (y cierra la sesión X)
 *	SHOW_ALL - desoculta todas las ventanas en el escritorio
 *	HIDE_OTHERS - oculta todas las ventanas en el excritorio excepto la actual
 *	SAVE_SESSION - guarda el estado actual de sesión del escritorio, incluyendo 
 *		       todas las aplicaciones que se estan ejecutando y todos sus parámetros,
 *		       geometria, posición en la pantalla, escritorio al que pertenecen, el dock
 *		       o clip desde el que fueron lanzados, y si estan minimizados, sombreados
 *		       o ocultos. Asímismo también se guarda el escritorio en el usuario está.
 *		       Todo ello será repuesto cada vez que se arranque WindowMaker hasta que
 *		       se vuelva a guardar SAVE_SESSION o se use CLEAR_SESSION. 
 *		       Si SaveSessionOnExit = Yes; en el fichero de dominio de WindowMaker
 *		       entonces el guardado es automatico en cada final de sesión, 
 *		       sobreescribiendo cualquier SAVE_SESSION o CLEAR_SESSION.
 *	CLEAR_SESSION - limpia cualquier sesión guardada anteriormente. No tiene efecto si
 *		       SaveSessionOnExit is Verdadero.
 *
 * OPEN_MENU sintáxis:
 *   1. Manejando Ficheros de menú.
 *	// Abre un fichero.menu que contiene información de menu válida que será insertada
 *	// en la posición actual
 *	OPEN_MENU fichero.menu
 *   2. Manejando tuberias de menú.
 *	// ejecuta un comado y usa su salida estándar para construir el  menu.
 *	// La salida del comando ha de ser una descripción válida de menú.
 *	// El espacio entre '|' y comando es opcional.
 *	OPEN_MENU | comando
 *   3. Manejando directorios.
 *	// Abre uno o más directorios y construye un menú a base de 
 *	// todos los subdirectorios y sus ficheros ejecutables ordenados alfabéticamente
 *	OPEN_MENU /algún/directorio [/algún/otro/directorio ...]
 *   4. Manejando directory con comandos.
 *	// Abre uno o más directorios y construye un menú a base de 
 *	// todos los subdirectorios y sus ficheros ejecutables ordenados alfabéticamente
 *	// y precediendo estos con un comando.
 *	OPEN_MENU /algún/dirrctorio [/algún/otro/directorio ...] WITH comando -opciones
 *
 *
 * <Parámetros> es el programa a ejecutar.
 *
 * ** Opciones para la linea de comandos EXEC:
 * %s - substituye con la selección actual
 * %a(mensaje) - abre una caja de entrada de datos con un mensaje y realiza la sustitución
 *		con los datos recibidos a través de la caja
 * %w - sustituye por XID de la ventana actual
 *
 * Se pueden poner carácteres especiales (como % y ") con el carácter \ :
 * ex: xterm -T "\"Hola Mundo\""
 *
 * Puedes asímismo usar caracteres escape como \n
 *
 * Cada sentencia MENU debe tener una sentencia END que la finalice.
 *
 * Ejemplo:
 *
 * "Test" MENU
 *	"XTerm" EXEC xterm
 *		// crea un submenu con los contenidos de /usr/openwin/bin
 *	"XView apps" OPEN_MENU "/usr/openwin/bin"
 *		// algunas aplicaciones X11 de diversos directorios
 *	"X11 apps" OPEN_MENU /usr/X11/bin ~/bin/X11
 *		// algunos fondos de escritorio
 *	"Background" OPEN_MENU ~/images /usr/share/images WITH wmsetbg -u -t
 *		// inserta el fichero style.menu en esta entrada de menu
 *	"Style" OPEN_MENU style.menu
 * "Test" END
 */

#include "wmmacros"

"GNU WindowMaker" MENU
	"Info" MENU
		"Info Panel..." INFO_PANEL
		"Legal" LEGAL_PANEL
		"Consola del Sistema" EXEC xconsole
		"Carga del Sistema" EXEC xosview || xload
		"Lista de Procesos" EXEC xterm -e top
		"Manual de usuario" EXEC xman
	"Info" END
	"XTerm" EXEC xterm -sb 
	"Rxvt" EXEC rxvt -bg black -fg white -fn fixed
	"Escritorios" WORKSPACE_MENU
	"Aplicaciones" MENU
		"Gráficos" MENU
			"Gimp" EXEC gimp >/dev/null
			"XV" EXEC xv
			"XPaint" EXEC xpaint
			"XFig" EXEC xfig
		"Gráficos" END
		"X File Manager" EXEC xfm
		"OffiX Files" EXEC files
		"LyX" EXEC lyx
		"Netscape" EXEC netscape 
  		"Ghostview" EXEC ghostview %a(Enter file to view)
		"Acrobat" EXEC /usr/local/Acrobat3/bin/acroread %a(Enter PDF to view)
  		"TkDesk" EXEC tkdesk
	"Aplicaciones" END
	"Editores" MENU
		"XFte" EXEC xfte
		"XEmacs" EXEC xemacs || emacs
		"XJed" EXEC xjed 
		"NEdit" EXEC nedit
		"Xedit" EXEC xedit
		"VI" EXEC xterm -e vi
	"Editores" END
	"Miscelánea" MENU
		"Xmcd" EXEC xmcd 2> /dev/null
		"Xplaycd" EXEC xplaycd
		"Xmixer" EXEC xmixer
	"Miscelánea" END
	"Utilidades" MENU
		"Calculadora" EXEC xcalc
		"Propiedades de ventana" EXEC xprop | xmessage -center -title 'xprop' -file -
		"Selector de Fuentes" EXEC xfontsel
		"Emulador de Terminal" EXEC xminicom
		"Lupa" EXEC xmag
		"Mapa de Color" EXEC xcmap
		"XKill" EXEC xkill
		"ASClock" EXEC asclock -shape
		"Clipboard" EXEC xclipboard
	"Utilidades" END

	"Selección" MENU
		"Copiar" EXEC echo '%s' | wxcopy
		"Enviar a" EXEC xterm -name mail -T "Pine" -e pine %s
		"Navigar" EXEC netscape %s
		"Buscar en el Manual" EXEC MANUAL_SEARCH(%s)
	"Selección" END

	"Escritorio" MENU
		"Ocultar otras" HIDE_OTHERS
		"Mostrar todas" SHOW_ALL
		"Arreglar Iconos" ARRANGE_ICONS
		"Refrescar" REFRESH
		"Bloquear" EXEC xlock -allowroot -usefirst
		"Guardar Sesión" SAVE_SESSION
		"Borrar Sesión Guardada" CLEAR_SESSION
	"Escritorio" END

	"Apariencia" MENU
		"Temas" OPEN_MENU -noext  THEMES_DIR ~/GNUstep/Library/WindowMaker/Themes WITH setstyle
		"Estilos" OPEN_MENU -noext  STYLES_DIR ~/GNUstep/Library/WindowMaker/Styles WITH setstyle
		"Juegos de Iconos" OPEN_MENU -noext  ICON_SETS_DIR ~/GNUstep/Library/WindowMaker/IconSets WITH seticons
		"Fondo" MENU
			"Sólido" MENU
                        	"Black" WS_BACK '(solid, black)'
                        	"Blue"  WS_BACK '(solid, "#505075")'
				"Indigo" WS_BACK '(solid, "#243e6c")'
				"Deep Blue" WS_BACK '(solid, "#180090")'
                        	"Purple" WS_BACK '(solid, "#554466")'
                        	"Wheat"  WS_BACK '(solid, "wheat4")'
                        	"Dark Gray"  WS_BACK '(solid, "#333340")'
                        	"Wine" WS_BACK '(solid, "#400020")'
			"Sólido" END
			"Gradiente" MENU
				"Flag" WS_BACK '(mdgradient, green, red, white, green)'
				"Sky" WS_BACK '(vgradient, blue4, white)'
			"Gradiente" END
			"Imagenes" OPEN_MENU -noext  BACKGROUNDS_DIR ~/GNUstep/Library/WindowMaker/Backgrounds WITH wmsetbg -u -t
		"Fondo" END
		"Guardar Tema" EXEC getstyle -t ~/GNUstep/Library/WindowMaker/Themes/"%a(Theme name)"
		"Guardar Juego de Iconos" EXEC geticonset ~/GNUstep/Library/WindowMaker/IconSets/"%a(IconSet name)"
	"Apariencia" END

	"Salir"	MENU
		"Rearrancar" RESTART
		"Arrancar AfterStep" RESTART afterstep
		"Salir..."  EXIT
		"Cerrar la sesión..." SHUTDOWN
	"Salir" END
"GNU WindowMaker" END


