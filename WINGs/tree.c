


//#include <stdlib.h>
#include <string.h>

#include "WUtil.h"


typedef struct W_TreeNode {
    void *data;

    //unsigned int uflags:16;

    WMArray *leaves;

    int depth;

    struct W_TreeNode *parent;

    WMFreeDataProc *destructor;
} W_TreeNode;



void
destroyNode(void *data)
{
    WMTreeNode *node = (WMTreeNode*) data;

    if (node->destructor) {
        (*node->destructor)(node->data);
    }
    if (node->leaves) {
        WMFreeArray(node->leaves);
    }
    wfree(node);
}


WMTreeNode*
WMCreateTreeNode(void *data)
{
    return WMCreateTreeNodeWithDestructor(data, NULL);
}


WMTreeNode*
WMCreateTreeNodeWithDestructor(void *data, WMFreeDataProc *destructor)
{
    WMTreeNode *node;

    node = (WMTreeNode*) wmalloc(sizeof(W_TreeNode));
    memset(node, 0, sizeof(W_TreeNode));

    node->destructor = destructor;

    node->data = data;
    node->parent = NULL;
    node->depth = 0;
    node->leaves = WMCreateArrayWithDestructor(1, destroyNode);

    return node;
}


WMTreeNode*
WMInsertItemInTree(WMTreeNode *parent, int index, void *item)
{
    WMTreeNode *node;

    wassertrv(parent!=NULL, NULL);

    node = WMCreateTreeNodeWithDestructor(item, parent->destructor);
    node->parent = parent;
    node->depth = parent->depth+1;
    if (index < 0 || index > WMGetArrayItemCount(parent->leaves)) {
        WMAddToArray(parent->leaves, node);
    } else {
        WMInsertInArray(parent->leaves, index, node);
    }

    return node;
}


WMTreeNode*
WMAddItemToTree(WMTreeNode *parent, void *item)
{
    WMTreeNode *node;

    wassertrv(parent!=NULL, NULL);

    node = WMCreateTreeNodeWithDestructor(item, parent->destructor);
    node->parent = parent;
    node->depth = parent->depth+1;
    WMAddToArray(parent->leaves, node);

    return node;
}


static void
updateNodeDepth(WMTreeNode *node, int depth)
{
    int i;

    node->depth = depth;
    for (i=0; i<WMGetArrayItemCount(node->leaves); i++) {
        updateNodeDepth(WMGetFromArray(node->leaves, i), depth+1);
    }
}


WMTreeNode*
WMInsertNodeInTree(WMTreeNode *parent, int index, WMTreeNode *node)
{
    wassertrv(parent!=NULL, NULL);
    wassertrv(node!=NULL, NULL);

    node->parent = parent;
    updateNodeDepth(node, parent->depth+1);
    if (index < 0 || index > WMGetArrayItemCount(parent->leaves)) {
        WMAddToArray(parent->leaves, node);
    } else {
        WMInsertInArray(parent->leaves, index, node);
    }

    return node;
}


WMTreeNode*
WMAddNodeToTree(WMTreeNode *parent, WMTreeNode *node)
{
    wassertrv(parent!=NULL, NULL);
    wassertrv(node!=NULL, NULL);

    node->parent = parent;
    updateNodeDepth(node, parent->depth+1);
    WMAddToArray(parent->leaves, node);

    return node;
}


int
WMGetTreeNodeDepth(WMTreeNode *node)
{
    return node->depth;
}


void
WMDestroyTreeNode(WMTreeNode *node)
{
    destroyNode(node);
}


