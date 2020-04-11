
#include <stdint.h>

#include "pages.h"
#include "inode.h"
#include "util.h"
#include "bitmap.h"

const int INODE_COUNT = 256;


void*
get_ibitmap()
{
    uint8_t* page = pages_get_page(0);
    return (void*)(page + 32);
}


inode*
get_inode(int inum)
{
    uint8_t* base = (uint8_t*) pages_get_page(1);
    inode* nodes = (inode*)(base);
    return &(nodes[inum]);
}

int
alloc_inode(int mode)
{
    void* map = get_ibitmap();
    for (int ii = 1; ii < INODE_COUNT; ++ii) {
	    if(!bitmap_get(map, ii)){
		bitmap_put(map, ii, 1);
		inode* node = get_inode(ii);
		node->refs = 1;
		node->mode = mode;
		node->size = 0;
		

		printf("+ alloc_inode() -> %d\n", ii);
		return ii;
		}

    }
    

    return -1;
}

void
free_inode(int inum)
{
    printf("+ free_inode(%d)\n", inum);

    inode* node = get_inode(inum);
    memset(node, 0, sizeof(inode));
}

void
print_inode(inode* node)
{
    if (node) {
        printf("node{mode: %04o, size: %d}\n",
               node->mode, node->size);
    }
    else {
        printf("node{null}\n");
    }
}

