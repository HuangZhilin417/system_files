
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "directory.h"
#include "pages.h"
#include "slist.h"
#include "util.h"
#include "inode.h"

#define ENT_SIZE 64

void
directory_init()
{
    inode* rn = get_inode(1);

    if (rn->mode == 0) {
        rn->size = 0;
        rn->mode = 040755;
    }
}

char*
directory_get(int ii)
{
    char* base = pages_get_page(1);
    return base + ii*ENT_SIZE;
}

int
directory_lookup(const char* name)
{
    for (int ii = 0; ii < 64; ++ii) {
        dirent* ent = (dirent*)directory_get(ii);
	printf("the looked: %s, the ent: %s\n", name, ent->name);
        if (streq(ent->name, name)) {
            return ent->inum;
        }
    }
    return -ENOENT;
}

int
tree_lookup(const char* path)
{
    assert(path[0] == '/');

    if (streq(path, "/")) {
        return 1;
    }

    return directory_lookup(path + 1);
}

int
directory_put(const char* name, int inum)
{
    dirent* ent = (dirent*) directory_get(inum);
    strlcpy(ent->name, name, 48);
    printf("+ dirent = '%s'\n", ent->name);

    inode* node = get_inode(inum);
    ent->inum = inum;
    printf("+ directory_put(..., %s, %d) -> 0\n", name, inum);
    print_inode(node);

    return 0;
}

int
directory_delete(const char* name)
{
    printf(" + directory_delete(%s)\n", name);

    int inum = directory_lookup(name);
    free_inode(inum);

    dirent* ent = (dirent*)directory_get(inum);
    memset(ent->name, 0, 48*sizeof(char));
    ent->inum = 0;

    return 0;
}

slist*
directory_list()
{
    printf("+ directory_list()\n");
    slist* ys = 0;

    for (int ii = 0; ii < 64; ++ii) {
        dirent* ent = (dirent*)directory_get(ii);
        if (ent) {
            printf(" - %d: %s [%d]\n", ii, ent->name, ii);
            ys = s_cons(ent->name, ys);
        }
    }

    return ys;
}

void
print_directory(inode* dd)
{
    printf("Contents:\n");
    slist* items = directory_list(dd);
    for (slist* xs = items; xs != 0; xs = xs->next) {
        printf("- %s\n", xs->data);
    }
    printf("(end of contents)\n");
    s_free(items);
}
