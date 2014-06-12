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
#include "xadisk_parse.h"

extern uint8_t xa_linux_ext2_dir_gettype(void *dir_entry);
extern uint32_t xa_linux_ext2_dir_getinode(void *dir_entry);
extern uint16_t xa_linux_ext2_dir_getreclen(void *dir_entry);
extern uint8_t xa_linux_ext2_dir_getnamelen(void *dir_entry);

void parse_name(void *dir_entry, char *res){
	strncpy(res, (char *)(dir_entry+8), *((uint8_t *)(dir_entry+6)));
}

/* Parses the in-memory directory structure given the raw block */
void parse_block_dir(unsigned char *block, short dir_size, dir_table *res)
{
	unsigned char *limit;
	int i;
	short len;

	limit = block + dir_size;
	
	for(i=0;block < limit;i++,(block += xa_linux_ext2_dir_getreclen(block))){
		res->table[i].inode = xa_linux_ext2_dir_getinode(block);
		res->table[i].type = xa_linux_ext2_dir_gettype(block);
		len = xa_linux_ext2_dir_getnamelen(block);
		res->table[i].path_elem = malloc(len + 1); //+1 needed for extra '\0' terminator
		parse_name(block, res->table[i].path_elem);
		res->table[i].path_elem[len] = '\0';
	}
	res->size = i;
}

/* Sorts the directory - necessary for comparison*/
/* Use an algorithm that works efficiently when the vector is already, or almost completely sorted and/or when it is short */
/* Insertion sort */

void sort_dir(dir_table *dir)
{
	int i, j, n;
	dir_entry aux;
	
	n = dir->size;
	for(i=1;i<n;i++){
		aux = dir->table[i];
		for(j = i-1;(j >= 0) && (strcmp(aux.path_elem, dir->table[j].path_elem) < 0);j--)
			dir->table[j+1] = dir->table[j];
		dir->table[j+1] = aux;
	}
}

/* 
 * Compares both dirs to determine the symetric difference and returns it in a dir_diff[]
 * Executes a classical sorted comparison algorithm by sequentially processing both vectors - O(N)
*/
void compare_dirs(dir_table *dir_old, dir_table *dir_new, dir_optable *res)
{
	int i, j, k;
	
	i = j = k = 0;
	while((i < dir_old->size) && (j < dir_new->size)){
		if(strcmp(dir_old->table[i].path_elem, dir_new->table[j].path_elem) < 0){ /* Removed objects */
			if(dir_old->table[i].type == 1)
				res->table[k].op = OP_RMFILE;
			else if(dir_old->table[i].type == 2)
				res->table[k].op = OP_RMDIR;
			strcpy(res->table[k].path_elem, dir_old->table[i].path_elem); //this could be a problem!
			i++; k++;
		}
		else if(strcmp(dir_new->table[j].path_elem, dir_old->table[i].path_elem) < 0){ /* Created objects */
			if(dir_new->table[j].type == 1)
				res->table[k].op = OP_MKFILE;
			else if(dir_new->table[j].type == 2)
				res->table[k].op = OP_MKDIR;
			strcpy(res->table[k].path_elem, dir_new->table[j].path_elem); //this could be a problem!
			j++; k++;
		}
		else {
			i++; j++;
		}
	}
	while(i < dir_old->size){
		if(dir_old->table[i].type == 1)
			res->table[k].op = OP_RMFILE;
		else if(dir_old->table[i].type == 2)
			res->table[k].op = OP_RMDIR;
		strcpy(res->table[k].path_elem, dir_old->table[i].path_elem); //this could be a problem!
		i++; k++;
	}
	
	while(j < dir_new->size){
		if(dir_new->table[j].type == 1)
			res->table[k].op = OP_MKFILE;
		else if(dir_new->table[j].type == 2)
			res->table[k].op = OP_MKDIR;
		strcpy(res->table[k].path_elem, dir_new->table[j].path_elem); //this could be a problem!
		j++; k++;
	}
	res->size = k;
}
