#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/rwlock.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <linux/syscalls.h>
#include <linux/init.h>
#include <linux/uidgid.h>

#include "../structure/properties.h"
#include "../structure/rcu_list.h"
#include "../structure/service_list_handler.h"

extern long sys_goto_sleep(level_t *);
extern long sys_awake(level_t *);

/* service_list */
static volatile tag_t *service_list;

/* This array keeps the usage status of the services */
static volatile int service_list_status[TAG_SERVICES_NUM] = {[0 ... (TAG_SERVICES_NUM-1)] = 0};

/* The first service_list index free for tag service allocation. Useful for IPC_PRIVATE services */
static volatile int first_index_free = -1;

void service_list_dealloc(void) {
   vfree(service_list);
}

int service_list_alloc(void) {

   /* service_list allocation */
   service_list = (tag_t *) vmalloc(TAG_SERVICES_NUM*sizeof(tag_t));
   if (!service_list) {
      printk("%s: unable to allocate the service_list\n", MODNAME);
      return -1;
   }

   memset(service_list, 0, TAG_SERVICES_NUM*sizeof(tag_t));

   first_index_free = 0;

   return 0;
}

int create_service(int key, int permission) {
   tag_t *new_service;
   int i;

   /* The service_list is full */
   if (first_index_free == -1) {
      printk("%s: thread %d - the service_list is full. Allocation failed\n", MODNAME, current->pid);
      return -1;
   }

   new_service = (tag_t *) vmalloc(sizeof(tag_t));

   /* istantiates a private tag service */
   if (key == IPC_PRIVATE) {
      new_service->key = first_index_free;
      /* This is the key the thread can use to open the process-limited service */
      key = first_index_free + 1;
      new_service->priv = 1; 
   } else {
      if (service_list_status[key-1] == 1) {
         vfree(new_service);
         printk("%s: thread %d - the tag service with key %d already exists. Allocation failed\n", MODNAME, current->pid, key);
         return -1;
      }

      new_service->key = key-1;
      new_service->priv = 0;
   }

   /* The IPC_PRIVATE resource can be accessed only by thread belonging to the
      creator process. So the resource has a process visibility */
   new_service->owner = current->tgid;
   new_service->open = 0;

   /* The service can be accessed and used by any user or only by a specified user */
   if (permission == ANY) {
      new_service->perm = -1;
   } else if (permission == ONLY_OWNER) {
      new_service->perm = current->cred->uid.val;
   }

   /* The spinlocks inizialization */
   spin_lock_init(&(new_service->awake_all_lock));
   rwlock_init(&(new_service->send_lock));
   rwlock_init(&(new_service->receive_lock));

   /* All the tag service levels are initialized */
   level_t *level_obj;
   for (i=0; i<LEVELS; i++) {
      level_obj = &(new_service->levels[i]);

      /* Just the minimum needed is initialized for every level */
      spin_lock_init(&(new_service->level_activation_lock[i]));
      level_obj->active = 0;
   }

   /* Service allocation */
   service_list[new_service->key] = *new_service;
   service_list_status[new_service->key] = 1;

   /* Updating the first_index_free variable */
   for (i=first_index_free+1; i<TAG_SERVICES_NUM; i++) {
      if (service_list_status[i] == 0) {
         first_index_free = i;
         break;
      }
      if (i == TAG_SERVICES_NUM-1) {
         first_index_free = -1;
      }
   }
   printk("%s: thread %d - tag service with key %d correctly created\n", MODNAME, current->pid, key);
   return key;

}

int access_check(int key) {
   tag_t *tag_service = &(service_list[key-1]);

   /* IPC_PRIVATE used during the istantiation */
   if (tag_service->priv == 1 && tag_service->owner != current->tgid) {
      printk("%s: thread %d - Usage of the tag service with key %d denied\n", MODNAME, current->pid, key);
      return -1;
   }

   /* Service not accessible by all the users */
   if (tag_service->perm != -1 && tag_service->perm != current->cred->uid.val) {
      printk("%s: thread %d - the user %d cannot use the tag service with key %d\n", MODNAME, current->pid, current->cred->uid.val, key);
      return -1;
   }

   if (tag_service->open == 0) {
      printk("%s: thread %d - the tag service with key %d is closed\n", MODNAME, current->pid, key);
      return EOPEN;
   }

   return 0;
}

