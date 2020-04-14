// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "pages.h"

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode(int mode);
void free_inode(int inum);
int get_isize(inode* node);
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
void* get_ibitmap();
//int inode_get_pnum(inode* node, int fpn);

#endif
