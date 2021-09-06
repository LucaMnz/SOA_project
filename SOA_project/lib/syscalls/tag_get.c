#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/ctype.h>

#include "../structure/properties.h"

/* Through this Syscall it's possible to create a process or 
   open it according to commands: CREATE or OPEN. Every process 
   has a permission which is needed to access; if a process was 
   created with IPC_PRIVATE it can be visible only for the creator 
   of the process. Permission to access to a specific tag service can be
   defined as ONLY_OWNER or ANY. */

extern int open_service(int, int);
extern int create_service(int, int);

unsigned long tag_get_addr(void);
EXPORT_SYMBOL(tag_get_addr);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(3, _tag_get, int, key, int, command, int, permission) {
#else
asmlinkage int sys_tag_get(int key, int command, int permission) {
#endif
   int ret;

   /* error check */
   if (permission != ONLY_OWNER && permission != ANY) {
      printk("%s: thread %d - 'permission' argument must be ONLY_OWNER (%d) or ANY (%d)", MODNAME, current->pid, ONLY_OWNER, ANY);
      return -1;
   }

   if (command == CREATE) {
      goto create;
   } else if (command == OPEN) {
      goto open;
   } else {
      printk("%s: thread %d - 'command' argument must be CMD_CREATE (%d) or CMD_OPEN (%d)\n", MODNAME, current->pid, CREATE, OPEN);
      return -1;
   }

create:

   /* IPC_PRIVATE = 0 */
   if (key < 0 || key > TAG_SERVICES_NUM) {
      printk("%s: thread %d - 'key' argument must be a positive integer between 1 and %d or IPC_PRIVATE\n", MODNAME, current->pid, TAG_SERVICES_NUM);
      return -1;
   }

   /* This function allocates an element in the service_list, with 'key' 
      as index and with 'permission' as visibility rule. 
      IPC_PRIVATE as 'key' can be used exclusively here to istantiate a
      service visible just for the creator process */
   ret = create_service(key, permission);
   return ret; /* key or -1 (error) */

open:

   if (key <= 0 || key > TAG_SERVICES_NUM) {
      printk("%s: thread %d - 'key' argument must be a positive integer between 1 and %d\n", MODNAME, current->pid, TAG_SERVICES_NUM);
      return -1;
   }

   /* This function opens an existing tag service with 'key' as index only
      if the calling thread has the correct privileges. 
      Here IPC_PRIVATE is not available. */
   ret = open_service(key, permission);
   return ret; /* tag or -1 (error) */
}

/* Useful for syscall installing */
unsigned long tag_get_addr(void) {
   return (unsigned long) __x64_sys_tag_get;
}