int open_service(int key, int permission) {
   tag_t *tag_service;

    /* error check */
   if (service_list_status[key-1] == 0) {
      printk("%s: thread %d - the key %d does not match any existing tag service. Opening failed\n", MODNAME, current->pid, key);
      return -1;
   }
   if (access_check(key) == -1) return -1;

   tag_service = &(service_list[key-1]);

   /* Tag service opening */
   tag_service->open = 1;

   asm volatile("mfence");

   printk("%s: thread %d - the service with tag %d is now open\n", MODNAME, current->pid, key);

   /* The return value is the tag */
   return key-1;
}

int send_message(int key, int level, char *buffer, size_t size) {
   tag_t *tag;
   level_t *level_obj;
   int err;

   /* error check */
   if (service_list_status[key-1] == 0) {
      printk("%s: thread %d - the specified tag does not exist. Operation failed\n", MODNAME, current->pid);
      return -1;
   }
   if ((err = access_check(key)) == -1) return -1;
   if (err == EOPEN) return -1;

   tag = &(service_list[key-1]);
   read_lock(&(tag->send_lock));
   level_obj = &(tag->levels[level]);

   /* check level initialization */
   spin_lock(&(tag->level_activation_lock[level]));
   if (level_obj->active == 0) {
      /* The level-specific message list is a RCU list */
      rcu_messages_list_init(&(level_obj->msg_list));

      level_obj->active = 1;
      level_obj->waiting_threads = 0;
      level_obj->head.task = NULL;
      level_obj->head.pid = -1;
      level_obj->head.awake = -1;
      level_obj->head.already_hit = -1;
      level_obj->head.next = NULL;

      /* Spinlocks initialization */
      spin_lock_init(&(level_obj->queue_lock));
      spin_lock_init(&(level_obj->msg_lock));

      asm volatile("mfence");
   }

   spin_unlock(&(tag->level_activation_lock[level]));

   msg_t message;
   message.size = size;
   message.reading_threads = level_obj->waiting_threads;
   message.msg = (char *) vmalloc(size);

   if (!message.msg) {
      printk("%s: thread %d - error in memory allocation for sending message\n", MODNAME, current->pid);
      read_unlock(&(tag->send_lock));
      return -1;
   }

   /* The message is copied into the service buffer */
   strncpy(message.msg, buffer, size);

   spin_lock(&(level_obj->msg_lock));

   if (level_obj->waiting_threads == 0) {
      printk("%s: thread %d - no waiting threads. The message will not be sent\n", MODNAME, current->pid);
   } else {
      /* The message sending awakes the waiting threads on that level.
         They will read the message through RCU mechanism */
      int res = rcu_messages_list_insert(&(level_obj->msg_list), message);

      if (res == -1) {
         spin_unlock(&(level_obj->msg_lock));
         read_unlock(&(tag->send_lock));

         return -1;
      }

      int resumed_pid;

      asm volatile("mfence");

      resumed_pid = -1;

      /* Until there is a thread to wake up */
      while (resumed_pid != 0) {
         /* The first waiting thread is woke up */
         resumed_pid = sys_awake(level_obj);

         if (resumed_pid == 0) break;

         printk("%s: thread %d woke up thread %d\n", MODNAME, current->pid, resumed_pid);
      }
   }

   spin_unlock(&(level_obj->msg_lock));

   read_unlock(&(tag->send_lock));

   return 0;
}

