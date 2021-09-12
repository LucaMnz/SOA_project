#ifndef SERVICE_LIST_HANDLER_H
#define SERVICE_LIST_HANDLER_H

#include "properties.h"
#include "rcu_list.h"


/* An element of the wait queue list. It allows to handle the sleeping 
   thread during the service usage */
typedef struct _wq_elem {
   struct task_struct *task;
   int pid;
   int awake;
   int already_hit;
   struct _wq_elem *next;
} wq_t;

/* Every thread can exchange messages in the context of a level */
typedef struct _level {
   int active; /* inizialization */
   int waiting_threads; 
   wq_t head; /* head level queueing */
   spinlock_t queue_lock; /* threads queueing */
   spinlock_t msg_lock; /* messages access */
   list_t msg_list; /* List of messages sent on this level */
} level_t;

/* Service list */
typedef struct _tag_service {
   int key; 
   int perm;  /* permission */
   pid_t owner; 
   int priv; /* IPC_PRIVATE */
   int open; /* Can be 0 or 1, respectly: close or open */
   level_t levels[LEVELS];  /* Each service has it's own levels list */
   spinlock_t awake_all_lock; /* threads awakening through tag_ctl syscall */
   rwlock_t send_lock; /* lock for message sendings */
   rwlock_t receive_lock; /* lock for message receivings */
   spinlock_t level_activation_lock[LEVELS]; /* lock for level initialization */
} tag_t;

/*  Service list utility */

char *status_builder(void);

int create_service(int key, int permission);

int open_service(int key, int permission);

int send_message(int key, int level, char *buffer, size_t size);

char *receive_message(int key, int level, char* buffer, size_t size);

int awake_all_threads(int key);

int remove_tag(int key);

int service_list_alloc(void);

void service_list_dealloc(void);

#endif
