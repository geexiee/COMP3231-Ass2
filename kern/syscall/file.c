#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>

// Initialiser function for per-process fd_table
void init_fd_table(void) {
    // initialising the fd_t for this process
    fd_table = kmalloc(__OPEN_MAX*sizeof(struct of_t *));
    // initialise each value in the table to NULL
    for(int i = 3; i < __OPEN_MAX; i++) {
        fd_table[i] = NULL;
    }

    char c1[] = "con:";
    int free_vnode_index = find_free_of(open_ft);
    struct vnode **vn = &open_ft[free_vnode_index].vnode;
    vfs_open(c1, O_WRONLY, 0, vn);

    char c2[] = "con:";
    int free_vnode_index2 = find_free_of(open_ft);
    struct vnode **vn2 = &open_ft[free_vnode_index2].vnode;
    vfs_open(c2, O_WRONLY, 0, vn2);

    fd_table[1] = &open_ft[free_vnode_index];
    fd_table[2] = &open_ft[free_vnode_index];
    kprintf("\ninitialised per-process fd_t\n");
}

// declare global open file table
void create_open_ft() {
    kprintf("Trying to initialise global of_t\n");
    // only runs if open_ft == NULL;
    open_ft = kmalloc(__OPEN_MAX*sizeof(struct of_t));
    // check if memory was allocated
    KASSERT(open_ft != NULL);

    //initialise to null
    for(int i = 0; i < __OPEN_MAX; i++) {
        open_ft[i].fp = 0;
        open_ft[i].vnode = NULL;
    }
    kprintf("Initialised global of_t\n");
}

int find_free_of(struct of_t *open_ft) {
    int ret = -1;
    // FD 0,1,2 is reserved for STDIN/STDOUT/STDERR
    for(int i = 3; i < __OPEN_MAX; i++) {
        if((open_ft+i)->vnode == NULL) {
            return i;
        }
    }
    return ret;
}

int find_free_fd(struct of_t **fd_t) {
    int ret = -1;
    // FD 0,1,2 is reserved for STDIN/STDOUT/STDERR
    for(int i = 3; i < __OPEN_MAX; i++) {
        if(fd_t[i] == NULL) {
            return i;
        }
    }
    return ret;
}


//Basic structure of syscall
int
sys_open(userptr_t filename, int flags, mode_t mode, int *err)
{
    int free_of;
    int free_fd;
    int vopen;
    char *kfilename;
    int flag = -1;   // keep track of the open flag

    struct vnode **vn = kmalloc(sizeof(struct vnode));

    // check input if valid
    if (filename == NULL) {
        *err = EFAULT;
        return -1;
    }
    //check the flag mode
    switch (flags & O_ACCMODE) {
	case O_RDONLY:
		flag = rd;
		break;
	case O_WRONLY:
		flag = wr;
		break;
	case O_RDWR:
        flag = rdwr;
		break;
	default:
        // invalid flag input
		*err = EINVAL;
        return -1;
	};

    // find free entry in fd table
    free_fd = find_free_fd(fd_table);

    if(free_fd < 0) {
        *err = EMFILE;
        return -1; //per-process table is full
    }

    free_of = find_free_of(open_ft);


    free_of = find_free_of(open_ft);
    if(free_of < 0) {
        *err = ENFILE;
        return -1; //global-open file table is full
    }

    // safely copy userspace filename to kernelspace-filename (don't need *done)
    kfilename = kmalloc(__NAME_MAX);
    *err = copyinstr(filename, kfilename, __NAME_MAX, NULL);
    if(*err != 0) {
        return -1; //returns error code: EFAULT (bad addr) or ENAMETOOLONG
    }

    // access and store address in vnode
    vopen = vfs_open(kfilename, flags, mode, vn);
    if (vopen < 0) {
        *err = ENOENT;
        return -1; //file does not exist
    }


    // assign values to the corresponding of_t entry
    open_ft[free_of].fp = 0;
    open_ft[free_of].vnode = *vn;
    open_ft[free_of].flag = flag;

    // make the fd point to the of_table
    fd_table[free_fd] = &open_ft[free_of];

    return free_fd;
}


// sys_read
ssize_t sys_read(int fd, void *buf, size_t buflen, ssize_t *err) {
    struct of_t *of = fd_table[fd];
    off_t fp = -1;
    int flag = -1;
    struct vnode *vn = NULL;

    if (of == NULL) {
        *err = EBADF;
        return -1;
    } else {
        flag = of->flag;
        vn = of->vnode;
        fp = of->fp;
    }

    if (flag != rdwr && flag != rd) {
        *err = EBADF;
        return -1;
    }

    if (buf == NULL) {
        *err = EFAULT;
        return -1;
    }

    struct iovec iov;
    struct uio u;

    uio_kinit(&iov, &u, buf, buflen, fp, UIO_READ); // initialising the iovec and uio to use in vop_read

    int result = VOP_READ(vn, &u);
    if (result != 0) {
        kprintf("fucked up the vop_read");
        *err = result;
        return -1;
    }
    ssize_t bytes_read = buflen - u.uio_resid;
    of->fp = bytes_read;

    return bytes_read;
}

