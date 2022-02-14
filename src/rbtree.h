#ifndef __RBTREE_H__
#define __RBTREE_H__

#define RB_TREE_ENTRY(ptr, type, member)                           \
        ((type *)((char *)ptr - (size_t)&((type *)0)->member))

typedef struct rb_node rb_node;

struct rb_node {
    rb_node *left;
    rb_node *right;
    rb_node *parent;
    int      color;
};

void rbt_insert(rb_node **root, rb_node *node);
void rb_link_node(rb_node *parent, rb_node *node, rb_node **pos);

#endif
