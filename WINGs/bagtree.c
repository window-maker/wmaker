



#include <stdlib.h>
#include <string.h>

#include "WUtil.h"


typedef struct W_Node {
    struct W_Node *parent;
    struct W_Node *left;
    struct W_Node *right;
    int color;
    
    void *data;
    int index;
} W_Node;


typedef struct W_TreeBag {
    W_Node *root;
    
    W_Node *nil;		       /* sentinel */
    
    int count;
} W_TreeBag;



static int getItemCount(WMBag *self);
static int appendBag(WMBag *self, WMBag *bag);
static int putInBag(WMBag *self, void *item);
static int insertInBag(WMBag *self, int index, void *item);
static int removeFromBag(WMBag *bag, void *item);
static int eraseFromBag(WMBag *bag, int index);
static int deleteFromBag(WMBag *bag, int index);
static void *getFromBag(WMBag *bag, int index);
static int countInBag(WMBag *bag, void *item);
static int firstInBag(WMBag *bag, void *item);
static void *replaceInBag(WMBag *bag, int index, void *item);
static int sortBag(WMBag *bag, int (*comparer)(const void*, const void*));
static void emptyBag(WMBag *bag);
static void freeBag(WMBag *bag);
static void mapBag(WMBag *bag, void (*function)(void*, void*), void *data);
static int findInBag(WMBag *bag, int (*match)(void*));;
static void *first(WMBag *bag, WMBagIterator *ptr);
static void *last(WMBag *bag, WMBagIterator *ptr);
static void *next(WMBag *bag, WMBagIterator *ptr);
static void *previous(WMBag *bag, WMBagIterator *ptr);
static void *iteratorAtIndex(WMBag *bag, int index, WMBagIterator *ptr);
static int indexForIterator(WMBag *bag, WMBagIterator ptr);


static W_BagFunctions bagFunctions = {
    getItemCount,
	appendBag,
	putInBag,
	insertInBag,
	removeFromBag,
	eraseFromBag,
	deleteFromBag,
	getFromBag,
	firstInBag,
	countInBag,
	replaceInBag,
	sortBag,
	emptyBag,
	freeBag,
	mapBag,
	findInBag,
	first,
	last,
	next,
	previous,
	iteratorAtIndex,
	indexForIterator
};




#define IS_LEFT(node) (node == node->parent->left)
#define IS_RIGHT(node) (node == node->parent->right)



static void leftRotate(W_TreeBag *tree, W_Node *node)
{
    W_Node *node2;
    
    node2 = node->right;
    node->right = node2->left;

    node2->left->parent = node;

    node2->parent = node->parent;

    if (node->parent == tree->nil) {
	tree->root = node2;
    } else {
	if (IS_LEFT(node)) {
	    node->parent->left = node2;
	} else {
	    node->parent->right = node2;
	}
    }
    node2->left = node;
    node->parent = node2;
}



static void rightRotate(W_TreeBag *tree, W_Node *node)
{
    W_Node *node2;
    
    node2 = node->left;
    node->left = node2->right;
    
    node2->right->parent = node;
    
    node2->parent = node->parent;

    if (node->parent == tree->nil) {
	tree->root = node2;
    } else {
	if (IS_LEFT(node)) {
	    node->parent->left = node2;
	} else {
	    node->parent->right = node2;
	}
    }
    node2->right = node;
    node->parent = node2;
}



static void treeInsert(W_TreeBag *tree, W_Node *node)
{
    W_Node *y = tree->nil;
    W_Node *x = tree->root;
    
    while (x != tree->nil) {
	y = x;
	if (node->index < x->index)
	    x = x->left;
	else
	    x = x->right;
    }
    node->parent = y;
    if (y == tree->nil)
	tree->root = node;
    else if (node->index < y->index)
	y->left = node;
    else
	y->right = node;
}


static void rbTreeInsert(W_TreeBag *tree, W_Node *node)
{
    W_Node *y;
    
    treeInsert(tree, node);

    node->color = 'R';
    
    while (node != tree->root && node->parent->color == 'R') {
	if (IS_LEFT(node->parent)) {
	    y = node->parent->parent->right;
	    
	    if (y->color == 'R') {
		
		node->parent->color = 'B';
		y->color = 'B';
		node->parent->parent->color = 'R';
		node = node->parent->parent;
		
	    } else {
		if (IS_RIGHT(node)) {
		    node = node->parent;
		    leftRotate(tree, node);
		}
		node->parent->color = 'B';
		node->parent->parent->color = 'R';
		rightRotate(tree, node->parent->parent);
	    }
	} else {
	    y = node->parent->parent->left;
	    
	    if (y->color == 'R') {
		
		node->parent->color = 'B';
		y->color = 'B';
		node->parent->parent->color = 'R';
		node = node->parent->parent;
		
	    } else {
		if (IS_LEFT(node)) {
		    node = node->parent;
		    rightRotate(tree, node);
		}
		node->parent->color = 'B';
		node->parent->parent->color = 'R';
		leftRotate(tree, node->parent->parent);
	    }
	}
    }
    tree->root->color = 'B';
}



