// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME 48

#include "slist.h"
#include "pages.h"
#include "inode.h"

typedef struct dirent {
    char name[DIR_NAME];
    int  inum;
    int  is_dir;
    char _reserved[8];
} dirent;

//init the root dir, which is inode 0;
void directory_init();

//given a inode, which is a directory, it will search through the directory and see if that name exist
int directory_lookup(inode* dd, const char* name);

//given a path, it will look through the path given, and see if that file exist, if it does it will return the inode number for the last item in the path
int tree_lookup(const char* path);

//given a inode, which is the directory, it will put the name of the file or directory with the inum to the the inode directory given, and the is_dir indicates if the new file that is getting put in the inode directory is a file or a directory 
int directory_put(inode* dd, const char* name, int inum, int is_dir);

//deletes a file in the given inode directory, if refs become zero then that files data block also gets deleted
int directory_delete(inode* dd, const char* name);

//put the path into slist
slist* directory_list(const char* path);

//prints all the stuff in the directory and also prints nested
//directory
void print_directory(inode* dd);

#endif

