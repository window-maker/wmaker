/* Generic single linked list to keep various information 
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.

Author: Kresten Krab Thorup

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* As a special exception, if you link this library with files compiled with
   GCC to produce an executable, this does not cause the resulting executable
   to be covered by the GNU General Public License. This exception does not
   however invalidate any other reasons why the executable file might be
   covered by the GNU General Public License.  */

#ifndef __LIST_H_
#define __LIST_H_

#include "config.h"
#include "wconfig.h"


typedef struct LinkedList {
  void *head;
  struct LinkedList *tail;
} LinkedList;

INLINE LinkedList* list_cons(void* head, LinkedList* tail);

INLINE void list_insert_sorted(void* item, LinkedList** list,
                               int (*compare)(void*, void*));

INLINE int list_length(LinkedList* list);

INLINE void* list_nth(int index, LinkedList* list);

INLINE void list_remove_head(LinkedList** list);

INLINE LinkedList *list_remove_elem(LinkedList* list, void* elem);

INLINE void list_mapcar(LinkedList* list, void(*function)(void*));

INLINE LinkedList*list_find(LinkedList* list, void* elem);

INLINE void list_free(LinkedList* list);

#endif