static void rbDeleteFixup(W_TreeBag *tree, W_Node *node)
{
    W_Node *w;
    
    while (node != tree->root && node->color == 'B') {
	if (IS_LEFT(node)) {
	    w = node->parent->right;
	    if (w->color == 'R') {
		w->color = 'B';
		node->parent->color = 'R';
		leftRotate(tree, node->parent);
		w = node->parent->right;
	    }
	    if (w->left->color == 'B' && w->right->color == 'B') {
		w->color = 'R';
		node = node->parent;
	    } else {
		if (w->right->color == 'B') {
		    w->left->color = 'B';
		    w->color = 'R';
		    rightRotate(tree, w);
		    w = node->parent->right;
		}
		w->color = node->parent->color;
		node->parent->color = 'B';
		w->right->color = 'B';
		leftRotate(tree, node->parent);
		node = tree->root;
	    }
	} else {
	    w = node->parent->left;
	    if (w->color == 'R') {
		w->color = 'B';
		node->parent->color = 'R';
		leftRotate(tree, node->parent);
		w = node->parent->left;
	    }
	    if (w->left->color == 'B' && w->right->color == 'B') {
		w->color = 'R';
		node = node->parent;
	    } else {
		if (w->left->color == 'B') {
		    w->right->color = 'B';
		    w->color = 'R';
		    rightRotate(tree, w);
		    w = node->parent->left;
		}
		w->color = node->parent->color;
		node->parent->color = 'B';
		w->left->color = 'B';
		leftRotate(tree, node->parent);
		node = tree->root;
	    }	    
	}
    }
    node->color = 'B';
}


static W_Node *treeMinimum(W_Node *node, W_Node *nil)
{
    while (node->left != nil)
	node = node->left;
    return node;
}


static W_Node *treeMaximum(W_Node *node, W_Node *nil)
{
    while (node->right != nil)
	node = node->right;
    return node;
}


static W_Node *treeSuccessor(W_Node *node, W_Node *nil)
{
    W_Node *y;
    
    if (node->right != nil) {
	return treeMinimum(node->right, nil);
    }
    y = node->parent;
    while (y != nil && node == y->right) {
	node = y;
	y = y->parent;
    }
    return y;
}


static W_Node *treePredecessor(W_Node *node, W_Node *nil)
{
    W_Node *y;
    
    if (node->left != nil) {
	return treeMaximum(node->left, nil);
    }
    y = node->parent;
    while (y != nil && node == y->left) {
	node = y;
	y = y->parent;
    }
    return y;
}


static W_Node *rbTreeDelete(W_TreeBag *tree, W_Node *node)
{
    W_Node *nil = tree->nil;
    W_Node *x, *y;
    
    if (node->left == nil || node->right == nil) {
	y = node;
    } else {
	y = treeSuccessor(node, nil);
    }
    
    if (y->left != nil) {
	x = y->left;
    } else {
	x = y->right;
    }
    
    x->parent = y->parent;
    
    if (y->parent == nil) {
	tree->root = x;
    } else {
	if (IS_LEFT(y)) {
	    y->parent->left = x;
	} else {
	    y->parent->right = x;
	}
    }
    if (y != node) {
	node->index = y->index;
	node->data = y->data;
    }
    if (y->color == 'B') {
	rbDeleteFixup(tree, x);
    }
    
    return y;
}



static W_Node *treeSearch(W_Node *root, W_Node *nil, int index)
{
    if (root == nil || root->index == index) {
	return root;
    }
    
    if (index < root->index) {
	return treeSearch(root->left, nil, index);
    } else {
	return treeSearch(root->right, nil, index);
    }
}


static W_Node *treeFind(W_Node *root, W_Node *nil, void *data)
{
    W_Node *tmp;
    
    if (root == nil || root->data == data)
	return root;

    tmp = treeFind(root->left, nil, data);
    if (tmp != nil)
	return tmp;
    
    tmp = treeFind(root->right, nil, data);

    return tmp;
}





#if 0
static char buf[512];

static void printNodes(W_Node *node, W_Node *nil, int depth)
{
    if (node == nil) {
	return;
    }
    
    printNodes(node->left, nil, depth+1);

    memset(buf, ' ', depth*2);
    buf[depth*2] = 0;
    if (IS_LEFT(node))
	printf("%s/(%2i\n", buf, node->index);
    else
	printf("%s\\(%2i\n", buf, node->index);

    printNodes(node->right, nil, depth+1);
}


