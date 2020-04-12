
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


int
directory_lookup(inode* dd, const char* name)
{
  void* directory = pages_get_page(dd->ptrs[0]);

    for (int ii = 0; ii < dd->size; ii += ENT_SIZE) {
        dirent* entry = (dirent*)(directory + ii);

        if (streq(entry->name, name)) {
            return entry->inum;
        }
    }
      	
      return -ENOENT;
}

int
tree_lookup(const char* path)
{
    if (strcmp(path, "/") == 0) {
        return 0;
    }

    slist* dir_list = directory_list(path);


    int inum = 0;
    while (dir_list != 0 && inum != -1 && dir_list->next) {
        inode* curr = get_inode(inum);
        inum = directory_lookup(curr, dir_list->data);
        dir_list = dir_list->next;
    }

  
    return inum;
   }

int
directory_put(inode* dd, const char* name, int inum)
{
  
   if (grow_inode(dd, sizeof(dirent)) != -1) {
        void* base = pages_get_page(dd->ptrs[0]);
    
        dirent* de = (dirent*)(base + (dd->size - sizeof(dirent)));
	char new_name[48];
	memset(name, '\0', sizeof(name));
   	strcpy(new_name, name);
	de->name = new_name;
        de->inum = inum;
        return 0;
    }
    
    return -1;
  }

int
directory_delete(inode* dd, const char* name)
{
    int rv = -1;
    int dirent_addr = -1;
    int dirent_inum = -1;

    void* directory = pages_get_page(dd->ptrs[0]);

    for (int i = 0; i < dd->size; i += sizeof(dirent)) {
        dirent* ent = (dirent*)(directory + i);

        if (streq(ent->name, name)) {
            dirent_addr = i;
            dirent_inum = ent->inum;
            break;
        }
    }
    inode* data = get_inode(dirent_inum);
    data->refs -= 1;
    if(data->refs == 0){
	free_inode(dirent_inum);
    }


    for(int ii = dirent_addr + sizeof(dirent); ii < dd->size; ii += sizeof(dirent)){
	dirent* curr = (dirent*)(directory + ii);
	dirent* prev = (dirent*)(directory + ii - sizeof(dirent));
	memset(prev->name, '\0', 48);
	strcpy(prev->name, curr->name);
	prev->inum = curr->inum;

    }
   return shrink_inode(dd, sizeof(dirent));
    

   
   }

slist*
directory_list(const char* path)
{
    slist* list = 0;
    int i = 0;
    int n = 0;
    char hi[48];
    char ch;
    int slash = 0;
    int length = strlen(path);
    while (i < length){
        ch = path[i];
        hi[n] = ch;
        n++;
        if(ch == '/' || i == length - 1){
            if(i == length - 1){
                n++;
            }
            slash = i;
            char data[48];
            memcpy(data, hi, n - 1);
            data[n - 1] = '\0';
            list = s_cons(data, list);
            memset(hi, '\0', sizeof(hi));
            n = 0;
        }
    i++;
    }

    return s_reverse(list);
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
