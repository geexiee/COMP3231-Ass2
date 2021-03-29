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



// declare global open file table
void create_open_ft() {
    //check if there is already an open file table
    if(open_ft == NULL) {
        struct of_t *open_ft = kmalloc(__OPEN_MAX*sizeof(struct of_t *));
    }
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
        if(open_ft->vnode == NULL) {
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
sys_open(userptr_t filename, int flags, mode_t mode)
{
    /*
    PSEUDO
    0. check flag mode
    1. Create Array for the process. -initiate to Null
    2. Initiate global array of fd in global or separate function? -initiate to Null
    3. Initiate file descriptor
    4. Have a find free file descriptor
    5. assign free file descriptor to point to an filepointer incrementor
    6.
    7. Linking the vnode
        - Pass in the filename into VOP to get a vnode
        - "vnode = vfs_open("file", ...)";
        - should return a vnode
    */
    int free_fd;
    int free_of;
    int vopen;


    struct vnode* vn;

    //check the flag mode
    bool rd = false;
    bool wr = false;
    bool rw = false;
    switch (flags & O_ACCMODE) {
	case O_RDONLY:
		rd = true;
		break;
	case O_WRONLY:
		wr = true;
		break;
	case O_RDWR:
		rw = true;
		break;
	default:
        // invalid flag input
		return EINVAL;
	};

    // initiate new per-process file descriptor -> char *array of size MAX
    struct of_t **fd_t = kmalloc(__OPEN_MAX*sizeof(struct of_t *));
    // initialise to NULL
    for(int i = 0; i < __OPEN_MAX; i++) {
        fd_t[i] = NULL;
    }
//check    // call helper function to create global open file table
    if (open_ft == NULL) create_open_ft();

    // assign free pointer in
    free_fd = find_free_fd(fd_t);

    free_of = find_free_of(open_ft);


    // access and store in vnode

    vopen = vfs_open((char *)filename, flags, mode, &vn);
    if (vopen) {
        return vopen;
    }

    // assign vnode of free Open file table
    open_ft[free_of].vnode = vn;
    fd_t[free_fd] = &open_ft[free_of];


    /*
    1. instantiate the open file table(s? both local and global?) (do in a separate function?)
    2. assign an entry in the local OFT to this call (fd is an index in the local OFT)
        a. might need to leave out 0,1,2?
    3. have that entry point to a struct in the global OFT which contains a file pointer
    and an unassigned vnode
        a. may need to instantiate the struct manually and
        give it an appropriate file pointer and empty vnode? (to be instantiated in vfs_open)
    4. use ACC_MODE mask to check with flags to determine the operation mode (to put into VFS_open)
    5. call VFS_open using the pathname, flags, mode, and retval(vnode which was
    just created in the global OFT).
        a. when accessing the vnode through the struct in the global OFT,
        use a lock for synchronisation
        b. calling this should instantiate the vnode so that it corresponds to the
        file we're trying to open (according to the flags)
    6. if VFS_open returns 0, the vnode was instantiated successfully
    7. release the lock
    8. free memory -> in sys_close
    */

    //WHAT DO I NEED to create?
    /*
    1. input arguments, what need to be passed in?
    2. Checks:
        - check if arguments passed in is not NULL
        - check if file exists in system
        -
    3. Handler:
        - Read, Write, APPEND Etc. (In the manual)
        - Error handling
        - Assign file handle (return int)
        - create process open_file_table
    */



    return 0;
}
