#ifndef __XADISK_HASH_H
#define __XADISK_HASH_H

#include <stdint.h>

#include "xadisk_parse.h"

#define HASH_SIZE 32768

typedef struct block_node_s {
	uint32_t block;
	uint32_t inode;
	uint8_t type;
	char *path;
	dir_table *dir;
	unsigned char *raw_block;
	struct block_node_s *next;
} block_node, *block_list;

typedef struct hash_entry_s {
	unsigned char active;
	block_list list;
} hash_entry;

void init_hash(void);

void destroy_hash(void);

void insert_hash(block_node *b);

void remove_hash(uint32_t block);

block_node *check_hash(uint32_t block);

#endif
