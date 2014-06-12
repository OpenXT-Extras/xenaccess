
/* Enables large file support in Linux (> 2GB) */

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include "xadisk_global.h"
#include "xadisk.h"

#define LINUX_EXT2_ROOT_INODE 2
#define LINUX_EXT2_SUPERBLOCK_OFFSET 1024
#define LINUX_EXT2_NBLOCKS 15
#define LINUX_EXT2_GROUPDESC_SIZE 32
/*
typedef struct xa_linux_ext2_sb_s {
	uint32_t blocks_total;
	uint32_t block_size;
	uint32_t blocks_per_group;
	uint32_t inodes_per_group;
	uint16_t inode_size;
} xa_linux_ext2_sb;
*/
unsigned char *curdir_block, *curdir_inode;

void xa_linux_ext2_read_sb(int fd, xa_linux_ext2_sb *sb)
{
	unsigned char *p, *q;

	q = (unsigned char *)mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
	p = q;
	p += (LINUX_EXT2_SUPERBLOCK_OFFSET+4);

	sb->blocks_total = (*((uint32_t *)p)); p += 24;
	sb->block_size = 1024 * (1 << (*((uint32_t *)p))); p += 8;
	sb->blocks_per_group = (*((uint32_t *)p)); p += 4;
	sb->inodes_per_group = (*((uint32_t *)p)); p += 48;
	sb->inode_size = (*((uint16_t *)p));

	munmap(q, 4096);
}

void xa_linux_ext2_read_groupdesc(int fd, xa_linux_ext2_sb sb, uint32_t **gd)
{
	unsigned char *p;
	int ngroups, i;
	
	ngroups = sb.blocks_total / sb.blocks_per_group + 1; //temporary - should be ceiling, really
	
	p = (unsigned char *)mmap(0, (ngroups * LINUX_EXT2_GROUPDESC_SIZE), PROT_READ, MAP_PRIVATE, fd, 4096);
	p += 8; /* First inode field position */
	*gd = (uint32_t *)malloc(ngroups * sizeof(uint32_t));
	
	for(i=0;i < ngroups;i++){
		(*gd)[i] = (*((uint32_t *)p));
		p += LINUX_EXT2_GROUPDESC_SIZE;
	}
}

uint8_t xa_linux_ext2_dir_gettype(void *dir_entry)
{
	return (*((uint8_t *)(dir_entry+7)));
}

uint32_t xa_linux_ext2_dir_getinode(void *dir_entry)
{
	return (*((uint32_t *)dir_entry));
}

uint16_t xa_linux_ext2_dir_getreclen(void *dir_entry)
{
	return (*((uint16_t *)(dir_entry+4)));
}

uint8_t xa_linux_ext2_dir_getnamelen(void *dir_entry)
{
	return (*((uint8_t *)(dir_entry+6)));
}

char *xa_linux_ext2_dir_getname(void *dir_entry)
{
	return (char *)(dir_entry+8);
}

/* Returns file offset (64 bits) for a block group in which is located the inode we are looking for */
off_t xa_linux_ext2_dir_getgroup(xa_linux_ext2_sb sb, uint32_t inode)
{
	int group;

	group = inode / sb.inodes_per_group;
	return ((off_t)group * (off_t)sb.blocks_per_group * (off_t)sb.block_size);
}

/* Returns pointer do a directory entry inside a data block */
unsigned char *xa_linux_ext2_finddir(unsigned char *dir_block, char *name, short dir_size, short name_len)
{
	unsigned char *pos, *limit;

	pos = dir_block;
	limit = dir_block + dir_size;
	
	while(pos < limit){
		if((name_len == xa_linux_ext2_dir_getnamelen(pos)) && 
				  (!strncmp(xa_linux_ext2_dir_getname(pos), name, name_len)))
			return pos;
		pos += xa_linux_ext2_dir_getreclen(pos);
	}
	return NULL;
}

/* Returns pointer to inode structure on disk given pointer to group */
unsigned char *xa_linux_ext2_findinode(unsigned char *group, xa_linux_ext2_sb sb, uint32_t first_inode, uint32_t inode) 
{
	group += (first_inode % sb.blocks_per_group) * sb.block_size + (inode % sb.inodes_per_group - 1) * sb.inode_size;
	
	return group;
}

