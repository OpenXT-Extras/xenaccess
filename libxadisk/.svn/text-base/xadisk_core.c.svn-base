/* Ties everything together */
#define _GNU_SOURCE

/* Large file support */
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fs.h>
#include <blktaplib.h>
#include <pthread.h>

#include "xadisk_global.h"
#include "xadisk_parse.h"
#include "xadisk_hash.h"
#include "xadisk.h"

#define APP_FIFO_NAME "appfifo"

/* Received from from the tapfifo: disk block + metadata */
#define BUFSIZE 5009

extern hash_entry ht[HASH_SIZE];
//extern pthread_mutex_t hash_mutex;

extern int xa_linux_ext2_dir(int fd, xa_linux_ext2_sb sb, uint32_t *inode_table, char *dirpath, uint32_t *blocks, uint32_t *nblocks, uint32_t *inode);
extern void xa_linux_ext2_init(xadisk_t *x);

typedef struct block_op_s {
	unsigned char op;
	uint64_t sector;
	int nb_sectors;
	unsigned char data[4096];
} block_op;

void parse_op(unsigned char *buf, block_op *b)
{
	b->op = buf[0];
	memcpy(&(b->sector), buf+1, 8);
	memcpy(&(b->nb_sectors), buf+9, 4);
	memcpy(&(b->data), buf+13, 4096);
}

xadisk_t *xadisk_init(int domid, char *image_path)
{
	xadisk_t *x;
	
	x = (xadisk_t *)malloc(sizeof(xadisk_t));
	x->domid = domid;
	asprintf(&(x->image_path), "%s", image_path);
	x->image_fd = open(x->image_path, O_RDONLY | O_LARGEFILE);
	
	xa_linux_ext2_init(x);
	asprintf(&(x->fifo_path),"/tmp/%s%d", APP_FIFO_NAME, domid);
	mkfifo(x->fifo_path,S_IRWXU|S_IRWXG|S_IRWXO);
	
	if((x->fifo_fd = open(x->fifo_path, O_RDWR | O_NONBLOCK)) == -1)
		perror("Error opening application FIFO:");
	
	init_hash();
	
	return x;
}

int xadisk_destroy(xadisk_t *x)
{
	//close(x->fifo_fd);
	close(x->image_fd);
	
	//if((unlink(x->fifo_path)) != 0){
	//	perror("Error removing FIFO:");
	//}		
	free(x->image_path);
	free(x->fifo_path);
	free(x->i_tables);
	free(x);
	
	destroy_hash();
	return 0;
}

xadisk_obj_t *xadisk_set_watch(xadisk_t *x, char *path)
{
	xadisk_obj_t *obj;
	block_node *node;
	unsigned char read_buf[4096];
	int i;
	
	obj = (xadisk_obj_t *)malloc(sizeof(xadisk_obj_t));
	
	if(xa_linux_ext2_dir(x->image_fd, x->sb, x->i_tables, path, obj->blocks, &(obj->nblocks), &(obj->inode)) != 0){
		printf("Directory not found!\n");
		return NULL;
	}
	
	asprintf(&(obj->path),"%s",path);
	
	for(i=0;i<obj->nblocks;i++){
		node = (block_node *)malloc(sizeof(block_node));
		node->block = obj->blocks[i];
		node->inode = obj->inode;
		node->type = 2;
		asprintf(&(node->path),"%s",path);
		node->dir = (dir_table *)malloc(sizeof(dir_table));
		
		lseek(x->image_fd, (off_t)(node->block) * (off_t)4096, SEEK_SET);
		read(x->image_fd, read_buf, 4096);
		
		parse_block_dir(read_buf, 4096, node->dir);
		sort_dir(node->dir);
		
		insert_hash(node);
	}
	return obj;
}


int xadisk_unset_watch(xadisk_t *x, xadisk_obj_t *obj)
{
	int i;
	
	//Removes blocks in obj->blocks[i] from hash table
	for(i=0;i<obj->nblocks;i++){
		remove_hash(obj->blocks[i]);
	}
	free(obj->path);
	free(obj);

    return 0;
}

/* Core processing is done here */

void xadisk_core(xadisk_t *x){
	
	unsigned char buf[BUFSIZE];
	dir_table *t;
	block_node *b;
	block_op op;
	dir_optable *diff;
	int i;
	int foo;
	uint32_t block;
	FILE *tapf;
	
	tapf = fdopen(x->fifo_fd, "w");
	
	while((foo=read(x->tapfifo_fd, buf, BUFSIZE)) != 0){
		parse_op(buf, &op);
		block = op.sector/8;
		b = check_hash(block);
		if(b != NULL){
			t = (dir_table *)malloc(sizeof(dir_table));
			diff = (dir_optable *)malloc(sizeof(dir_optable));
			
			parse_block_dir(op.data, 4096, t);
			sort_dir(t);
			compare_dirs(b->dir, t, diff);
			if(diff->size > 0){
				for(i=0;i<(diff->size);i++){
					switch(diff->table[i].op){
						case OP_MKDIR:
							fprintf(tapf,"MKDIR: ");
							break;
						case OP_RMDIR:
							fprintf(tapf,"RMDIR: ");
							break;
						case OP_MKFILE:
							fprintf(tapf,"MKFILE: ");
							break;
						case OP_RMFILE:
							fprintf(tapf,"RMFILE: ");
							break;
					}
					fprintf(tapf,"%s/%s\n", b->path, diff->table[i].path_elem);
				}
				
				free(b->dir);
				b->dir = t;
			}
		}
	}
	pthread_exit(NULL);
}

/* xadisk_activate - activates FIFO thread */
int xadisk_activate(xadisk_t *x, char *tapfifo_path)
{
	if((x->tapfifo_fd = open(tapfifo_path, O_RDONLY)) == -1){
		perror("Error opening TAP FIFO:");
		return 1;
	}
		
	if(pthread_create(&(x->thread), NULL, (void *)xadisk_core, x) != 0){
		perror("Error creating FIFO thread:");
		return -1;
	}
	return 0;
}

/* xadisk_deactivate - deactivates FIFO thread */
int xadisk_deactivate(xadisk_t *x)
{
	
	if(pthread_cancel(x->thread) != 0){
		perror("Error destroying FIFO thread:");
		return -1;
	}
	
	close(x->tapfifo_fd);
	return 0;
}