char *receive_message(int key, int level, char* buffer, size_t size) {
   tag_t *tag;
   level_t *level_obj;
   int err;

   /* errors check */
   if (service_list_status[key-1] == 0) {
      printk("%s: thread %d - the specified tag does not exist. Operation failed\n", MODNAME, current->pid);
      return NULL;
   }
   if ((err = access_check(key)) == -1) return NULL;
   if (err == EOPEN) return NULL;

   tag = &(service_list[key-1]);
   read_lock(&(tag->receive_lock));
   level_obj = &(tag->levels[level]);

   /* level object initialization check */
   spin_lock(&(tag->level_activation_lock[level]));
   if(level_obj->active == 0){
      /* The level-specific message list is a RCU list */
      rcu_messages_list_init(&(level_obj->msg_list));

      level_obj->active = 1;
      level_obj->waiting_threads = 0;
      level_obj->head.task = NULL;
      level_obj->head.pid = -1;
      level_obj->head.awake = -1;
      level_obj->head.already_hit = -1;
      level_obj->head.next = NULL;

      /* Spinlock for thread queueing and message storing initialization */
      spin_lock_init(&(level_obj->queue_lock));
      spin_lock_init(&(level_obj->msg_lock));

      asm volatile("mfence");
   }

   spin_unlock(&(tag->level_activation_lock[level]));

   /* increment waiting threads */
   level_obj->waiting_threads++;

   printk("%s: thread %d wants to sleep waiting for a message\n", MODNAME, current->pid);

   read_unlock(&(tag->receive_lock));

   /* The thread requests for a sleep, waiting for the message receiving */
   sys_goto_sleep(level_obj);

   /* This is the next instruction executed after the awakening */
   read_lock(&(tag->receive_lock));
   spin_lock(&(level_obj->msg_lock));

   msg_t *msg = level_obj->msg_list.head;

   /* The thread is resumed but there is no available message */
   if (msg == NULL) {
      spin_unlock(&(level_obj->msg_lock));
      asm volatile("mfence");
      printk("%s: thread %d - no available messages\n", MODNAME, current->pid);
      level_obj->waiting_threads--;
      read_unlock(&(tag->receive_lock));
      return NULL;
   }

   /* size fix */
   if (size > msg->size) {
      size = msg->size;
   }

   /* Actual message copy on thread buffer */
   strncpy(buffer, msg->msg, size);
   buffer[size] = '\0';
   printk("%s: thread %d - message read\n", MODNAME, current->pid);

   int reading_threads;
   reading_threads = msg->reading_threads-1;

   /* remove msg from rcu list after it has been read by all threads*/
   if (reading_threads == 0) {
      rcu_messages_list_remove(&(level_obj->msg_list), msg);
   }

   level_obj->waiting_threads--;
   spin_unlock(&(level_obj->msg_lock));
   read_unlock(&(tag->receive_lock));
   return buffer;
}