/* Given pointer to inode, return data block numbers in a vector */
void xa_linux_ext2_datablocks(unsigned char *inode, uint32_t blocks[LINUX_EXT2_NBLOCKS], uint32_t *nblocks)
{
	unsigned char *p;
	int i;	

	*nblocks = (*((uint32_t *)(inode+28)))/8;
	p = inode + 40;

	for(i=0;i<*nblocks;i++){
		blocks[i] = (*((uint32_t *)p));
		p += sizeof(uint32_t);
	}
	*nblocks = i;
}

/* This is the highest level function - gets the directory name and returns pointers to its blocks */
int xa_linux_ext2_dir(int fd, xa_linux_ext2_sb sb, uint32_t *inode_table, char *dirpath, uint32_t *blocks, uint32_t *nblocks, uint32_t *inode)
{
	uint32_t curinode;
	off_t group_off;
	unsigned char *group_ptr, *inode_ptr, *block_ptr, *dentry_ptr;
	int i, path_elem_len, group;
	char path_elem[MAX_PATH_LEN];
	
	curinode = LINUX_EXT2_ROOT_INODE;
	
	while(1){		
		/* Gets the group number */
		group = curinode / sb.inodes_per_group;
		
		/* Gets the offset for the group the inode is in */
		group_off = xa_linux_ext2_dir_getgroup(sb, curinode);
		
		/* Maps the group into memory */
		group_ptr = (unsigned char *)mmap(0, (sb.blocks_per_group * sb.block_size), PROT_READ, MAP_PRIVATE, fd,
						group_off);
		
		/* Gets pointer to inode struct inside the group */
		inode_ptr = xa_linux_ext2_findinode(group_ptr, sb, inode_table[group], curinode);

		/* Gets data block numbers from the inode struct */
		xa_linux_ext2_datablocks(inode_ptr, blocks, nblocks);
				
		while(*dirpath == '/') 
			dirpath++;
		if(*dirpath == '\0')
			break;
		path_elem_len = 0;
		
		for(i=0;(i<MAX_PATH_LEN) && (*dirpath != '/') && (*dirpath != '\0');i++){
			path_elem[i] = *dirpath;
			dirpath++;
			path_elem_len++;
		}
		
		/* Seeks pointer to the directory entry inside each data block number */
		for(i=0;i<*nblocks;i++){
			block_ptr = (unsigned char *)mmap(0, sb.block_size, PROT_READ, MAP_PRIVATE, fd,
			(off_t)blocks[i] * (off_t)sb.block_size);
			if((dentry_ptr = xa_linux_ext2_finddir(block_ptr, path_elem, sb.block_size, path_elem_len)) != NULL)
				break;
		}
		if(i == *nblocks) /* Invalid path */
			return -1;
			
		if((dentry_ptr != NULL) && (xa_linux_ext2_dir_gettype(dentry_ptr) == 2)){
			curinode = xa_linux_ext2_dir_getinode(dentry_ptr);
		}
		
		munmap(group_ptr, sb.blocks_per_group * sb.block_size);
		munmap(block_ptr, sb.block_size);
	}
	*inode = curinode;
	return 0;
}

void xa_linux_ext2_listdirs(unsigned char *dir_block, short dir_size)
{
	unsigned char *pos, *limit;
	char *name;
	int iname;

	pos = dir_block;
	limit = dir_block + dir_size;
	
	while(pos < limit){
		iname = 0;

		name = xa_linux_ext2_dir_getname(pos);
		printf("Name: ");
		while(iname < xa_linux_ext2_dir_getnamelen(pos)){
			putchar(*name);
			name++; iname++;
		}
		printf("\n");

		printf("Name size: %d\nEntry inode: %d\nEntry type: %d\nEntry size: %d\n\n",
		       xa_linux_ext2_dir_getnamelen(pos), xa_linux_ext2_dir_getinode(pos),
		       xa_linux_ext2_dir_gettype(pos), xa_linux_ext2_dir_getreclen(pos));

		pos += xa_linux_ext2_dir_getreclen(pos);
	}
}

void xa_linux_ext2_init(xadisk_t *x)
{
	xa_linux_ext2_read_sb(x->image_fd, &(x->sb));
	xa_linux_ext2_read_groupdesc(x->image_fd, x->sb, &(x->i_tables));

}


