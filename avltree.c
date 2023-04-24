/* ========================================================================= */
/**
 * @file avltree.c
 *
 * @copyright
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avltree.h"
#include "assert.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

/* == Declarations ========================================================= */

/** @private State of the AVL tree. */
struct _bs_avltree_t {
    /** Pointer to the root node. NULL indicates the tree is empty. */
    bs_avltree_node_t         *root_ptr;
    /** Number of nodes stored in this tree. */
    size_t                    nodes;
    /** Points to the method for comparing nodes. */
    bs_avltree_node_cmp_t     cmp;
    /** Points to the method for destroying a node. May be NULL. */
    bs_avltree_node_destroy_t destroy;
};

static void bs_avltree_flush(bs_avltree_t *tree_ptr);
static void bs_avltree_node_replace(bs_avltree_t *tree_ptr,
                                    bs_avltree_node_t *old_node_ptr,
                                    bs_avltree_node_t *new_node_ptr);
static size_t bs_avltree_node_size(bs_avltree_t *tree_ptr,
                                   bs_avltree_node_t *node_ptr);
static size_t bs_avltree_node_height(bs_avltree_t *tree_ptr,
                                     bs_avltree_node_t *node_ptr);
static void bs_avltree_node_exchange(bs_avltree_t *tree_ptr,
                                     bs_avltree_node_t *old_node_ptr,
                                     bs_avltree_node_t *new_node_ptr);
static void bs_avltree_node_delete(bs_avltree_t *tree_ptr,
                                   bs_avltree_node_t *node_ptr);

static bool bs_avltree_rot_right(bs_avltree_t *tree_ptr,
                                 bs_avltree_node_t *node_ptr);
static bool bs_avltree_rot_left(bs_avltree_t *tree_ptr,
                                bs_avltree_node_t *node_ptr);
static void bs_avltree_verify_node(
    bs_avltree_t *tree_ptr,
    const void *(*key_from_node)(const bs_avltree_node_t *node_ptr),
    bs_avltree_node_t *node_ptr,
    const void *min_key_ptr,
    const void *max_key_ptr);

static bs_avltree_node_t *bs_avltree_node_min(bs_avltree_node_t *node_ptr);
static bs_avltree_node_t *bs_avltree_node_max(bs_avltree_node_t *node_ptr);

/* == Exported Functions =================================================== */

