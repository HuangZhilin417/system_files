
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <libgen.h>
#include <bsd/string.h>
#include <stdint.h>

#include "storage.h"
#include "slist.h"
#include "util.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"


int iptr_write(int iptr, const char* buf, size_t size, off_t offset);
int ptr_write(inode* node, int ptr, const char* buf, size_t size, off_t offset);
int iptr_read(int iptr, char* buf, size_t size, off_t offset);
int ptr_read(inode* node, int ptr, char* buf, size_t size, off_t offset);
	
	void
storage_init(const char* path, int create)
{
    //printf("storage_init(%s, %d);\n", path, create);
    pages_init(path, create);
    if (create) {
        directory_init();
    }
}

int
storage_stat(const char* path, struct stat* st)
{
    printf("+ storage_stat(%s)\n", path);
    int inum = tree_lookup(path);
    printf("the inum is: %d\n", inum);


    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    printf("+ storage_stat(%s); inode %d\n", path, inum);
    print_inode(node);

    memset(st, 0, sizeof(struct stat));
    st->st_uid   = getuid();
    st->st_mode  = node->mode;
    st->st_size  = node->size;
    st->st_nlink = node->refs;
    return 0;
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    int trv = storage_truncate(path, offset + size);
    if (trv < 0) {
        return trv;
    }

    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    int start_idx = offset /4096;
    off_t real_off = offset - (start_idx * 4096);

    if ((offset + size) > node->size) {
        if(grow_inode(node, offset + size - node->size) < 0){
		return -1;
        }
    }
    

    if(start_idx >= 2){
	    return iptr_read(node->iptr, buf, size, offset - 8192);
    }else{
	    return ptr_read(node, start_idx, buf, size, real_off);
    }
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
    int trv = storage_truncate(path, offset + size);
    if (trv < 0) {
        return trv;
    }

    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    int start_idx = offset /4096;
    off_t real_off = offset - (start_idx * 4096);

    if ((offset + size) > node->size) {
        if(grow_inode(node, offset + size - node->size) < 0){
		return -1;
        }
    }
    

    if(start_idx >= 2){
	    return iptr_write(node->iptr, buf, size, offset - 8192);
    }else{
	    return ptr_write(node, start_idx, buf, size, real_off);
    }

    
   }

int
ptr_write(inode* node, int ptr, const char* buf, size_t size, off_t offset){
	void* data = pages_get_page(node->ptrs[ptr]);
	if(offset + size < 4096){	
		memcpy(data + offset, buf, size);
		return 0;
	}else{
		size_t curr_size = 4096 - offset;
		memcpy(data + offset, buf, curr_size);
		if(ptr){
			return iptr_write(node->iptr, buf + curr_size, size - curr_size, 0);
		}else{
			return ptr_write(node, 1, buf + curr_size, size - curr_size, 0);
		}
	}

}


int
iptr_write(int iptr, const char* buf, size_t size, off_t offset){
	inode* node = get_inode(iptr);
    	uint8_t* data = pages_get_page(iptr);

    	int start_idx = offset /4096;
    	off_t real_off = offset - (start_idx * 4096);    

    	if(start_idx >= 2){
		return iptr_write(node->iptr, buf, size, offset - 8192);
   	}else{
		return ptr_write(node, start_idx, buf, size, real_off);
    	}

}

int
ptr_read(inode* node, int ptr, char* buf, size_t size, off_t offset){
	void* data = pages_get_page(node->ptrs[ptr]);
	if(offset + size < 4096){	
		memcpy(buf, data + offset, size);
		return 0;
	}else{
		size_t curr_size = 4096 - offset;
		memcpy(buf, data + offset, curr_size);
		if(ptr){
			return iptr_read(node->iptr, buf + curr_size, size - curr_size, 0);
		}else{
			return ptr_read(node, 1, buf + curr_size, size - curr_size, 0);
		}
	}

}


int
iptr_read(int iptr, char* buf, size_t size, off_t offset){
	inode* node = get_inode(iptr);
   	uint8_t* data = pages_get_page(iptr);

   	int start_idx = offset /4096;
    off_t real_off = offset - (start_idx * 4096);    

    if(start_idx >= 2){
		return iptr_read(node->iptr, buf, size, offset - 8192);
   	}else{
		return ptr_read(node, start_idx, buf, size, real_off);
    }

}

int
storage_truncate(const char *path, off_t size)
{	
	int rv = -1;
    	int inum = tree_lookup(path);
	inode* inode = get_inode(inum);
    	if (inode->size > size) {
      		rv = shrink_inode(inode, size);
   	}
    	else {
        	rv = grow_inode(inode, size);
    	}
	return rv;
}


int
storage_mknod(const char* path, int mode, int is_dir)
{  
    char* name = get_name(path);

    int    inum = alloc_inode(mode);
    inode* node = get_inode(inum);
    node->mode = mode;
    node->size = 0;



    printf("+ mknod did here\n");
    if (directory_lookup(node, name) != -ENOENT) {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    char* parent = get_parent(path);
    int parent_inum = tree_lookup(parent);
    inode* parentdir = get_inode(parent_inum);

    printf("+ mknod create %s [%04o] - #%d\n", path, mode, inum);

    return directory_put(parentdir, name, inum, is_dir);
}

int
storage_chmod(const char* path, mode_t mode){ 
    
    int inum = tree_lookup(path);
    if (inum >= 0) {
        inode* node = get_inode(inum);
	node->mode = mode;
	return 0;
    }else 
	return -1;

  

}


slist*
storage_list(const char* path)
{
    return list_all(path);
}

int
storage_unlink(const char* path)
{
    char* parent = get_parent(path);
    int inum = tree_lookup(parent);
    inode* node = get_inode(inum);
  
    const char* name = path + 1;
    return directory_delete(node, name);
}

int
storage_link(const char* from, const char* to)
{   int rv = -1;
    struct stat from_st;
    struct stat to_st;

    int from_rv = storage_stat(from, &from_st);
    if (from_rv == -ENOENT) {
        return from_rv;
    }

    int to_rv = storage_stat(to, &to_st);
    assert(to_rv == -ENOENT);
    if (to_rv != -ENOENT) {
        return to_rv;
    }
    char* parent = get_parent(from);
    int parentinum = tree_lookup(parent);
    int inum = tree_lookup(from);
    inode* parentnode = get_inode(parentinum);
    
    
    inode* node = get_inode(inum);
    node->refs += 1;
    int is_dir = 0;
    if(node->mode >= 040000){
        int is_dir = 1;
    }
    
    return directory_put(parentnode, to, inum, is_dir);
}

int
storage_rename(const char* from, const char* to)
{   
//    if (inum < 0) {
//        printf("mknod fail");
//        return inum;
//    }
    char* parent = get_parent(from);
    int parent_num = tree_lookup(parent);
    inode* parent_node = get_inode(parent_num);
    const char* old_name = get_name(from);
    const char* new_name = get_name(to);
    return change_directory_name(parent_node, old_name, new_name);
}

int
storage_set_time(const char* path, const struct timespec ts[2])
{
    // Maybe we need space in a pnode for timestamps.
    return 0;
}
