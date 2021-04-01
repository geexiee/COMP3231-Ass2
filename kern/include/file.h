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
int init_fd_table(void);
void create_open_ft(void);
int find_free_of(struct of_t *open_ft);
int find_free_fd(struct of_t **fd_t);

// syscalls
int sys_open(userptr_t filename, int flags, mode_t mode, int *err);
ssize_t sys_read(int fd, void *buf, size_t buflen, ssize_t *err);
ssize_t sys_write(int fd, void *buf, size_t nbytes, ssize_t *err);
int sys_close (int fd, int *err);
int sys_dup2(int oldfd, int newfd, int *err);
off_t sys_lseek(int fd, off_t offset, int whence, int *err);

#endif /* _FILE_H_ */