/* ------------------------------------------------------------------------- */
bs_avltree_t *bs_avltree_create(bs_avltree_node_cmp_t cmp,
                                bs_avltree_node_destroy_t destroy)
{
    bs_avltree_t              *tree_ptr;

    tree_ptr = calloc(1, sizeof(bs_avltree_t));
    if (NULL == tree_ptr) return NULL;

    tree_ptr->cmp = cmp;
    tree_ptr->destroy = destroy;
    return tree_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_avltree_destroy(bs_avltree_t *tree_ptr)
{
    bs_avltree_flush(tree_ptr);

    free(tree_ptr);
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_lookup(bs_avltree_t *tree_ptr,
                                     const void *key_ptr)
{
    bs_avltree_node_t         *node_ptr;
    int                       cmp_rv;

    node_ptr = tree_ptr->root_ptr;
    while (NULL != node_ptr) {
        cmp_rv = tree_ptr->cmp(node_ptr, key_ptr);
        if (0 == cmp_rv) {
            return node_ptr;
        } else if (0 < cmp_rv) {
            /* node larger than key. proceed on the left. */
            node_ptr = node_ptr->left_ptr;
        } else {
            /* node less than key. proceed on the right. */
            node_ptr = node_ptr->right_ptr;
        }
    }

    return NULL;
}

/* ------------------------------------------------------------------------- */
bool bs_avltree_insert(bs_avltree_t *tree_ptr,
                       const void *new_key_ptr,
                       bs_avltree_node_t *new_node_ptr,
                       bool do_overwrite)
{
    bs_avltree_node_t         *node_ptr;
    bs_avltree_node_t         *parent_ptr;
    bs_avltree_node_t         *non_balanced_ptr;
    int                       cmp_rv;

    parent_ptr = NULL;
    non_balanced_ptr = NULL;
    node_ptr = tree_ptr->root_ptr;

    /* walk tree to appropriate bottom node. */
    parent_ptr = NULL;
    while (NULL != node_ptr) {

        /* keep track of current node and of last non-balanced node */
        parent_ptr = node_ptr;
        if (0 != node_ptr->balance) non_balanced_ptr = node_ptr;

        cmp_rv = tree_ptr->cmp(node_ptr, new_key_ptr);
        if (0 == cmp_rv) {
            /* node equals new node. replace, but only if requested. */
            if (!do_overwrite) return false;
            bs_avltree_node_replace(tree_ptr, node_ptr, new_node_ptr);
            return true;
        } else if (0 < cmp_rv) {
            /* node greater than new node. proceed on the left. */
            node_ptr = node_ptr->left_ptr;
        } else if (0 > cmp_rv) {
            /* node less than new node. proceed on the right. */
            node_ptr = node_ptr->right_ptr;
        }
    }

    /* attach new node to bottom node */
    tree_ptr->nodes++;
    new_node_ptr->parent_ptr = parent_ptr;
    if (NULL == new_node_ptr->parent_ptr) {
        /* was an empty tree, all fine. */
        tree_ptr->root_ptr = new_node_ptr;
        return true;
    }

    /* attach to correct side of parent node */
    if (0 < cmp_rv) {
        parent_ptr->left_ptr = new_node_ptr;
    } else {
        parent_ptr->right_ptr = new_node_ptr;
    }

    /* walk up along all balanced parents. adjust balance accordingly. */
    node_ptr = new_node_ptr;
    while (parent_ptr != non_balanced_ptr) {
        if (parent_ptr->left_ptr == node_ptr) {
            parent_ptr->balance--;
        } else {
            parent_ptr->balance++;
        }

        node_ptr = parent_ptr;
        parent_ptr = node_ptr->parent_ptr;
    }

    /* return, if entire path was in full balance */
    if (NULL == non_balanced_ptr) return true;

    /* tree was not balanced. adjust non-balanced node. */
    if (non_balanced_ptr->left_ptr == node_ptr) {
        non_balanced_ptr->balance--;
        if (-2 >= non_balanced_ptr->balance) {
            if (0 < node_ptr->balance) {
                bs_avltree_rot_left(tree_ptr, node_ptr);
            }
            bs_avltree_rot_right(tree_ptr, non_balanced_ptr);
        }
    } else {
        non_balanced_ptr->balance++;
        if (2 <= non_balanced_ptr->balance) {
            if (0 > node_ptr->balance) {
                bs_avltree_rot_right(tree_ptr, node_ptr);
            }
            bs_avltree_rot_left(tree_ptr, non_balanced_ptr);
        }
    }

    return true;
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_delete(bs_avltree_t *tree_ptr,
                                     const void *key_ptr) {
    bs_avltree_node_t *node_ptr;

    node_ptr = bs_avltree_lookup(tree_ptr, key_ptr);
    if (NULL == node_ptr) {
        return NULL;
    }

    bs_avltree_node_delete(tree_ptr, node_ptr);
    return node_ptr;
}

/* ------------------------------------------------------------------------- */
size_t bs_avltree_size(const bs_avltree_t *tree_ptr)
{
    return tree_ptr->nodes;
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_min(bs_avltree_t *tree_ptr)
{
    bs_avltree_node_t         *node_ptr;

    node_ptr = tree_ptr->root_ptr;
    if (NULL == node_ptr) {
        return NULL;
    }
    return bs_avltree_node_min(node_ptr);
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_max(bs_avltree_t *tree_ptr)
{
    bs_avltree_node_t         *node_ptr;

    node_ptr = tree_ptr->root_ptr;
    if (NULL == node_ptr) {
        return NULL;
    }
    return bs_avltree_node_max(node_ptr);
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_node_next(__UNUSED__ bs_avltree_t *tree_ptr,
                                        bs_avltree_node_t *node_ptr)
{
    if (NULL != node_ptr->right_ptr) {
        node_ptr = node_ptr->right_ptr;

        while (NULL != node_ptr->left_ptr) {
            node_ptr = node_ptr->left_ptr;
        }

        return node_ptr;
    }

    while ((NULL != node_ptr->parent_ptr) &&
           (node_ptr == node_ptr->parent_ptr->right_ptr)) {
        node_ptr = node_ptr->parent_ptr;
    }
    return node_ptr->parent_ptr;
}

/* ------------------------------------------------------------------------- */
bs_avltree_node_t *bs_avltree_node_prev(__UNUSED__ bs_avltree_t *tree_ptr,
                                        bs_avltree_node_t *node_ptr)
{
    if (NULL != node_ptr->left_ptr) {
        node_ptr = node_ptr->left_ptr;

        while (NULL != node_ptr->right_ptr) {
            node_ptr = node_ptr->right_ptr;
        }

        return node_ptr;
    }

    while ((NULL != node_ptr->parent_ptr) &&
           (node_ptr == node_ptr->parent_ptr->left_ptr)) {
        node_ptr = node_ptr->parent_ptr;
    }
    return node_ptr->parent_ptr;
}

/* == Local methods ======================================================== */

/* ------------------------------------------------------------------------- */
/** Will destroy (if given) each of the nodes. */
void bs_avltree_flush(bs_avltree_t *tree_ptr)
{
    bs_avltree_node_t         *node_ptr;
    bs_avltree_node_t         *parent_ptr;

    node_ptr = tree_ptr->root_ptr;
    while (NULL != node_ptr) {

        /* walk tree downwards as far as possible */
        if (NULL != node_ptr->left_ptr) {
            node_ptr = node_ptr->left_ptr;
            continue;
        }
        if (NULL != node_ptr->right_ptr) {
            node_ptr = node_ptr->right_ptr;
            continue;
        }

        /* store parent node, then clear bottom-most node */
        parent_ptr = node_ptr->parent_ptr;
        memset(node_ptr, 0, sizeof(bs_avltree_node_t));
        if (NULL != tree_ptr->destroy) {
            tree_ptr->destroy(node_ptr);
        }
        tree_ptr->nodes--;

        /* if the parent is NULL, we just flushed the root node. all done. */
        if (NULL == parent_ptr) break;

        /* in parent: mark the just-deleted node as gone */
        if (parent_ptr->left_ptr == node_ptr) {
            parent_ptr->left_ptr = NULL;
        } else {
            parent_ptr->right_ptr = NULL;
        }

        node_ptr = parent_ptr;
    }

    tree_ptr->root_ptr = NULL;
    BS_ASSERT(0 == tree_ptr->nodes);
}

/* ------------------------------------------------------------------------- */
/**
 * Replaces the node at |old_node_ptr| with |new_node_ptr| in the |tree_ptr|.
 */
void bs_avltree_node_replace(bs_avltree_t *tree_ptr,
                             bs_avltree_node_t *old_node_ptr,
                             bs_avltree_node_t *new_node_ptr)
{
    memcpy(new_node_ptr, old_node_ptr, sizeof(bs_avltree_node_t));
    memset(old_node_ptr, 0, sizeof(bs_avltree_node_t));
    if (NULL != tree_ptr->destroy) {
        tree_ptr->destroy(old_node_ptr);
    }

    /* update parent's (respectively root's) child node */
    if (NULL == new_node_ptr->parent_ptr) {
        tree_ptr->root_ptr = new_node_ptr;
    } else {
        if (new_node_ptr->parent_ptr->left_ptr == old_node_ptr) {
            new_node_ptr->parent_ptr->left_ptr = new_node_ptr;
        } else {
            new_node_ptr->parent_ptr->right_ptr = new_node_ptr;
        }
    }

    /* update children's parent node */
    if (NULL != new_node_ptr->left_ptr) {
        new_node_ptr->left_ptr->parent_ptr = new_node_ptr;
    }
    if (NULL != new_node_ptr->right_ptr) {
        new_node_ptr->right_ptr->parent_ptr = new_node_ptr;
    }
}

/* ------------------------------------------------------------------------- */
/** Returns the size of the subtree rooted at |node_ptr|. */
size_t bs_avltree_node_size(__UNUSED__ bs_avltree_t *tree_ptr,
                            bs_avltree_node_t *node_ptr) {
    if (NULL == node_ptr) return 0;

    return 1 +
        bs_avltree_node_size(tree_ptr, node_ptr->left_ptr) +
        bs_avltree_node_size(tree_ptr, node_ptr->right_ptr);
}

/* ------------------------------------------------------------------------- */
/** Returns the height of the subtree rooted at |node_ptr|. */
size_t bs_avltree_node_height(__UNUSED__ bs_avltree_t *tree_ptr,
                              bs_avltree_node_t *node_ptr) {
    size_t size_left = 0, size_right = 0;
    if (NULL != node_ptr->left_ptr) {
        size_left = bs_avltree_node_height(tree_ptr, node_ptr->left_ptr);
    }
    if (NULL != node_ptr->right_ptr) {
        size_right = bs_avltree_node_height(tree_ptr, node_ptr->right_ptr);
    }
    return size_left > size_right ? size_left + 1 : size_right + 1;
}

/* ------------------------------------------------------------------------- */
/**
 * Exchanges the position of |node1_ptr| and |node2_ptr| in the tree.
 *
 * @param tree_ptr
 * @param node1_ptr
 * @param node2_ptr
 *
 * @note This will not maintain the AVL tree integrity.
 */
void bs_avltree_node_exchange(bs_avltree_t *tree_ptr,
                              bs_avltree_node_t *node1_ptr,
                              bs_avltree_node_t *node2_ptr)
{
    bs_avltree_node_t         tmp_node;
    int                       pos1, pos2;

    /* Store relative position of the parents. We want to rebuild that. */
    if (NULL == node1_ptr->parent_ptr) {
        pos1 = 0;
    } else if (node1_ptr->parent_ptr->left_ptr == node1_ptr) {
        pos1 = -1;
    } else {
        BS_ASSERT(node1_ptr->parent_ptr->right_ptr == node1_ptr);
        pos1 = 1;
    }

    if (NULL == node2_ptr->parent_ptr) {
        pos2 = 0;
    } else if (node2_ptr->parent_ptr->left_ptr == node2_ptr) {
        pos2 = -1;
    } else {
        BS_ASSERT(node2_ptr->parent_ptr->right_ptr == node2_ptr);
        pos2 = 1;
    }

    memcpy(&tmp_node, node1_ptr, sizeof(bs_avltree_node_t));
    memcpy(node1_ptr, node2_ptr, sizeof(bs_avltree_node_t));
    memcpy(node2_ptr, &tmp_node, sizeof(bs_avltree_node_t));

    /* Catch the case where we just exchanged direct neighbors */
    if (node1_ptr->parent_ptr == node1_ptr) {
        node1_ptr->parent_ptr = node2_ptr;
    }
    if (node1_ptr->left_ptr == node1_ptr) {
        node1_ptr->left_ptr = node2_ptr;
    }
    if (node1_ptr->right_ptr == node1_ptr) {
        node1_ptr->right_ptr = node2_ptr;
    }
    if (node2_ptr->parent_ptr == node2_ptr) {
        node2_ptr->parent_ptr = node1_ptr;
    }
    if (node2_ptr->left_ptr == node2_ptr) {
        node2_ptr->left_ptr = node1_ptr;
    }
    if (node2_ptr->right_ptr == node2_ptr) {
        node2_ptr->right_ptr = node1_ptr;
    }

    /* update children's parent node(s) */
    if (NULL != node1_ptr->left_ptr) {
        node1_ptr->left_ptr->parent_ptr = node1_ptr;
    }
    if (NULL != node1_ptr->right_ptr) {
        node1_ptr->right_ptr->parent_ptr = node1_ptr;
    }

    if (NULL != node2_ptr->left_ptr) {
        node2_ptr->left_ptr->parent_ptr = node2_ptr;
    }
    if (NULL != node2_ptr->right_ptr) {
        node2_ptr->right_ptr->parent_ptr = node2_ptr;
    }

    /* update parent's child nodes */
    if (0 == pos2) {
        tree_ptr->root_ptr = node1_ptr;
    } else if (0 > pos2) {
        node1_ptr->parent_ptr->left_ptr = node1_ptr;
    } else {
        node1_ptr->parent_ptr->right_ptr = node1_ptr;
    }

    if (0 == pos1) {
        tree_ptr->root_ptr = node2_ptr;
    } else if (0 > pos1) {
        node2_ptr->parent_ptr->left_ptr = node2_ptr;
    } else {
        node2_ptr->parent_ptr->right_ptr = node2_ptr;
    }
}

/* ------------------------------------------------------------------------- */
void bs_avltree_node_delete(bs_avltree_t *tree_ptr,
                            bs_avltree_node_t *node_ptr)
{
    bs_avltree_node_t *next_larger_node_ptr = NULL;

    if ((NULL != node_ptr->left_ptr) && (NULL != node_ptr->right_ptr)) {
        // Node with two children. Replace with the min of the right-hand-side
        // subtree.
        next_larger_node_ptr = bs_avltree_node_min(node_ptr->right_ptr);
        bs_avltree_node_exchange(tree_ptr, node_ptr, next_larger_node_ptr);
    }

    // Being here: |node_ptr| has either no or one child(ren). If there is any
    // child, swap it with that one. Then, |node_ptr| is a leaf and easily
    // detached.
    bs_avltree_node_t *leaf_node_ptr =
        (node_ptr->left_ptr != NULL) ? node_ptr->left_ptr : node_ptr->right_ptr;
    if (NULL != leaf_node_ptr) {
        // swap it with the one to the right.
        bs_avltree_node_exchange(tree_ptr, node_ptr, leaf_node_ptr);
    }

    if (NULL == node_ptr->parent_ptr) {
        tree_ptr->root_ptr = NULL;
        tree_ptr->nodes--;
        memset(node_ptr, 0, sizeof(bs_avltree_node_t));
        return;
    }

    // Now we need to start backtracing...
    bs_avltree_node_t *parent_ptr = node_ptr->parent_ptr;
    bool removed_left = parent_ptr->left_ptr == node_ptr;

    // Now: Actually remove the node.
    if (removed_left) {
        parent_ptr->left_ptr = NULL;
    } else {
        parent_ptr->right_ptr = NULL;
    }
    memset(node_ptr, 0, sizeof(bs_avltree_node_t));

    do {
        // Adjust balance according to which side the removal happened.
        if (removed_left) {
            parent_ptr->balance++;
        } else {
            parent_ptr->balance--;
        }

        if (parent_ptr->balance == 0) {
            node_ptr = parent_ptr;
            parent_ptr = node_ptr->parent_ptr;
            if (NULL != parent_ptr) {
                removed_left = parent_ptr->left_ptr == node_ptr;
            }
            continue;
        }

        if (parent_ptr->balance == -2) {  // Leaning left.
            BS_ASSERT(parent_ptr->left_ptr != NULL);
            if (parent_ptr->left_ptr->balance > 0) {
                bs_avltree_rot_left(tree_ptr, parent_ptr->left_ptr);
                bs_avltree_rot_right(tree_ptr, parent_ptr);
            } else {
                BS_ASSERT(parent_ptr->left_ptr->left_ptr != NULL);
                if (bs_avltree_rot_right(tree_ptr, parent_ptr)) {
                    break;
                }
            }
        } else if (parent_ptr->balance == 2) {  // Leaning right.
            BS_ASSERT(parent_ptr->right_ptr != NULL);
            if (parent_ptr->right_ptr->balance < 0) {
                bs_avltree_rot_right(tree_ptr, parent_ptr->right_ptr);
                bs_avltree_rot_left(tree_ptr, parent_ptr);
            } else {
                BS_ASSERT(parent_ptr->right_ptr->right_ptr != NULL);
                if (bs_avltree_rot_left(tree_ptr, parent_ptr)) {
                    break;
                }
            }
        } else {
            // Balance is only one off, no furter re-tracing needed.
            break;
        }

        node_ptr = parent_ptr->parent_ptr;
        parent_ptr = node_ptr->parent_ptr;
        if (NULL != parent_ptr) {
            removed_left = parent_ptr->left_ptr == node_ptr;
        }
    } while (NULL != parent_ptr);

    tree_ptr->nodes--;
}


/* ------------------------------------------------------------------------- */
/**
 * Rotate tree right around node_ptr (D):
 *
 *     \?/         \?/
 *      D           B
 *     / \         / \  *
 *    B   E  ==>  A   D
 *   / \             / \  *
 *  A   C           C   E
 *
 * @return Whether this changed the subtree's height.
 */
bool bs_avltree_rot_right(bs_avltree_t *tree_ptr, bs_avltree_node_t *node_ptr)
{
    bs_avltree_node_t         *left_ptr;

    left_ptr = node_ptr->left_ptr;
    bool changed_height = 0 == left_ptr->balance;

    node_ptr->balance++;
    if (0 > left_ptr->balance) {
        node_ptr->balance -= left_ptr->balance;
    }

    left_ptr->balance++;
    if (0 < node_ptr->balance) {
        left_ptr->balance += node_ptr->balance;
    }

    /* establish D <-> C */
    node_ptr->left_ptr = left_ptr->right_ptr;
    if (NULL != node_ptr->left_ptr) {
        node_ptr->left_ptr->parent_ptr = node_ptr;
    }

    /* establish B to D's parent */
    left_ptr->parent_ptr = node_ptr->parent_ptr;
    if (NULL != left_ptr->parent_ptr) {
        if (left_ptr->parent_ptr->left_ptr == node_ptr) {
            left_ptr->parent_ptr->left_ptr = left_ptr;
        } else {
            left_ptr->parent_ptr->right_ptr = left_ptr;
        }
    } else {
        tree_ptr->root_ptr = left_ptr;
    }

    /* B -> D */
    left_ptr->right_ptr = node_ptr;
    node_ptr->parent_ptr = left_ptr;
    return changed_height;
}

/* ------------------------------------------------------------------------- */
/**
 * Rotate tree left around node_ptr (B):
 *
 *    \?/           \?/
 *     B             D
 *    / \           / \
 *   A   D   ==>   B   E
 *      / \       / \
 *     C   E     A   C
 *
 * @return Whether this changed the subtree's height.
 */
bool bs_avltree_rot_left(bs_avltree_t *tree_ptr, bs_avltree_node_t *node_ptr)
{
    bs_avltree_node_t         *right_ptr;

    right_ptr = node_ptr->right_ptr;
    bool changed_height = 0 == right_ptr->balance;

    node_ptr->balance--;
    if (0 < right_ptr->balance) {
        node_ptr->balance -= right_ptr->balance;
    }

    right_ptr->balance--;
    if (0 > node_ptr->balance) {
        right_ptr->balance += node_ptr->balance;
    }

    /* establish B <-> C */
    node_ptr->right_ptr = right_ptr->left_ptr;
    if (NULL != right_ptr->left_ptr) {
        right_ptr->left_ptr->parent_ptr = node_ptr;
    }

    /* establish D to B's parent */
    right_ptr->parent_ptr = node_ptr->parent_ptr;
    if (NULL != right_ptr->parent_ptr) {
        if (right_ptr->parent_ptr->left_ptr == node_ptr) {
            right_ptr->parent_ptr->left_ptr = right_ptr;
        } else {
            right_ptr->parent_ptr->right_ptr = right_ptr;
        }
    } else {
        tree_ptr->root_ptr = right_ptr;
    }

    /* D -> B */
    right_ptr->left_ptr = node_ptr;
    node_ptr->parent_ptr = right_ptr;
    return changed_height;
}

/* ------------------------------------------------------------------------- */
/**
 * Verifies the tree's consistency.
 */
void bs_avltree_verify_node(
    bs_avltree_t *tree_ptr,
    const void *(*key_from_node)(const bs_avltree_node_t *node_ptr),
    bs_avltree_node_t *node_ptr,
    const void *min_key_ptr,
    const void *max_key_ptr)
{
    const void* key_ptr;

    if (node_ptr == NULL) return;

    if (node_ptr->parent_ptr == NULL) {
        BS_ASSERT(tree_ptr->root_ptr == node_ptr);
    } else {
        BS_ASSERT((node_ptr == node_ptr->parent_ptr->left_ptr) ||
                  (node_ptr == node_ptr->parent_ptr->right_ptr));
    }

    if (NULL != min_key_ptr) {
        BS_ASSERT(tree_ptr->cmp(node_ptr, min_key_ptr) > 0);
    }
    if (NULL != max_key_ptr) {
        BS_ASSERT(tree_ptr->cmp(node_ptr, max_key_ptr) < 0);
    }

    if (NULL != key_from_node) {
        key_ptr = key_from_node(node_ptr);
    } else {
        key_ptr = NULL;
    }

    int height_left = 0, height_right = 0;

    if (NULL != node_ptr->left_ptr) {
        BS_ASSERT(node_ptr->left_ptr->parent_ptr == node_ptr);
        bs_avltree_verify_node(tree_ptr,
                               key_from_node,
                               node_ptr->left_ptr,
                               min_key_ptr,
                               key_ptr);
        height_left = bs_avltree_node_height(tree_ptr, node_ptr->left_ptr);
    }
    if (NULL != node_ptr->right_ptr) {
        BS_ASSERT(node_ptr->right_ptr->parent_ptr == node_ptr);
        bs_avltree_verify_node(tree_ptr,
                               key_from_node,
                               node_ptr->right_ptr,
                               key_ptr,
                               max_key_ptr);
        height_right = bs_avltree_node_height(tree_ptr, node_ptr->right_ptr);
    }

    BS_ASSERT(node_ptr->balance == height_right - height_left);

}

/* ------------------------------------------------------------------------- */
/**
 * @param node_ptr
 *
 * @return The minimum node of the given subtree.
 */
bs_avltree_node_t *bs_avltree_node_min(bs_avltree_node_t *node_ptr)
{
    if (NULL == node_ptr->left_ptr) {
        return node_ptr;
    }
    return bs_avltree_node_min(node_ptr->left_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * @param node_ptr
 *
 * @return The minimum node of the given subtree.
 */
bs_avltree_node_t *bs_avltree_node_max(bs_avltree_node_t *node_ptr)
{
    if (NULL == node_ptr->right_ptr) {
        return node_ptr;
    }
    return bs_avltree_node_max(node_ptr->right_ptr);
}

/* == Test Functions ======================================================= */
/** @cond TEST */

#define BS_AVLTREE_TEST_VALUES          4096
#define BS_AVLTREE_TEST_VALUE_MAX       3500

/** @internal */
typedef struct {
    bs_avltree_node_t         node;
    int                       value;
} bs_avltree_test_node_t;


static bs_avltree_test_node_t *bs_avltree_test_node_create(int value);
static void bs_avltree_test_node_destroy(bs_avltree_node_t *node_ptr);
static int bs_avltree_test_node_cmp(const bs_avltree_node_t *node_ptr,
                                    const void *key_ptr);
static const void *bs_avltree_test_node_key(const bs_avltree_node_t *node_ptr);

static void bs_avltree_test_random(bs_test_t *test_ptr);

const bs_test_case_t          bs_avltree_test_cases[] = {
    { 1, "random", bs_avltree_test_random },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
void bs_avltree_test_random(bs_test_t *test_ptr)
{
    char                      test_flag[BS_AVLTREE_TEST_VALUE_MAX];
    int                       random_values[BS_AVLTREE_TEST_VALUES];
    bs_avltree_test_node_t    *node_ptr;
    bs_avltree_t              *tree_ptr;
    int                       value_idx, value;
    int                       min_value, max_value;
    int                       outcome, expected_outcome;
    bool                      do_overwrite;
    size_t                    nodes;

    /* setup BS_AVLTREE_TEST_NUM random values. Multiple occurrences OK */
    srand(12345);
    memset(&random_values, 0, sizeof(random_values));
    max_value = 0;
    min_value = BS_AVLTREE_TEST_VALUE_MAX;
    for (value_idx = 0; value_idx < BS_AVLTREE_TEST_VALUES; value_idx++) {
        value = rand() % BS_AVLTREE_TEST_VALUE_MAX;
        random_values[value_idx] = value;
        if (min_value > value) min_value = value;
        if (max_value < value) max_value = value;
    }

    /* build tree. should better work */
    tree_ptr = bs_avltree_create(bs_avltree_test_node_cmp,
                                 bs_avltree_test_node_destroy);
    BS_TEST_VERIFY_NEQ(test_ptr, tree_ptr, NULL);

    /* run through all random values and add each one to the tree */
    nodes = 0;
    memset(&test_flag, 0, sizeof(test_flag));
    for (value_idx = 0; value_idx < BS_AVLTREE_TEST_VALUES; value_idx++) {
        value = random_values[value_idx];

        /* setup node */
        node_ptr = bs_avltree_test_node_create(value);
        node_ptr->value = value;
        BS_TEST_VERIFY_NEQ(test_ptr, tree_ptr, NULL);
        if (NULL == node_ptr) break;

        /* specific handling for multiple occurrences */
        switch (test_flag[value]) {
        case 0:
            /* first attempt: must insert */
            expected_outcome = true;
            nodes++;
            do_overwrite = false;
            break;

        case 1:
            /* second atttempt: overwrite = false, must reject */
            expected_outcome = false;
            do_overwrite = false;
            break;

        default:
            /* third+ attempt(s): overwrite = true, must insert */
            expected_outcome = true;
            do_overwrite = true;
            break;
        }
        test_flag[value]++;

        /* insert and verify outcome */
        outcome = bs_avltree_insert(
            tree_ptr, &node_ptr->value, &node_ptr->node, do_overwrite);

        BS_TEST_VERIFY_EQ(test_ptr, outcome, expected_outcome);
        BS_TEST_VERIFY_NEQ(test_ptr, tree_ptr->root_ptr, NULL);
        BS_TEST_VERIFY_EQ(test_ptr, tree_ptr->nodes, nodes);

        BS_ASSERT(tree_ptr->nodes == bs_avltree_node_size(
                      tree_ptr, tree_ptr->root_ptr));
        bs_avltree_verify_node(tree_ptr,
                               bs_avltree_test_node_key,
                               tree_ptr->root_ptr,
                               NULL,
                               NULL);

        /* clean up if node was rejected */
        if (!outcome) {
            bs_avltree_test_node_destroy(&node_ptr->node);
        }
    }
    BS_TEST_VERIFY_EQ(test_ptr, nodes, bs_avltree_size(tree_ptr));

    /* some lookup operations */
    for (value_idx = 0; value_idx < BS_AVLTREE_TEST_VALUES; value_idx++) {
        value = random_values[value_idx];

        node_ptr = (bs_avltree_test_node_t*)
            bs_avltree_lookup(tree_ptr, &value);

        BS_TEST_VERIFY_NEQ(test_ptr, node_ptr, NULL);
        if (NULL != node_ptr) {
            BS_TEST_VERIFY_EQ(test_ptr, node_ptr->value, value);
        }
    }

    /* Verify min & max operations */
    node_ptr = (bs_avltree_test_node_t*)bs_avltree_min(tree_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, node_ptr->value, min_value);
    node_ptr = (bs_avltree_test_node_t*)bs_avltree_max(tree_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, node_ptr->value, max_value);

    /* Step through the tree, verify it is strictly monotously increasing */
    value = 0;
    node_ptr = (bs_avltree_test_node_t*)bs_avltree_min(tree_ptr);
    do {
        while ((value < BS_AVLTREE_TEST_VALUE_MAX) &&
               (0 == test_flag[value])) {
            value++;
        }
        BS_TEST_VERIFY_EQ(test_ptr, value, node_ptr->value);

        node_ptr = (bs_avltree_test_node_t*)
            bs_avltree_node_next(tree_ptr, &node_ptr->node);
        value++;
    } while (NULL != node_ptr);

    while ((value < BS_AVLTREE_TEST_VALUE_MAX) &&
           (0 == test_flag[value])) {
        value++;
    }
    BS_TEST_VERIFY_EQ(test_ptr, value, BS_AVLTREE_TEST_VALUE_MAX);

    /* deletion operation. Remove half the nodes, the others to flush */
    for (value_idx = 0; value_idx < BS_AVLTREE_TEST_VALUES; value_idx++) {
        value = random_values[value_idx];

        node_ptr = (bs_avltree_test_node_t*)
            bs_avltree_delete(tree_ptr, &value);
        if (node_ptr != NULL) {
            BS_TEST_VERIFY_EQ(test_ptr, node_ptr->value, value);
            bs_avltree_test_node_destroy(&node_ptr->node);
        }

        BS_ASSERT(tree_ptr->nodes == bs_avltree_node_size(tree_ptr, tree_ptr->root_ptr));
        bs_avltree_verify_node(tree_ptr,
                               bs_avltree_test_node_key,
                               tree_ptr->root_ptr,
                               NULL,
                               NULL);
    }

    /* Attempt to delete a non-existing node. */
    value = BS_AVLTREE_TEST_VALUE_MAX;
    BS_ASSERT(NULL == bs_avltree_delete(tree_ptr, &value));

    bs_avltree_flush(tree_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, tree_ptr->root_ptr, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_avltree_size(tree_ptr));

    bs_avltree_destroy(tree_ptr);

    bs_test_succeed(test_ptr, "Operations with %d randomized nodes",
                    BS_AVLTREE_TEST_VALUES);
}


/* ------------------------------------------------------------------------- */
bs_avltree_test_node_t *bs_avltree_test_node_create(int value)
{
    bs_avltree_test_node_t    *node_ptr;

    node_ptr = calloc(1, sizeof(bs_avltree_test_node_t));
    if (NULL == node_ptr) return NULL;

    node_ptr->value = value;
    return node_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_avltree_test_node_destroy(bs_avltree_node_t *node_ptr)
{
    free(node_ptr);
}

/* ------------------------------------------------------------------------- */
int bs_avltree_test_node_cmp(const bs_avltree_node_t *node_ptr,
                             const void *key_ptr)
{
    const bs_avltree_test_node_t        *bs_node_ptr;
    const int                           *int_key_ptr;

    bs_node_ptr = (const bs_avltree_test_node_t*)node_ptr;
    int_key_ptr = (const int*)key_ptr;

    if (bs_node_ptr->value < *int_key_ptr) return -1;
    if (bs_node_ptr->value > *int_key_ptr) return 1;
    return 0;
}

/* ------------------------------------------------------------------------- */
const void *bs_avltree_test_node_key(const bs_avltree_node_t *node_ptr)
{
    const bs_avltree_test_node_t        *bs_node_ptr;

    bs_node_ptr = (const bs_avltree_test_node_t*)node_ptr;
    return &bs_node_ptr->value;
}

/** @endcond */
/* == End of avltree.c ===================================================== */
