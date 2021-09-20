#ifndef DEV_H
#define DEV_H

/* device utility */

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
int register_dev(void);
void unregister_dev(void);

#endif
