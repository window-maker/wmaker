/*
 *  Window Maker window manager
 *
 *  Copyright (c) 1997-2003 Alfredo K. Kojima
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

#ifndef WMDEFAULTS_H_
#define WMDEFAULTS_H_

typedef struct WDDomain {
	char *domain_name;
	WMPropList *dictionary;
	char *path;
	time_t timestamp;
} WDDomain;

WDDomain * wDefaultsInitDomain(char *domain, Bool requireDictionary);

void wDefaultsMergeGlobalMenus(WDDomain *menuDomain);

void wReadDefaults(WScreen *scr, WMPropList *new_dict);
void wDefaultUpdateIcons(WScreen *scr);
void wReadStaticDefaults(WMPropList *dict);
void wDefaultsCheckDomains(void *arg);
void wSaveDefaults(WScreen *scr);
void wDefaultFillAttributes(char *instance, char *class,
                            WWindowAttributes *attr, WWindowAttributes *mask,
                            Bool useGlobalDefault);

char *wDefaultGetIconFile(char *instance, char *class, Bool noDefault);

RImage * wDefaultGetImage(WScreen *scr, char *winstance, char *wclass, int max_size);


int wDefaultGetStartWorkspace(WScreen *scr, char *instance, char *class);
void wDefaultChangeIcon(WScreen *scr, char *instance, char* class, char *file);
char *get_default_icon_filename(WScreen *scr, char *winstance, char *wclass, char *command,
				Bool noDefault);
RImage *get_default_icon_rimage(WScreen *scr, char *file_name, int max_size);
#endif /* WMDEFAULTS_H_ */
