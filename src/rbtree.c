#include <stdio.h>
#include "rbtree.h"

#define RB_RED               (1u)
#define RB_BLACK             (0u)

#define rbt_is_red(node)     (RB_RED == (node)->color)
#define rbt_is_black(node)   (RB_BLACK == (node)->color)
#define rbt_set_red(node)    ((node)->color = RB_RED)
#define rbt_set_black(node)  ((node)->color = RB_BLACK)


/************************************** red black tree *********************************************/
/*  node after parent，copy node to pos */
void rb_link_node(rb_node *parent, rb_node *node, rb_node **pos) {
    node->parent = parent;
    node->left = node->right = NULL;
    node->color = RB_RED;
    *pos = node;
}

/**
 *       parent                       parent
 *        |                            |
 *        y    Right Rotate (y)        x      
 *       / \   ----------------->     / \   
 *      x   T3                      T1   y        
 *     / \      <--------------         / \
 *   T1   T2     Left Rotation(x)     T2   T3
 */
static void rbtree_right_rotate(rb_node **root, rb_node *y) {
    rb_node *x = y->left;
    rb_node *T2= x->right;
    rb_node *parent = y->parent;

    x->right = y;
    y->parent = x;

    y->left = T2;
    if (T2) T2->parent = y;

    x->parent = parent;
    if (parent) {
        if (y == parent->right)
            parent->right = x;
        else
            parent->left = x;
    } else
        *root = x;
}

static void rbtree_left_rotate(rb_node **root, rb_node *x) {
    rb_node *y = x->right;
    rb_node *T2= y->left;
    rb_node *parent = x->parent;

    y->left = x;
    x->parent = y;

    x->right = T2;
    if (T2) T2->parent = x;

    y->parent = parent;
    if (parent) {
        if (x == parent->left) 
            parent->left = y;
        else 
            parent->right = y;
    } else
        *root = y;
}

void rbt_insert(rb_node **root, rb_node *node) {
    rb_node *parent, *gparent;
    /* node->color = RB_RED; */
    while ((parent = node->parent) && rbt_is_red(parent)) {
        gparent = parent->parent;

        if (parent == gparent->left) {
            rb_node *uncle = gparent->right;
            /* case 1 */
            if (uncle && rbt_is_red(uncle)) {
                rbt_set_black(uncle);
                rbt_set_black(parent);
                rbt_set_red(gparent);
                node = gparent;
                continue;
            }
            /* case 2 */
            if (parent->right == node) {
                struct rb_node *tmp;
                rbtree_left_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }
            /* case 3 */
            rbt_set_black(parent);
            rbt_set_red(gparent);
            rbtree_right_rotate(root, gparent);
        } else {
            rb_node *uncle = gparent->left;
            if (uncle && rbt_is_red(uncle)) {
                rbt_set_black(uncle);
                rbt_set_black(parent);
                rbt_set_red(gparent);
                node = gparent;
                continue;
            }

            if (parent->left == node) {
                rb_node *tmp;
                rbtree_right_rotate(root, parent);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            rbt_set_black(parent);
            rbt_set_red(gparent);
            rbtree_left_rotate(root, gparent);
        }
    }

    rbt_set_black(*root);
}

