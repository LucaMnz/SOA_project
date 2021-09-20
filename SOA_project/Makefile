KPATH :=/lib/modules/$(shell uname -r)/build

obj-m += TBDE.o
TBDE-objs += setup.o
TBDE-objs += ./lib/syscalls/tag_get.o ./lib/syscalls/tag_send.o ./lib/syscalls/tag_receive.o ./lib/syscalls/tag_ctl.o
TBDE-objs += ./lib/core/vtpmo.o ./lib/core/sys_table.o ./lib/core/service_list_handler.o ./lib/core/rcu_list.o ./lib/core/sleep-wakeup.o ./lib/core/dev.o

all:
	make -C $(KPATH) M=$(PWD) modules
clean:
	make -C $(KPATH) M=$(PWD) clean
