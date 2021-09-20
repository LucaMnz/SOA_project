#ifndef RCU_LIST_H
#define RCU_LIST_H

/* Messages between threads are stored in a RCU
   list */
typedef struct _msg {
        struct _msg *next;
        char *msg;
        size_t size;
        int reading_threads;
} msg_t;

/* This structure allows an efficient RCU management of the messages.
   It is based on mechanism with epochs. In this way the buffer 
   reuse takes place only after a grace period. */
typedef struct _rcu_msg_list {
	int epoch;
	long standing[2];
        msg_t *head;
	spinlock_t write_lock;
} list_t;

/* msg utility */
void rcu_messages_list_init(list_t *list);

int rcu_messages_list_insert(list_t *list, msg_t msg);

int rcu_messages_list_remove(list_t *list, msg_t *msg);

#endif
