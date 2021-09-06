#ifndef PROPERTIES_H
#define PROPERTIES_H

#define MODNAME "TBDE"

/* Here are defined all the properties which consists in constants
   to make easier to modify those important parameters*/

/* Available levels for each tag service */
#define LEVELS 32

/* Commands allowed in tag_get syscall */
#define CREATE 0
#define OPEN 1

/* Command allowed in tag_ctl syscall */
#define AWAKE_ALL 0
#define REMOVE 1

/* Number of entries in syscall table */
#define NR_ENTRIES 256

/* Permission level for TAG service operations */
#define ONLY_OWNER 0
#define ANY 1

/* Number of entries in the service list */
#define TAG_SERVICES_NUM 256

/* Maximum size of a message sent/received by a thread */
#define MAX_MSG_SIZE 4096

/* vtpmo */
#define NO_MAP -1

/* Number of syscalls needed in the subsystem */
#define NR_NEW_SYSCALLS 4

#define EOPEN -2

#define DEVNAME "TBDE"

#define MAJOR_NM 237

#endif
