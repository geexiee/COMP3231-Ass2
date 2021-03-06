/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>

// The fd_t for the process, an array of pointers to of entries
#define fd_table (curproc->fd_t)

// the values in the global of_t
struct of_t {
    int flag; // keep track of if its rd(0) wr(1) or rdwr(2)
    off_t fp;
    struct vnode *vnode;
};

// array of open files (global ft), initialised in main
struct of_t *open_ft;
int init_fd_table(void); // initialise per process file-descriptor table
void create_open_ft(void); // initialise global open-files table
int find_free_of(struct of_t *open_ft); // find next free open file slot in table
int find_free_fd(struct of_t **fd_t); // find next free fd slot in table
void destroy_fd_table(void); // shutdown per process file-descriptor table
void destroy_open_ft(void); // shutdown global open-files table


// syscalls
int sys_open(userptr_t filename, int flags, mode_t mode, int *err);
ssize_t sys_read(int fd, void *buf, size_t buflen, ssize_t *err);
ssize_t sys_write(int fd, void *buf, size_t nbytes, ssize_t *err);
int sys_close (int fd, int *err);
int sys_dup2(int oldfd, int newfd, int *err);
off_t sys_lseek(int fd, off_t offset, int whence, int *err);


#endif /* _FILE_H_ */
