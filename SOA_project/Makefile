KPATH :=/lib/modules/$(shell uname -r)/build

obj-m += TBDE.o
TBDE-objs += setup.o
TBDE-objs += ./lib/syscalls/tag_get.o ./lib/syscalls/tag_send.o ./lib/syscalls/tag_receive.o ./lib/syscalls/tag_ctl.o
TBDE-objs += ./lib/core/vtpmo.o ./lib/core/sys_table_handler.o ./lib/core/service_list_handler.o ./lib/core/messages_list.o ./lib/core/sleep-wakeup-queue.o ./lib/core/dev_handler.o

all:
	make -C $(KPATH) M=$(PWD) modules
clean:
	make -C $(KPATH) M=$(PWD) clean
