#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <syscall.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../lib/structure/properties.h"

#define TAG_GET 134
#define TAG_SEND 174
#define TAG_RECEIVE 182
#define TAG_CTL 183

/* This module provide a simple main function to test
   the syscalls added */

typedef struct _test_args{
        int key;
        int perm;
} test_args;

typedef struct _msg_test_args{
        int tag;
        int level;
        char *buff;
        size_t size;
} msg_test_args;

typedef struct _ctl_test_args{
        int tag;
} ctl_test_args;

int fd;

void awakening_test();
void removing_test();
void *create_service(void *params);
void *open_service(void *params);
void *receive_message(void *params);
void *send_message(void *params);
void *remove_service(void *params);
void *awake_threads(void *params);

void creation_test() {
   pthread_t tid0, tid1, tid2, tid3, tid4, tid5;

   test_args args1 = {.key = 25, .perm = ANY};
   pthread_create(&tid0, NULL, create_service, (void *) &args1);

   test_args args2 = {.key = 50, .perm = ANY};
   pthread_create(&tid2, NULL, create_service, (void *) &args2);

   test_args args3 = {.key = 75, .perm = ONLY_OWNER};
   pthread_create(&tid3, NULL, create_service, (void *) &args3);

   test_args args5 = {.key = IPC_PRIVATE, .perm = ANY};
   pthread_create(&tid5, NULL, create_service, (void *) &args5);
   
   test_args args4 = {.key = 300, .perm = ANY};
   pthread_create(&tid4, NULL, create_service, (void *) &args4);
   
   pthread_create(&tid1, NULL, create_service, (void *) &args1);
   
   printf("It' not allowed to create two service with the same tag and/or choose a number greater than 256.\n");

   pthread_join(tid0, NULL);
   pthread_join(tid1, NULL);
   pthread_join(tid2, NULL);
   pthread_join(tid3, NULL);
   pthread_join(tid4, NULL);
   pthread_join(tid5, NULL);

}

void opening_test() {
   pthread_t tid0, tid1, tid2, tid3;

   test_args args1 = {.key = 25, .perm = ANY};
   pthread_create(&tid0, NULL, open_service, (void *) &args1);

   test_args args2 = {.key = 50, .perm = ANY};
   pthread_create(&tid1, NULL, open_service, (void *) &args2);

   test_args args3 = {.key = 75, .perm = ANY};
   pthread_create(&tid2, NULL, open_service, (void *) &args3);

   test_args args4 = {.key = 100, .perm = ANY};
   pthread_create(&tid3, NULL, open_service, (void *) &args4);
   
   printf("It's not possible to open a service with a tag who doesn't exist.\n");

   pthread_join(tid0, NULL);
   pthread_join(tid1, NULL);
   pthread_join(tid2, NULL);
   pthread_join(tid3, NULL);

}

void removing_test() {
   pthread_t tid0, tid1;

   ctl_test_args args1 = {.tag = 49};
   pthread_create(&tid0, NULL, remove_service, (void *) &args1);

   ctl_test_args args2 = {.tag = 24};
   pthread_create(&tid1, NULL, remove_service, (void *) &args2);

   pthread_join(tid0, NULL);
   pthread_join(tid1, NULL);

}

void awakening_test() {
   pthread_t tid0, tid1;

   ctl_test_args args1 = {.tag = 24};
   pthread_create(&tid0, NULL, awake_threads, (void *) &args1);

   ctl_test_args args2 = {.tag = 49};
   pthread_create(&tid1, NULL, awake_threads, (void *) &args2);

   pthread_join(tid0, NULL);
   pthread_join(tid1, NULL);
}

void *create_service(void *params) {
   int ret;
   test_args *args = (test_args *) params;

   ret = syscall(TAG_GET, args->key, CREATE, args->perm);

   if (ret == -1) {
      printf("Thread %ld: service %d creation failed\n", syscall(SYS_gettid), args->key);
   } else {
      printf("Thread %ld: service %d creation succeed\n", syscall(SYS_gettid), args->key);
   }
}

void *open_service(void *params) {
   int ret;
   test_args *args = (test_args *) params;

   ret = syscall(TAG_GET, args->key, OPEN, args->perm);

   if (ret == -1) {
      printf("Thread %ld: service %d opening failed\n", syscall(SYS_gettid), args->key);
   } else {
      printf("Thread %ld: service %d opening succeed\n", syscall(SYS_gettid), args->key);
   }

}

void *receive_message(void *params) {
   int ret;
   msg_test_args *args = (msg_test_args *) params;
   
   printf("Thread %ld: tag %d start receiving at level %d\n", syscall(SYS_gettid), args->tag + 1, args->level);

   ret = syscall(TAG_RECEIVE, args->tag, args->level, args->buff, args->size);

   if (ret == -1) {
      printf("Thread %ld: tag %d receiving at level %d failed\n", syscall(SYS_gettid), args->tag + 1, args->level);
   } else {
      printf("Thread %ld: tag %d receiving message '%s' at level %d succeed (%ld bytes received)\n", syscall(SYS_gettid), args->tag + 1, args->buff, args->level, args->size);
   }

}