char *status_builder(void) {
   int i, j;
   int len = 0;
   tag_t *tag_obj;
   level_t *level_obj;

   char *service_status = (char *) vmalloc(sizeof(char) * 8192);

   if (!service_status) {
      printk("%s: error while allocating memory with vmalloc.\n", MODNAME);
      return NULL;
   }


   strcpy(service_status, "");

   for (i=0; i<TAG_SERVICES_NUM; i++) {
      if (service_list_status[i] == 1) {
         tag_obj = &(service_list[i]);

         printk("%s: key %d\n", MODNAME, tag_obj->key);

         if (tag_obj != NULL) {
            char tag[4];
            char creator[16];
            int key = i+1;

            printk("%s:var key: %d\n", MODNAME, key);

            /* Tag service creator */
            sprintf(creator, "%d", tag_obj->owner);
            /* Tag service identifier */
            sprintf(tag, "%d", key);

            for (j=0; j<LEVELS; j++) {
               level_obj = &(tag_obj->levels[j]);
               if (level_obj->active == 1) {
                  printk("%s:level: %d\n", MODNAME, j);

                  char level[4];
                  char waiting_threads[4];

                  /* Level considered */
                  sprintf(level, "%d", j);
                  /* Waiting threads on the considered tag service level */
                  sprintf(waiting_threads, "%d", level_obj->waiting_threads);

                  printk("%s: tag: %d - creator: %d - level: %d - waiting_threads: %d\n", MODNAME, key, tag_obj->owner, j, level_obj->waiting_threads);

                  strcat(service_status, tag);
                  len += sizeof(tag);
                  strcat(service_status, ", ");
                  len += sizeof(", ");
                  strcat(service_status, creator);
                  len += sizeof(creator);
                  strcat(service_status, ", ");
                  len += sizeof(", ");
                  strcat(service_status, level);
                  len += sizeof(level);
                  strcat(service_status, ", ");
                  len += sizeof(", ");
                  strcat(service_status, waiting_threads);
                  len += sizeof(waiting_threads);
                  strcat(service_status, "\n");
                  len += sizeof("\n");
               }
            }
         }
      }
   }

   service_status[len] = '\0';
   len = strlen(service_status);

   if (len == 0) {
      printk("%s: no active service\n", MODNAME);
      return "";
   }

   char *final_status = vmalloc(sizeof(char)*len);

   if (!final_status) {
      printk("%s: error while allocating memory with vmalloc\n", MODNAME);
      vfree(service_status);
      return NULL;
   }

   strncpy(final_status, service_status, len);
   final_status[len] = '\0';
   vfree(service_status);

   return final_status;
}

/* force all threads to awake */
int awake_all_threads(int key) {
   tag_t *tag;
   int resumed_pid, i;
   int err;

   /* error check */
   if (service_list_status[key-1] == 0) {
      printk("%s: thread %d - the specified tag does not exist. Operation failed\n", MODNAME, current->pid);
      return -1;
   }
   if ((err = access_check(key)) == -1) return -1;
   if (err == EOPEN) return -1;

   tag = &service_list[key-1];

   spin_lock(&(tag->awake_all_lock));

   // Check for waiting threads
   for (i=0; i<LEVELS; i++) {
      resumed_pid = -1;
      if (tag->levels[i].active == 1) {
         level_t *level_obj = &(tag->levels[i]);
         while (resumed_pid != 0) {
            resumed_pid = sys_awake(level_obj);

            if (resumed_pid == 0) break;

            printk("%s: thread %d woke up at level %d.\n", MODNAME, resumed_pid, i);
         }
      }
   }

   spin_unlock(&(tag->awake_all_lock));
   return 0;
}

int remove_tag(int key) {
   tag_t *tag;
   int i;

   /* error check */
   if (service_list_status[key-1] == 0) {
      printk("%s: thread %d - the specified tag does not exist. Operation failed\n", MODNAME, current->pid);
      return -1;
   }
   if (access_check(key) == -1) {
      return -1;
   }

   tag = &service_list[key-1];
   spin_lock(&(tag->awake_all_lock));
   write_lock(&(tag->send_lock));
   write_lock(&(tag->receive_lock));

   // Check for waiting threads
   for (i=0; i<LEVELS; i++) {
      if (tag->levels[i].active == 1) {
         if (tag->levels[i].waiting_threads != 0) {
            printk("%s: thread %d - the tag cannot be removed because there are threads waiting for messages at level %d.\n", MODNAME, current->pid, i);
            write_unlock(&(tag->send_lock));
            write_unlock(&(tag->receive_lock));
            spin_unlock(&(tag->awake_all_lock));
            return -1;
         }
      }
   }

   /* The tag service structure is deallocated */
   service_list_status[key-1] = 0;

   /* The first_index_free variable is updated */
   if (first_index_free == -1 || (key-1) < first_index_free) {
      first_index_free = key-1;
   }

   printk("%s: thread %d - the tag has been correctly removed\n", MODNAME, current->pid);
   write_unlock(&(tag->receive_lock));
   write_unlock(&(tag->send_lock));
   spin_unlock(&(tag->awake_all_lock));
   return 0;
}
