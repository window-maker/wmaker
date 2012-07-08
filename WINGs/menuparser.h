/* menuparser.h
 *
 *  Copyright (c) 2012 Christophe Curis
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _MENUPARSER_H_INCLUDED
#define _MENUPARSER_H_INCLUDED

/*
 * This file is not part of WINGs public API
 *
 * It defines internal things for the Menu Parser, the public API is
 * located in WINGs/WUtil.h as usual
 */

#define MAXLINE              1024

struct w_menu_parser {
	const char *file_name;
	FILE *file_handle;
	int line_number;
	char *rd;
	char line_buffer[MAXLINE];
};

#endif /* _MENUPARSER_H_INCLUDED */
