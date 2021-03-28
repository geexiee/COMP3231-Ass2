/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <vnode.h>
#include <types.h>

/*
 * Put your function declarations and data types here ...
 */

// Global file descriptor table entry (array value)
// struct fd_t_entry {
//     ssize_t *fp; //filepointer
//     struct vnode *vnode; // vnode
// };

// Lock
// struct lock *global_fd_t_lock;

// Function for setting up global fd_t
// void instantiate_variables();

// int sys_open(const char *filename, int flags, mode_t mode, int *retval);

#endif /* _FILE_H_ */
