/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

/*
 * Put your function declarations and data types here ...
 */

// only instance of the global open_file_table
// struct of_t open_ft[__OPEN_MAX];


 /* FD Table - array of pointers
 - Each entry holds the address of the OF Entries

 struct of_t **fd_t[] = kmalloc(sizeof(...*maxthing));

 // OF Table - array of structs (contains a pointer and offset off_t fp)

 struct of_t *open_ft = kmalloc(sizeof(...*maxthing))

 */

struct of_t {
    off_t fp;
    struct vnode *vnode;
};
// structure of each container in the open file table. to be initialised later
extern struct of_t open_ft[];

// initialise global open file table
void create_open_ft(void);
// find free of
int find_free_of(struct of_t *open_ft);
// find free int fd in per-process fd
int find_free_fd(struct of_t **fd_t);
// implement sys_open
int sys_open(userptr_t filename, int flags, mode_t mode);



#endif /* _FILE_H_ */
