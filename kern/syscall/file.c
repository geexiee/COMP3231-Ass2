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
    for(int i = 0; i < __OPEN_MAX; i++) {
        fd_table[i] = NULL;
    }
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

int find_free_fd(struct of_t **fd_table) {

    int ret = -1;
    // FD 0,1,2 is reserved for STDIN/STDOUT/STDERR
    for(int i = 3; i < __OPEN_MAX; i++) {
        if(fd_table[i] == NULL) {
            return i;
        }
    }
    return ret;
}


//Basic structure of syscall
int
sys_open(userptr_t filename, int flags, mode_t mode)
{
    /*
    PSEUDO
    0. check flag mode
    1. find free file descriptor
    2. assign free file descriptor to point to an open file in the of_t
    3. initialise vavlues in the openfile entry
    4. call vfs_open
    7. Linking the vnode
        - Pass in the filename into VOP to get a vnode
        - "vnode = vfs_open("file", ...)";
        - should return a vnode
    */

    int free_of;
    int free_fd;
    int vopen;
    char *kfilename;
    int err;
    int flag = -1;   // keep track of the open flag

    struct vnode **vn = kmalloc(sizeof(struct vnode));

    // check input if valid
    if (filename == NULL) {
        return EFAULT;
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
		return EINVAL;
	};

kprintf("flag is: %d (0 = rd, 1 = wr, 2 = rdwr) \n", flag);

    // find free entry in fd table
    free_fd = find_free_fd(fd_table);

kprintf("free fd is %d\n", free_fd);

    if(free_fd < 0) {
        return EMFILE; //per-process table is full
    }

    free_of = find_free_of(open_ft);

kprintf("free of is %d\n", free_of);
kprintf("assigned fd_t entry no.%d to of_t entry no.%d\n", free_fd, free_of);


    free_of = find_free_of(open_ft);
    if(free_of < 0) {
        return ENFILE; //global-open file table is full
    }

    // safely copy userspace filename to kernelspace-filename (don't need *done)
    kfilename = kmalloc(__NAME_MAX);
    err = copyinstr(filename, kfilename, __NAME_MAX, NULL);
    if(err != 0) {
        return err; //returns error code: EFAULT (bad addr) or ENAMETOOLONG
    }

    // access and store address in vnode
    vopen = vfs_open(kfilename, flags, mode, vn);
    if (vopen < 0) {
        return ENOENT; //file does not exist
    }


    // assign values to the corresponding of_t entry
    open_ft[free_of].fp = 0;
    open_ft[free_of].vnode = *vn;
    open_ft[free_of].flag = flag;
    
    // make the fd point to the of_table
    fd_table[free_fd] = &open_ft[free_of];
kprintf("struct addr1: %p\n", fd_table[free_fd]);
kprintf("struct addr2: %p\n", fd_table[0]);

    return free_fd;
}


// // sys_read
// ssize_t sys_read(int fd, void *buf, size_t buflen) {
//     struct of_t *of = fd_table[fd];
//     off_t fp = -1;
//     int flag = -1;
//     struct vnode *vn = NULL;


//     if (of == NULL) {
//         return EBADF;
//     } else {
//         int flag = of->flag;
//         vn = of->vnode;
//         fp = of->fp;
//     }

//     if (flag != rdwr && flag != rd) {
//         return EBADF;
//     }

//     if (buf == NULL) {
//         return EFAULT;
//     }

//     // create uio 
//     // create iovec
//     // init both using uio_kinit
//     // pass uio block in vop_read

//     struct addrspace as = as_create();
//     struct iovec curr_iovec = 
//     struct uio curr_uio = kmalloc(sizeof(struct uio));

//     int ret = vn->vn_ops->vop_read(vn, )    

//     return 0;
    
//         error checking:
//         a. not valid fd
//         b. buf is invalid
//         c. hardware error

//         0. get the vnode from the fd 
//             a. traverse through the fd_t and of_t
//         1. vop read that shit
//         2. move the offset by the number of bytes
//         3. return count of bytes
    
   


// }