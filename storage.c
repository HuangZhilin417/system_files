
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
    st->st_nlink = 1;
    return 0;
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }
    inode* node = get_inode(inum);
    printf("+ storage_read(%s); inode %d\n", path, inum);
    print_inode(node);

    if (offset >= node->size) {
        return 0;
    }

    if (offset + size >= node->size) {
        size = node->size - offset;
    }

    uint8_t* data = pages_get_page(inum);
    printf(" + reading from page: %d\n", inum);
    memcpy(buf, data + offset, size);

    return size;
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
    uint8_t* data = pages_get_page(inum);
    printf("+ writing to page: %d\n", inum);
    memcpy(data + offset, buf, size);

    return size;
}


int
storage_truncate(const char *path, off_t size)
{
    int inum = tree_lookup(path);
    if (inum < 0) {
        return inum;
    }

    inode* node = get_inode(inum);
    node->size = size;
    return 0;
}

int
storage_mknod(const char* path, int mode, int is_dir)
{
    char* tmp1 = alloca(strlen(path));
    char* tmp2 = alloca(strlen(path));
    strcpy(tmp1, path);
    strcpy(tmp2, path);

    const char* name = path + 1;

    int    inum = alloc_inode(mode);
    inode* node = get_inode(inum);
    node->mode = mode;
    node->size = 0;

    if (directory_lookup(node, name) != -ENOENT) {
        printf("mknod fail: already exist\n");
        return -EEXIST;
    }

    char* parent = get_parent(path);
    int parent_inum = tree_lookup(parent);
    inode* parentdir = get_inode(parent_inum);
    free(parent);

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
    return directory_list(path);
}

int
storage_unlink(const char* path)
{
    char* parent = get_parent(path);
    int inum = tree_lookup(parent);
    inode* node = get_inode(inum);
    free(parent);
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
    char* parent = get_parent(from);
    int inum = tree_lookup(parent);
    inode* node = get_inode(inum);
    node->refs += 1;
    free(parent); 
    return directory_put(to, inum);
}

int
storage_rename(const char* from, const char* to)
{   
    if (inum < 0) {
        printf("mknod fail");
        return inum;
    }
    char* parent = get_parent(from);
    int parent_num = tree_lookup(parent);
    inode* parent_node = get_inode(parent_num);
    char* old_name = get_name(from);
    char* new_name = get_name(to);
    return change_directory_name(parent_node, old_name, new_name);
}

int
storage_set_time(const char* path, const struct timespec ts[2])
{
    // Maybe we need space in a pnode for timestamps.
    return 0;
}
