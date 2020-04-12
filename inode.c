
#include <stdint.h>
#include <assert.h>

#include "pages.h"
#include "inode.h"
#include "util.h"
#include "bitmap.h"

const int INODE_COUNT = 256;

int grow_iptr(inode* node, int size);
void shrink_one_page(inode* node);

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
		node->iptr = 0;
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
    int pages = bytes_to_pages(node->size);
    
    free_page(node->ptrs[0]);
    free_page(node->ptrs[1]);

    if(pages > 2){
	free_inode(node->iptr);
    }
	

    
    void* map = get_ibitmap();
    bitmap_put(map, inum, 0);
    return;
}

int
grow_inode(inode* node, int size){
	int current_size = node->size;
	int current_page = bytes_to_pages(current_size);
	int new_page = bytes_to_pages(current_size + size);
	int grow_page = new_page - current_page;
	if(grow_page == 0){
		node->size = size;
		return 0;
	}else{
		node->size = size;
		while(current_page < 2 && grow_page > 0){
			int page = alloc_page();
			assert(page != -1);
			node->ptrs[current_page] = page;
			current_page += 1;
			grow_page -= 1;
			size -= 4096;
		}
		if(grow_page > 0){
			node->size = 2 * 4096;
			node->iptr = grow_iptr(node, size);
			assert(node->iptr != -1);
		}
		return 0;
		

	}

}

int 
grow_iptr(inode* node, int size){
	int new_num = alloc_inode(node->mode);
	assert(new_num != -1);
	inode* new_node = get_inode(new_num);
	grow_inode(new_node, size);
	return new_num;
}


//gets the inode size, including the iptrs
int
get_isize(inode* node){
	if(node->iptr){
		inode* more_node = get_inode(node->iptr);
		return node->size + get_isize(more_node);
	}else{
		return node->size;
	}
}


int
shrink_inode(inode* node, int size) 
{	
	if (size > node->size) {
        return -1;
	}
	int current_size = node->size;
	int current_page = bytes_to_pages(current_size);
	int new_page = bytes_to_pages(current_size - size);
	int shrink_page = current_page - new_page;

	if(shrink_page == 0){
		node->size = size;
		return 0;
	}else{
		while(shrink_page){
			shrink_one_page(node);
			shrink_page -= 1;
    		}
		return 0;
	}



}

void
shrink_one_page(inode* node){
	if(node->iptr){
		inode* next_node = get_inode(node->iptr);
		if(get_isize(next_node) > 4096){
			shrink_one_page(next_node);
		}else{
			int size = 4096 - next_node->size;
		        node->size -= size;
			free_inode(node->iptr);
			node->iptr = 0;
		}
	}
	else if(node->ptrs[1]){
		free_page(node->ptrs[1]);
		node->ptrs[1] = 0;
		node->size -= 4096;
	}else if(node->ptrs[0]){
		free_page(node->ptrs[0]);
		node->ptrs[0] = 0;
		node->size = 0;
	}
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