void PrintTree(WMBag *bag)
{
    W_TreeBag *tree = (W_TreeBag*)bag->data;
    
    printNodes(tree->root, tree->nil, 0);
}
#endif




#define SELF ((W_TreeBag*)self->data)

WMBag *WMCreateTreeBag(void)
{
    return WMCreateTreeBagWithDestructor(NULL);
}


WMBag *WMCreateTreeBagWithDestructor(void (*destructor)(void*))
{
    WMBag *bag;
    W_TreeBag *tree;
    
    bag = wmalloc(sizeof(WMBag));

    bag->data = tree = wmalloc(sizeof(W_TreeBag));
    memset(tree, 0, sizeof(W_TreeBag));

    tree->nil = wmalloc(sizeof(W_Node));
    memset(tree->nil, 0, sizeof(W_Node));
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
    tree->nil->index = WBNotFound;

    tree->root = tree->nil;
    
    bag->destructor = destructor;
    
    bag->func = bagFunctions;
    
    return bag;
}


static int getItemCount(WMBag *self)
{
    return SELF->count;
}


static int appendBag(WMBag *self, WMBag *bag)
{
    WMBagIterator ptr;
    void *data;
    
    for (data = first(bag, &ptr); data != NULL; data = next(bag, &ptr)) {
	if (!putInBag(self, data))
	    return 0;
    }
    
    return 1;
}


static int putInBag(WMBag *self, void *item)
{
    W_Node *ptr;
    
    ptr = wmalloc(sizeof(W_Node));
    
    ptr->data = item;
    ptr->index = SELF->count;
    ptr->left = SELF->nil;
    ptr->right = SELF->nil;
    ptr->parent = SELF->nil;
    
    rbTreeInsert(SELF, ptr);
    
    SELF->count++;
    
    return 1;
}


static int insertInBag(WMBag *self, int index, void *item)
{
    W_Node *ptr;

    ptr = wmalloc(sizeof(W_Node));

    ptr->data = item;
    ptr->index = index;
    ptr->left = SELF->nil;
    ptr->right = SELF->nil;
    ptr->parent = SELF->nil;

    rbTreeInsert(SELF, ptr);
    
    while ((ptr = treeSuccessor(ptr, SELF->nil)) != SELF->nil) {
	ptr->index++;
    }
    
    
    SELF->count++;

    return 1;
}



static int removeFromBag(WMBag *self, void *item)
{
    W_Node *ptr = treeFind(SELF->root, SELF->nil, item);
    
    if (ptr != SELF->nil) {
	W_Node *tmp;
	
	SELF->count--;
	
	tmp = treeSuccessor(ptr, SELF->nil);
	while (tmp != SELF->nil) {
	    tmp->index--;
	    tmp = treeSuccessor(tmp, SELF->nil);
	}
	
	ptr = rbTreeDelete(SELF, ptr);
	free(ptr);

	return 1;
    } else {
	return 0;
    }
}



static int eraseFromBag(WMBag *self, int index)
{
    W_Node *ptr = treeSearch(SELF->root, SELF->nil, index);
    
    if (ptr != SELF->nil) {
	
	SELF->count--;
	
	ptr = rbTreeDelete(SELF, ptr);
	free(ptr);

	return 1;
    } else {
	return 0;
    }
}


static int deleteFromBag(WMBag *self, int index)
{
    W_Node *ptr = treeSearch(SELF->root, SELF->nil, index);
    
    if (ptr != SELF->nil) {
	W_Node *tmp;
	
	SELF->count--;
	
	tmp = treeSuccessor(ptr, SELF->nil);
	while (tmp != SELF->nil) {
	    tmp->index--;
	    tmp = treeSuccessor(tmp, SELF->nil);
	}
	
	ptr = rbTreeDelete(SELF, ptr);
	free(ptr);

	return 1;
    } else {
	return 0;
    }
}


static void *getFromBag(WMBag *self, int index)
{    
    W_Node *node;

    node = treeSearch(SELF->root, SELF->nil, index);
    if (node != SELF->nil)
	return node->data;
    else
	return NULL;
}



static int firstInBag(WMBag *self, void *item)
{
    W_Node *node;
    
    node = treeFind(SELF->root, SELF->nil, item);
    if (node != SELF->nil)
	return node->index;
    else
	return WBNotFound;
}



static int treeCount(W_Node *root, W_Node *nil, void *item)
{
    int count = 0;
    
    if (root == nil)
	return 0;
    
    if (root->data == item)
	count++;

    if (root->left != nil)
	count += treeCount(root->left, nil, item);
    
    if (root->right != nil)
	count += treeCount(root->right, nil, item);

    return count;
}



static int countInBag(WMBag *self, void *item)
{    
    return treeCount(SELF->root, SELF->nil, item);
}


