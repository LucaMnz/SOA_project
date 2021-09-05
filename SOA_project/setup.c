#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>

#include "include/sys_table_handler.h"
#include "include/service_list_handler.h"
#include "include/dev_handler.h"

/* Setup module */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Menzolini");
MODULE_DESCRIPTION("TAG-based data exchange. A message is sent/received according to tag+level matching");

extern void get_syscalls_addresses(void);

extern int install_syscalls(void);
extern int uninstall_syscalls(void);

extern int service_list_alloc(void);
extern void service_list_dealloc(void);

extern int register_dev(void);
extern void unregister_dev(void);

/* Inizialization function */
static int __init install(void) {
   service_list_alloc();
   get_syscalls_addresses();
   install_syscalls();
   register_dev();
   return 0;
}

static void __exit uninstall(void) {
   uninstall_syscalls();
   service_list_dealloc();
   unregister_dev();
}

module_init(install);
module_exit(uninstall);