void *send_message(void *params) {
   int ret;
   msg_test_args *args = (msg_test_args *) params;
   
   printf("Thread %ld: tag %d start sending at level %d\n", syscall(SYS_gettid), args->tag + 1, args->level);

   ret = syscall(TAG_SEND, args->tag, args->level, args->buff, args->size);

   if (ret == -1) {
      printf("Thread %ld: tag %d sending at level %d failed\n", syscall(SYS_gettid), args->tag + 1, args->level);
   } else {
      printf("Thread %ld: tag %d sending message '%s' at level %d succeed (%ld bytes sent)\n", syscall(SYS_gettid), args->tag + 1, args->buff, args->level, args->size);
   }

}

void *awake_threads(void *params) {
   int ret;
   ctl_test_args *args = (ctl_test_args *) params;

   ret = syscall(TAG_CTL, args->tag, AWAKE_ALL);

   if (ret == -1) {
      printf("Thread %ld: threads awakening on tag %d failed\n", syscall(SYS_gettid), args->tag + 1);
   } else {
      printf("Thread %ld: threads awakening on tag %d succeed\n", syscall(SYS_gettid), args->tag + 1);
   }

}

void *remove_service(void *params) {
   int ret;
   ctl_test_args *args = (ctl_test_args *) params;

   ret = syscall(TAG_CTL, args->tag, REMOVE);

   if (ret == -1) {
      printf("Thread %ld: service with tag %d removal failed\n", syscall(SYS_gettid), args->tag + 1);
   } else {
      printf("Thread %ld: service with tag %d removal  succeed\n", syscall(SYS_gettid), args->tag + 1);
   }

}


void read_status() {
   int ret;

   char *buff = (char *) malloc(8192);

   ret = read(fd, buff, 8192);

   printf("%s\n", buff);

}

void msg_test() {
   pthread_t tid0, tid1, tid2, tid3, tid4, tid5;
   char *buff1 = malloc(32);
   char *buff2 = malloc(32);
   char *buff3 = malloc(32);
   char *buff4 = malloc(32);

   printf("/------- Testing sys_tag_receive/sys_tag_send -------/\n");
   printf("It's not possible to start receiving/sending at a service which doesn't exists.\n");
   msg_test_args args1 = {.tag = 24, .level = 6, .buff = buff1, .size = 32};
   pthread_create(&tid0, NULL, receive_message, (void *) &args1);

   msg_test_args args2 = {.tag = 57, .level = 12, .buff = buff2, .size = 32};
   pthread_create(&tid1, NULL, receive_message, (void *) &args2);

   msg_test_args args3 = {.tag = 49, .level = 2, .buff = buff3, .size = 32};
   pthread_create(&tid2, NULL, receive_message, (void *) &args3);

   msg_test_args args4 = {.tag = 49, .level = 2, .buff = buff4, .size = 32};
   pthread_create(&tid3, NULL, receive_message, (void *) &args4);

   sleep(2);
   msg_test_args args5 = {.tag = 49, .level = 2, .buff = "Echo", .size = strlen("Echo")};
   pthread_create(&tid4, NULL, send_message, (void *) &args5);

   msg_test_args args6 = {.tag = 89, .level = 30, .buff = "Lost echo", .size = strlen("Lost echo")};
   pthread_create(&tid5, NULL, send_message, (void *) &args6);

   sleep(2);

   printf("\n\n/---------Current status-----------/\n");
   printf("TAG - Creator - Level - Waiting Threads\n");
   read_status();

   sleep(2);

   printf("\n\n/--------- Testing sys_tag_ctl-----------/\n");
   printf("Removal test started...\n");
   removing_test();

   printf("Awakening test started...\n");
   awakening_test();

   pthread_join(tid0, NULL);
   pthread_join(tid1, NULL);
   pthread_join(tid2, NULL);
   pthread_join(tid3, NULL);
   pthread_join(tid4, NULL);
   pthread_join(tid5, NULL);
}


/* Main fucntion for testing */
void main(int argc, char **argv) {

   fd = open("/dev/TBDE", O_RDONLY);

   /* ------ Tests ------ */
   /* tag_get */
   printf("/-------- Testing sys_tag_get --------/\n");
   printf("Tag service creation Test...\n");
   creation_test();
   printf("Tag service opening test...\n");
   opening_test();
   printf("\n\n");
   sleep(5);

   /* call the msg_test function to test further */
   msg_test();
   printf("/-------- Test Finished --------/\n");

   close(fd);

}