static void *replaceInBag(WMBag *self, int index, void *item)
{
    W_Node *ptr = treeSearch(SELF->root, SELF->nil, index);
    void *old = NULL;

    if (item == NULL) {
	SELF->count--;
	
	ptr = rbTreeDelete(SELF, ptr);
	free(ptr);
    } else if (ptr != SELF->nil) {
	old = ptr->data;
	ptr->data = item;
    } else {
	W_Node *ptr;

	ptr = wmalloc(sizeof(W_Node));

	ptr->data = item;
	ptr->index = index;
	ptr->left = SELF->nil;
	ptr->right = SELF->nil;
	ptr->parent = SELF->nil;

	rbTreeInsert(SELF, ptr);

	SELF->count++;
    }
    
    return old;
}




static int sortBag(WMBag *self, int (*comparer)(const void*, const void*))
{
    void **items;
    W_Node *tmp;
    int i;


    items = wmalloc(sizeof(void*)*SELF->count);
    i = 0;
    
    tmp = treeMinimum(SELF->root, SELF->nil);
    while (tmp != SELF->nil) {
	items[i++] = tmp->data;
	tmp = treeSuccessor(tmp, SELF->nil);
    }

    qsort(&items[0], SELF->count, sizeof(void*), comparer);
    
    i = 0;
    tmp = treeMinimum(SELF->root, SELF->nil);
    while (tmp != SELF->nil) {
	tmp->index = i;
	tmp->data = items[i++];
	tmp = treeSuccessor(tmp, SELF->nil);
    }
    
    wfree(items);
    
    return 1;
}




static void deleteTree(WMBag *self, W_Node *node)
{
    if (node == SELF->nil)
	return;
    
    deleteTree(self, node->left);
    
    if (self->destructor)
	self->destructor(node->data);
    
    deleteTree(self, node->right);
    
    free(node);
}


static void emptyBag(WMBag *self)
{
    deleteTree(self, SELF->root);
    SELF->root = SELF->nil;
    SELF->count = 0;
}


static void freeBag(WMBag *self)
{
    emptyBag(self);
    
    free(self);
}



static void mapTree(W_TreeBag *tree, W_Node *node,
		    void (*function)(void*, void*), void *data)
{
    if (node == tree->nil)
	return;
    
    mapTree(tree, node->left, function, data);

    (*function)(node->data, data);

    mapTree(tree, node->right, function, data);
}


static void mapBag(WMBag *self, void (*function)(void*, void*), void *data)
{
    mapTree(SELF, SELF->root, function, data);
}



static int findInTree(W_TreeBag *tree, W_Node *node, int (*function)(void*))
{
    int index;
    
    if (node == tree->nil)
	return WBNotFound;
    
    index = findInTree(tree, node->left, function);
    if (index != WBNotFound)
	return index;

    if ((*function)(node->data)) {
	return node->index;
    }

    return findInTree(tree, node->right, function);
}


static int findInBag(WMBag *self, int (*match)(void*))
{
    return findInTree(SELF, SELF->root, match);
}




static void *first(WMBag *self, WMBagIterator *ptr)
{
    W_Node *node;
    
    node = treeMinimum(SELF->root, SELF->nil);
    
    if (node == SELF->nil) {
	*ptr = NULL;
	
	return NULL;
    } else {
	*ptr = node;
    
	return node->data;
    }
}



static void *last(WMBag *self, WMBagIterator *ptr)
{

    W_Node *node;
    
    node = treeMaximum(SELF->root, SELF->nil);
    
    if (node == SELF->nil) {
	*ptr = NULL;
	
	return NULL;
    } else {
	*ptr = node;
    
	return node->data;
    }
}



static void *next(WMBag *self, WMBagIterator *ptr)
{
    W_Node *node;
    
    if (*ptr == NULL)
	return NULL;
    
    node = treeSuccessor(*ptr, SELF->nil);
    
    if (node == SELF->nil) {
	*ptr = NULL;
	
	return NULL;
    } else {
	*ptr = node;
    
	return node->data;
    }
}



static void *previous(WMBag *self, WMBagIterator *ptr)
{
    W_Node *node;
    
    if (*ptr == NULL)
	return NULL;
    
    node = treePredecessor(*ptr, SELF->nil);
    

    if (node == SELF->nil) {
	*ptr = NULL;
	
	return NULL;
    } else {
	*ptr = node;
    
	return node->data;
    }
}



static void *iteratorAtIndex(WMBag *self, int index, WMBagIterator *ptr)
{
    W_Node *node;
    
    node = treeSearch(SELF->root, SELF->nil, index);
    
    if (node == SELF->nil) {
	*ptr = NULL;
	return NULL;
    } else {
	*ptr = node;
	return node->data;
    }
}


static int indexForIterator(WMBag *bag, WMBagIterator ptr)
{
    return ((W_Node*)ptr)->index;
}

