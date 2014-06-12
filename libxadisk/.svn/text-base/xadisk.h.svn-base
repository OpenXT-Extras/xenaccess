#ifndef __LIBXADISK_H
#define __LIBXADISK_H

#include <pthread.h>
#include <stdint.h>

typedef struct xa_linux_ext2_sb_s {
	uint32_t blocks_total;
	uint32_t block_size;
	uint32_t blocks_per_group;
	uint32_t inodes_per_group;
	uint16_t inode_size;
} xa_linux_ext2_sb;

typedef struct {
	int domid;
	char *image_path;
	char *fifo_path;
	pthread_t thread;
	int fifo_fd;
	int tapfifo_fd;
	int image_fd;
	xa_linux_ext2_sb sb;
	uint32_t *i_tables;
} xadisk_t;

typedef struct {
	char *path;
	uint32_t inode;
	uint32_t blocks[15];
	uint32_t nblocks;
} xadisk_obj_t;

xadisk_t *xadisk_init(int domid, char *image_path);

int xadisk_destroy(xadisk_t *x);

xadisk_obj_t *xadisk_set_watch(xadisk_t *x, char *obj);

int xadisk_unset_watch(xadisk_t *x, xadisk_obj_t *obj);

int xadisk_activate(xadisk_t *x, char *tapfifo_path);

int xadisk_deactivate(xadisk_t *x);

#endif