// sys write
ssize_t sys_write(int fd, void *buf, size_t nbytes, ssize_t *err) {
    struct of_t *of = fd_table[fd];
    off_t fp = -1;
    struct vnode *vn = NULL;

    if (of == NULL) {
        kprintf("of = NULL, fd is %d\n", fd);
        *err = EBADF;
        return -1;
    } else {
        vn = of->vnode;
        fp = of->fp;
    }

    if (buf == NULL) {
        kprintf("buffer we're writing from is null\n");
        *err = EFAULT;
        return -1;
    }

    struct iovec iov;
    struct uio u;

    uio_kinit(&iov, &u, buf, nbytes, fp, UIO_WRITE); // initialising the iovec and uio to use in vop_read

    int result = VOP_WRITE(vn, &u);
    if (result != 0) {
        *err = result;
        kprintf("fucked up the vop_write\n");
        return -1;
    }

    int bytes_written = nbytes - u.uio_resid;
    of->fp = bytes_written;

    return bytes_written;
}



int
sys_close(int fd, int *err) {

    //return value
    int ret = 0;
    if (fd_table[fd] == NULL) {
        *err = EBADF; // if fd is not a valid file handle
        return 1;
    }

    struct of_t *curr = fd_table[fd];

    if(curr->vnode == NULL) {
        *err = EBADF; //did not close fd
        return -1;
    }

    // get the struct containing the address of the open_file
    // Pass the vnode in to close open file
    vfs_close(curr->vnode);

// LOCK so count is consistent
    // close open file for the fd
    curr->fp = 0;
    curr->vnode = NULL;
    fd_table[fd] = NULL;
// UNLOCK
    return ret;
}

int
sys_dup2(int oldfd, int newfd, int *err) {

//LOCK
//check if valid file directories
    if(fd_table[oldfd] == NULL || fd_table[newfd] == NULL) {
        *err = EBADF;
        return -1;
    }

kprintf("OLD->%d and NEW->%d are VALID FDs\n" ,oldfd, newfd);

    // check if dup2 calls pass in the same fd
    if(oldfd == newfd) {
        return oldfd;
    }
    // if new fd is open, then close it
    if(fd_table[newfd] != NULL) {
        if( (sys_close(newfd, err)) ) {
//UNLOCK
/*ERROR*/   return  -1; // could not free file if not null
        }
    }
    // Assign the newfd the pointer to the struct holding the vnode and file pointer
    fd_table[newfd] = fd_table[oldfd];
//UNLOCK
    return newfd;
}


off_t
sys_lseek(int fd, off_t offset, int whence, int *err) {

    /* PSEUDO
    checks:
    1. load in file, check if valid
    2. check if you have seek permissions -> VOP_ISSEEKABLE
    3. check flags valid -> fail otherwise
    3. --
    4. based on flag we increment offset
    5. if flag is seek_end start from end VOP_SEEKEND
    6. check if negative position (EINVAL
    */

    // position holders for old and new offset
    off_t newpos = 0;
    off_t oldpos = 0;

    // check whence flag valid SEEK_SET, SEEK_CUR, SEEK_END
    switch (whence) {
        case SEEK_SET:
        case SEEK_CUR:
        case SEEK_END:
            break;
        default:
            *err = EINVAL;
            return -1;
    }

//lock
    // load in file, check if valid
    if(fd_table[fd] == NULL) {
        *err = EBADF;
//unlock
        return -1;
    }
//unlock
    struct of_t *curr = fd_table[fd];

    // check if VOP_ISSEEKABLE, returns <0
    int isseek = VOP_ISSEEKABLE(curr->vnode);
    if(isseek < 0) {
        *err = ESPIPE; // object does not support seeking
//unlock
        return -1;
    }
    // set position
    newpos = curr->fp;
    oldpos = curr->fp;

    // flags checked, has to be one of the 3
    switch (whence) {
        case SEEK_SET:
            newpos = offset;
        case SEEK_CUR:
            newpos = oldpos + offset;
        case SEEK_END:
            newpos = VOP_SEEKEND(curr->node) + offset;
            break;
        default:
            *err = EINVAL;
            return -1;
    }
//unlock
    if(newpos < 0) {
        return EINVAL;
    }
    return newpos;
}
