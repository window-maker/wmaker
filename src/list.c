/* Generic single linked list to keep various information 
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.


Author: Kresten Krab Thorup

Many modifications by Alfredo K. Kojima
 

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

#include <WUtil.h>
#include "list.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stdlib.h>

#define xrealloc(ptr, size) wrealloc(ptr, size)

/* Return a cons cell produced from (head . tail) */

INLINE LinkedList* 
list_cons(void* head, LinkedList* tail)
{
  LinkedList* cell;

  cell = (LinkedList*)wmalloc(sizeof(LinkedList));
  cell->head = head;
  cell->tail = tail;
  return cell;
}

/* Inserts "item" in list using "compare" function to make a sorted list */

INLINE void
list_insert_sorted(void* item, LinkedList** list, int (*compare)(void*, void*))
{
    LinkedList *tmp, *cell;

    cell = (LinkedList*)wmalloc(sizeof(LinkedList));
    cell->head = item;
    if ((*list == NULL) || ((*compare)((*list)->head, item) >= 0)) {
        cell->tail = *list;
        *list = cell;
    } else {
        tmp = *list;
        while (tmp->tail) {
            if ((*compare)(tmp->tail->head, item) >= 0)
                break;
            tmp = tmp->tail;
        }
        cell->tail = tmp->tail;
        tmp->tail = cell;
    }
}

/* Return the length of a list, list_length(NULL) returns zero */

INLINE int
list_length(LinkedList* list)
{
  int i = 0;
  while(list)
    {
      i += 1;
      list = list->tail;
    }
  return i;
}

/* Return the Nth element of LIST, where N count from zero.  If N 
   larger than the list length, NULL is returned  */

INLINE void*
list_nth(int index, LinkedList* list)
{
  while(index-- != 0)
    {
      if(list->tail)
	list = list->tail;
      else
	return 0;
    }
  return list->head;
}

/* Remove the element at the head by replacing it by its successor */

#if 0
INLINE void
list_remove_head(LinkedList** list)
{
  if (!*list) return;  
  if ((*list)->tail)
    {
      LinkedList* tail = (*list)->tail; /* fetch next */
      *(*list) = *tail;		/* copy next to list head */
      free(tail);			/* free next */
    }
  else				/* only one element in list */
    {
      free(*list);
      (*list) = 0;
    }
}

/* Alfredo, is there any reason for the function to be as above,
 * instead of the more simple version below?
 * The difference is that above function do a copying of the structure
 * unlike the below that only changes where list points.
 * This makes the above function twice slower, because it copies the
 * whole structure, which contains 2 pointers. -Dan.
 * 
 * I don't know. I just copied this file from the gcc distribution
 * (I liked the API). One could think that the above function would
 * keep the pointer to the list unchanged, while the below would change
 * it, but since the list is passed by reference that doesn't make any
 * sense. -Alfredo
 */

#else
INLINE void
list_remove_head(LinkedList** list)
{
  if (!*list) return;  

  {
      LinkedList* next = (*list)->tail; /* fetch next */
      free(*list);			/* free head */
      *list = next;			/* update list pointer */
  }
}
#endif


/* Remove the element with `car' set to ELEMENT */
/*
INLINE void
list_remove_elem(LinkedList** list, void* elem)
{
  while (*list)
    {
      if ((*list)->head == elem)
        list_remove_head(list);
      *list = (*list ? (*list)->tail : NULL);
    }
}*/

INLINE LinkedList *
list_remove_elem(LinkedList* list, void* elem)
{
    LinkedList *tmp;
    
    if (list) {
	if (list->head == elem) {
	    tmp = list->tail;
	    free(list);
	    return tmp;
	}
	list->tail = list_remove_elem(list->tail, elem);
	return list;
    }
    return NULL;
}


/* Return element that has ELEM as car */

INLINE LinkedList*
list_find(LinkedList* list, void* elem)
{
  while(list)
    {
    if (list->head == elem)
      return list;
    list = list->tail;
    }
  return NULL;
}

/* Free list (backwards recursive) */

INLINE void
list_free(LinkedList* list)
{
  if(list)
    {
      list_free(list->tail);
      free(list);
    }
}

/* Map FUNCTION over all elements in LIST */

INLINE void
list_mapcar(LinkedList* list, void(*function)(void*))
{
  while(list)
    {
      (*function)(list->head);
      list = list->tail;
    }
}
