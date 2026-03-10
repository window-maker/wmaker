/* keytree.c - Trie (prefix tree) for key-chain bindings
 *
 *  Window Maker window manager
 *
 *  Copyright (c) 2026 Window Maker Team
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

#include "wconfig.h"
#include <string.h>
#include <WINGs/WUtil.h>
#include "keytree.h"

/* Global trie root */
WKeyNode *wKeyTreeRoot = NULL;

WKeyNode *wKeyTreeFind(WKeyNode *siblings, unsigned int mod, KeyCode key)
{
	WKeyNode *p = siblings;

	while (p != NULL) {
		if (p->modifier == mod && p->keycode == key)
			return p;
		p = p->next_sibling;
	}
	return NULL;
}

WKeyNode *wKeyTreeInsert(WKeyNode **root, unsigned int *mods, KeyCode *keys, int nkeys)
{
	WKeyNode **slot = root;
	WKeyNode *parent = NULL;
	int i;

	if (nkeys <= 0)
		return NULL;

	for (i = 0; i < nkeys; i++) {
		WKeyNode *node = wKeyTreeFind(*slot, mods[i], keys[i]);

		if (node == NULL) {
			node = wmalloc(sizeof(WKeyNode));
			memset(node, 0, sizeof(WKeyNode));
			node->modifier = mods[i];
			node->keycode = keys[i];
			node->parent = parent;
			node->next_sibling = *slot;
			*slot = node;
		}

		parent = node;
		slot = &node->first_child;
	}

	return parent;   /* leaf */
}


void wKeyTreeDestroy(WKeyNode *node)
{
	/* Iterates siblings at each level, recurses only into children */
	while (node != NULL) {
		WKeyNode *next = node->next_sibling;
		WKeyAction *act, *next_act;

		wKeyTreeDestroy(node->first_child);
		for (act = node->actions; act != NULL; act = next_act) {
			next_act = act->next;
			wfree(act);
		}
		wfree(node);
		node = next;
	}
}

WKeyAction *wKeyNodeAddAction(WKeyNode *leaf, WKeyActionType type)
{
	WKeyAction *act = wmalloc(sizeof(WKeyAction));
	WKeyAction *p;

	memset(act, 0, sizeof(WKeyAction));
	act->type = type;

	/* Append to end of list to preserve insertion order */
	if (leaf->actions == NULL) {
		leaf->actions = act;
	} else {
		p = leaf->actions;
		while (p->next)
			p = p->next;
		p->next = act;
	}
	return act;
}