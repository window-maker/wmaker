

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define WINLINE inline
#else
# define WINLINE 
#endif

typedef struct list_t {
    void *head;
    struct list_t *tail;
} list_t;


WINLINE void *lhead(list_t *list);


WINLINE list_t *ltail(list_t *list);


WINLINE list_t *lcons(void *newHead, list_t *list);


WINLINE list_t *lappend(list_t *list, list_t *tail);


WINLINE void lfree(list_t *list);


WINLINE void *lfind(void *object, list_t *list, int (*compare)(void*, void*));


WINLINE int llength(list_t *list);


WINLINE list_t *lremove(list_t *list, void *object);


WINLINE list_t *lremovehead(list_t *list);

