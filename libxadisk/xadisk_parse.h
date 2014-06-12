#ifndef __XADISK_PARSE_H
#define __XADISK_PARSE_H

#include <stdint.h>

#define DIRMAX 256
#define OPMAX DIRMAX

/* List of possible directory operations handled so far */

#define OP_MKDIR 0
#define OP_RMDIR 1
#define OP_MKFILE 2
#define OP_RMFILE 3


typedef struct dir_entry_s {
	uint32_t inode;
	uint8_t type;
	char *path_elem;
} dir_entry;

typedef struct dir_table_s {
	short size;
	dir_entry table[DIRMAX];
} dir_table;
	
typedef struct dir_op_s {
	uint32_t inode;
	uint8_t op;
	char path_elem[MAX_PATH_LEN];
} dir_op;

typedef struct dir_optable_s {
	short size;
	dir_op table[OPMAX];
} dir_optable;

void parse_block_dir(unsigned char *block, short dir_size, dir_table *res);

void compare_dirs(dir_table *dir_old, dir_table *dir_new, dir_optable *res);

void sort_dir(dir_table *dir);

#endif
