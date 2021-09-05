#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#include "../include/properties.h"

/* Through this Syscall it's possible to make a thread
   send a message to a specific tag service at a certain
   level */

extern int send_message(int, int, char *, size_t);

unsigned long tag_send_addr(void);
EXPORT_SYMBOL(tag_send_addr);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(4, _tag_send, int, tag, int, level, char *, buffer, size_t, size) {
#else
asmlinkage int sys_tag_send(int tag, int level, char *buffer, size_t size) {
#endif
   int ret, err;

   /* error check */
   if (tag < 0 || tag > (TAG_SERVICES_NUM-1)) {
      printk("%s: thread %d - the specified tag is not valid\n", MODNAME, current->pid);
      return -1;
   }

   if (level < 0 || level >= LEVELS) {
      printk("%s: thread %d - the specified level is not valid\n", MODNAME, current->pid);
      return -1;
   }

   char kern_buffer[size];
   int dim = sizeof(kern_buffer) / sizeof(kern_buffer[0]);

   /* The user level buffer data is copied in the kernel level buffer */
   err = copy_from_user(kern_buffer, buffer, size);
   if (err == -1) {
      printk("%s: thread %d - error in copying data\n", MODNAME, current->pid);
      return -1;
   }

   if ((dim < (int) size) || (size > MAX_MSG_SIZE) || (dim > MAX_MSG_SIZE)) {
      printk("%s: thread %d - the size of the message must be lower or equal than buffer size and lower than %d byte\n", MODNAME, current->pid, MAX_MSG_SIZE);
      return -1;
   }

   /* Message sending to the specified service at the specified level */
   ret = send_message(tag+1, level, kern_buffer, size);

   return ret;
}

unsigned long tag_send_addr(void) {
   return (unsigned long) __x64_sys_tag_send;
}
