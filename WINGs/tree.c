


#include <string.h>

#include "WUtil.h"


typedef struct W_TreeNode {
    void *data;

    /*unsigned int uflags:16;*/

    WMArray *leaves;

    int depth;

    struct W_TreeNode *parent;

    WMFreeDataProc *destructor;
} W_TreeNode;




void
destroyNode(void *data)
{
    WMTreeNode *aNode = (WMTreeNode*) data;

    if (aNode->destructor) {
        (*aNode->destructor)(aNode->data);
    }
    if (aNode->leaves) {
        WMFreeArray(aNode->leaves);
    }
    wfree(aNode);
}


WMTreeNode*
WMCreateTreeNode(void *data)
{
    return WMCreateTreeNodeWithDestructor(data, NULL);
}


WMTreeNode*
WMCreateTreeNodeWithDestructor(void *data, WMFreeDataProc *destructor)
{
    WMTreeNode *aNode;

    aNode = (WMTreeNode*) wmalloc(sizeof(W_TreeNode));
    memset(aNode, 0, sizeof(W_TreeNode));

    aNode->destructor = destructor;

    aNode->data = data;
    aNode->parent = NULL;
    aNode->depth = 0;
    aNode->leaves = NULL;
    /*aNode->leaves = WMCreateArrayWithDestructor(1, destroyNode);*/

    return aNode;
}


WMTreeNode*
WMInsertItemInTree(WMTreeNode *parent, int index, void *item)
{
    WMTreeNode *aNode;

    wassertrv(parent!=NULL, NULL);

    aNode = WMCreateTreeNodeWithDestructor(item, parent->destructor);
    aNode->parent = parent;
    aNode->depth = parent->depth+1;
    if (!parent->leaves) {
        parent->leaves = WMCreateArrayWithDestructor(1, destroyNode);
    }
    if (index < 0) {
        WMAddToArray(parent->leaves, aNode);
    } else {
        WMInsertInArray(parent->leaves, index, aNode);
    }

    return aNode;
}


static void
updateNodeDepth(WMTreeNode *aNode, int depth)
{
    int i;

    aNode->depth = depth;

    if (aNode->leaves) {
        for (i=0; i<WMGetArrayItemCount(aNode->leaves); i++) {
            updateNodeDepth(WMGetFromArray(aNode->leaves, i), depth+1);
        }
    }
}


WMTreeNode*
WMInsertNodeInTree(WMTreeNode *parent, int index, WMTreeNode *aNode)
{
    wassertrv(parent!=NULL, NULL);
    wassertrv(aNode!=NULL, NULL);

    aNode->parent = parent;
    updateNodeDepth(aNode, parent->depth+1);
    if (!parent->leaves) {
        parent->leaves = WMCreateArrayWithDestructor(1, destroyNode);
    }
    if (index < 0) {
        WMAddToArray(parent->leaves, aNode);
    } else {
        WMInsertInArray(parent->leaves, index, aNode);
    }

    return aNode;
}


void
WMDestroyTreeNode(WMTreeNode *aNode)
{
    wassertr(aNode!=NULL);

    if (aNode->parent && aNode->parent->leaves) {
        WMRemoveFromArray(aNode->parent->leaves, aNode);
    } else {
        destroyNode(aNode);
    }
}


void
WMDeleteLeafForTreeNode(WMTreeNode *aNode, int index)
{
    wassertr(aNode!=NULL);
    wassertr(aNode->leaves!=NULL);

    WMDeleteFromArray(aNode->leaves, index);
}


static int
sameData(void *item, void *data)
{
    return (((WMTreeNode*)item)->data == data);
}


void
WMRemoveLeafForTreeNode(WMTreeNode *aNode, void *leaf)
{
    int index;

    wassertr(aNode!=NULL);
    wassertr(aNode->leaves!=NULL);

    index = WMFindInArray(aNode->leaves, sameData, leaf);
    if (index != WANotFound) {
        WMDeleteFromArray(aNode->leaves, index);
    }
}


void*
WMReplaceDataForTreeNode(WMTreeNode *aNode, void *newData)
{
    void *old;

    wassertrv(aNode!=NULL, NULL);

    old = aNode->data;
    aNode->data = newData;

    return old;
}


void*
WMGetDataForTreeNode(WMTreeNode *aNode)
{
    return aNode->data;
}


int
WMGetTreeNodeDepth(WMTreeNode *aNode)
{
    return aNode->depth;
}


WMTreeNode*
WMGetParentForTreeNode(WMTreeNode *aNode)
{
    return aNode->parent;
}


void
WMSortLeavesForTreeNode(WMTreeNode *aNode, WMCompareDataProc *comparer)
{
    wassertr(aNode!=NULL);

    if (aNode->leaves) {
        WMSortArray(aNode->leaves, comparer);
    }
}


static void
sortLeavesForNode(WMTreeNode *aNode, WMCompareDataProc *comparer)
{
    int i;

    if (!aNode->leaves)
        return;

    WMSortArray(aNode->leaves, comparer);
    for (i=0; i<WMGetArrayItemCount(aNode->leaves); i++) {
        sortLeavesForNode(WMGetFromArray(aNode->leaves, i), comparer);
    }
}


void
WMSortTree(WMTreeNode *aNode, WMCompareDataProc *comparer)
{
    wassertr(aNode!=NULL);

    sortLeavesForNode(aNode, comparer);
}


static WMTreeNode*
findNodeInTree(WMTreeNode *aNode, WMMatchDataProc *match, void *cdata)
{
    if (match==NULL) {
        if (aNode->data == cdata) {
            return aNode;
        }
    } else {
        if ((*match)(aNode->data, cdata)) {
            return aNode;
        }
    }

    if (aNode->leaves) {
        WMTreeNode *leaf;
        int i;

        for (i=0; i<WMGetArrayItemCount(aNode->leaves); i++) {
            leaf = findNodeInTree(WMGetFromArray(aNode->leaves, i), match, cdata);
            if (leaf) {
                return leaf;
            }
        }
    }

    return NULL;
}


WMTreeNode*
WMFindInTree(WMTreeNode *aTree, WMMatchDataProc *match, void *cdata)
{
    wassertrv(aTree!=NULL, NULL);

    return findNodeInTree(aTree, match, cdata);
}


