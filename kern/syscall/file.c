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

/*
 * Add your file-related functions here ...
 */

// Instantiating global fd_t and lock
struct lock global_fd_t_lock = lock_create("global_lock");
// Instantiating global fd_t array(?)
// struct fd_t_entry {
    // ssize_t *fp; //filepointer
    // struct vnode *vnode; // vnode
// };

/*
struct fd_t_entry *global_fd_t = kmalloc(__OPEN_MAX*sizeof(fd_t_entry));

if (global_fd_t == NULL) {
    panic("couldn't create the global fd_t :(");
}
*/

// int sys_open(const char *filename, int flags, mode_t mode, int *retval) {
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
// }
