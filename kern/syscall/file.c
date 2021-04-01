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
int init_fd_table(void) {
    // initialising the fd_t for this process
    fd_table = kmalloc(__OPEN_MAX*sizeof(struct of_t *));
    // initialise each value in the table to NULL
    for(int i = 3; i < __OPEN_MAX; i++) {
        fd_table[i] = NULL;
    }

    char c1[] = "con:";
    char c2[] = "con:";
   
    struct vnode **vn = kmalloc(sizeof(struct vnode *));
    struct vnode **vn2 = kmalloc(sizeof(struct vnode *));

    int ret = vfs_open(c1, O_WRONLY, 0, vn);
    if (ret) return ret; // error checking
    int ret2 = vfs_open(c2, O_WRONLY, 0, vn2);
    if (ret2) return ret2; // error checking

    struct of_t of1 = {O_WRONLY, 0, *vn};
    struct of_t of2 = {O_WRONLY, 0, *vn};

    open_ft[1] = of1;
    open_ft[2] = of2;

    fd_table[1] = &open_ft[1];
    fd_table[2] = &open_ft[2];
    return 0;
}

// declare global open file table
void create_open_ft() {
    // only runs if open_ft == NULL;
    open_ft = kmalloc(__OPEN_MAX*sizeof(struct of_t));
    // check if memory was allocated
    KASSERT(open_ft != NULL);

    //initialise to null
    for(int i = 0; i < __OPEN_MAX; i++) {
        open_ft[i].fp = 0;
        open_ft[i].vnode = NULL;
    }
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
		flag = O_RDONLY;
		break;
	case O_WRONLY:
		flag = O_WRONLY;
		break;
	case O_RDWR:
        flag = O_RDWR;
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
    if (vopen != 0) {
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

    if (flag != O_RDWR && flag != O_RDONLY) {
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
        *err = result;
        return -1;
    }

    off_t bytes_read = buflen - u.uio_resid;
    of->fp = bytes_read;
    
    return bytes_read;
}

// sys write
ssize_t sys_write(int fd, void *buf, size_t nbytes, ssize_t *err) {
    struct of_t *of = fd_table[fd];
    int flag = -1;
    off_t fp = -1;
    struct vnode *vn = NULL;

    if (of == NULL) {
        *err = EBADF;
        return -1;
    } else {
        vn = of->vnode;
        fp = of->fp;
        flag = of->flag; 
    }

    if (flag != O_RDWR && flag != O_WRONLY) {
        *err = EBADF;
        return -1;   
    }

    if (buf == NULL) {
        *err = EFAULT;
        return -1;
    }

    struct iovec iov;
    struct uio u;

    uio_kinit(&iov, &u, buf, nbytes, fp, UIO_WRITE); // initialising the iovec and uio to use in vop_read

    int result = VOP_WRITE(vn, &u);
    if (result != 0) {
        *err = result;
        return -1;
    }

    off_t bytes_written = nbytes - u.uio_resid;
    of->fp = bytes_written;

    return bytes_written;
}



int
sys_close(int fd, int *err) {
    int ret = 0;

    if (fd >= __OPEN_MAX || fd < 0) {
        *err = EBADF;
        return -1;
    }

    if (fd_table[fd] == NULL) {
        *err = EBADF; // if fd is not a valid file handle
        return -1;
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
    // Accessing STDIN should give ESPIPE (unseekable)
    if(fd == 0) {
        *err = ESPIPE;
        return -1;
    }
//unlock
    struct of_t *curr = fd_table[fd];

    // check if VOP_ISSEEKABLE, returns <0
    int isseek = VOP_ISSEEKABLE(curr->vnode);
    if(!isseek) {
        *err = ESPIPE; // object does not support seeking
//unlock
        return -1;
    }
    // set position
    newpos = curr->fp;
    oldpos = curr->fp;

    struct stat *s = kmalloc(sizeof(struct stat));

    // flags checked, has to be one of the 3
    switch (whence) {
        case SEEK_SET:
            newpos = offset;
            break;
        case SEEK_CUR:
            newpos = oldpos + offset;
            break;
        case SEEK_END:
            //USE VOP_STAT to return file size, t(basically end offset of file)
            VOP_STAT(curr->vnode, s);
            newpos = s->st_size + offset;

            break;
        default:
            *err = EINVAL;
            return -1;
    }
    //set new offset into table!
    fd_table[fd]->fp = newpos;
    kfree(s);
//unlock
    // check if newpos is referencing negative
    if(newpos < 0) {
        *err = EINVAL;
       return -1;
    }
    return newpos;
}
