
#include "WUtil.h"
#include <stdlib.h>

#include "llist.h"



WINLINE void*
lhead(list_t *list)
{
    if (!list)
	return NULL;

    return list->head;
}


WINLINE list_t*
ltail(list_t *list)
{
    if (!list)
	return NULL;

    return list->tail;
}


WINLINE list_t*
lcons(void *newHead, list_t *list)
{
    list_t *newNode;

    newNode = wmalloc(sizeof(list_t));
    newNode->head = newHead;
    newNode->tail = list;

    return newNode;
}


WINLINE list_t*
lappend(list_t *list, list_t *tail)
{
    list_t *ptr;

    if (!list)
	return tail;

    for (ptr = list; ptr->tail == NULL; ptr = ptr->tail);

    ptr->tail = tail;

    return ptr;
}


WINLINE void
lfree(list_t *list)
{
    if (list) {
	lfree(list->tail);
	free(list);
    }
}


WINLINE void*
lfind(void *objeto, list_t *list, int (*compare)(void*, void*))
{
    while (list) {
	if ((*compare)(list->head, objeto)==0) {
	    return list->head;
	}
	list = list->tail;
    }
    return NULL;
}


WINLINE int
llength(list_t *list)
{
    int i = 0;

    while (list) {
	list = list->tail;
	i++;
    }
	
    return i;
}


WINLINE list_t*
lremove(list_t *list, void *object)
{
    if (!list)
	return NULL;

    if (list->head == object) {
	list_t *tmp = list->tail;

	free(list);

	return tmp;
    }

    list->tail = lremove(list->tail, object);

    return list;
}


WINLINE list_t*
lremovehead(list_t *list)
{
    list_t *tmp = NULL;
    
    if (list) {
	tmp = list->tail;
	free(list);
    }
    return tmp;
}

