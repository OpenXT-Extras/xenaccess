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
#include <pthread.h>

#include "xadisk_global.h"
#include "xadisk_hash.h"
#include "xadisk_parse.h"

hash_entry ht[HASH_SIZE];
//pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;
		
int hash_block(uint32_t block)
{
	return (block % HASH_SIZE);
}

/* Initialize the hash table with special head nodes in each list */
void init_hash(void)
{
	int i;
	
	//pthread_mutex_lock(&hash_mutex);
	for(i=0;i<HASH_SIZE;i++){
		ht[i].active = 0;
		ht[i].list = NULL;
	}	
	//pthread_mutex_unlock(&hash_mutex);
}

void destroy_list(block_list *list)
{
	block_list aux;

	while((*list) != NULL){
		aux = (*list)->next;
		free(*list);
		*list = aux;
	}
}

void destroy_hash(void)
{
	int i;

	//pthread_mutex_lock(&hash_mutex);
	for(i=0;i<HASH_SIZE;i++){
		ht[i].active = 0;
		destroy_list(&(ht[i].list));
	}
	//pthread_mutex_unlock(&hash_mutex);
}

/* Inserts the block structure into the corresponding hash table linked list (sorted) */
void insert_list(block_list *list, block_node *b)
{
	block_list *p, q;

	p = list;
	q = *list;

	while((q != NULL) && (q->block < b->block)){
		p = &(q->next);
		q = q->next;
	}
	*p = b;
	b->next = q;
}

/* Inserts a block in the hash table */
void insert_hash(block_node *b)
{
	int i;

	i = hash_block(b->block);
	
	//pthread_mutex_lock(&hash_mutex);
	ht[i].active = ht[i].active | (unsigned char)0xFF; /* Uuuu....deep magic...I wonder what this does... */
	insert_list(&(ht[i].list), b);
	//pthread_mutex_unlock(&hash_mutex);
}

void remove_list(block_list *list, uint32_t block)
{
	block_list *p, q;

	p = list;
	q = *list;

	while((q != NULL) && (q->block < block)){
		p = &(q->next);
		q = q->next;
	}
	if((q != NULL) && (q->block == block)){
		*p = q->next;
		free(q);
	}
}

void remove_hash(uint32_t block)
{
	int i;

	i = hash_block(block);
	
	//pthread_mutex_lock(&hash_mutex);
	remove_list(&(ht[i].list), block);
	if(ht[i].list == NULL)
		ht[i].active = ht[i].active & (unsigned char)0x00; /* Uuuu....deep magic...I wonder what this does... */
	//pthread_mutex_unlock(&hash_mutex);
}

/* Checks for a certain block in the linked list, returning a pointer to it if it finds it. */
block_node *check_list(block_list list, uint32_t block)
{
	while((list != NULL) && (list->block < block)){
		list = list->next;
	}
	return ((list != NULL) && (list->block == block) ? list : NULL);
}

/* Searches for a block in the hash table */
block_node *check_hash(uint32_t block)
{
	block_node *b;
	
	//pthread_mutex_lock(&hash_mutex);
	if (ht[hash_block(block)].active == 0){  /* This will be the common case...let's optimize here with likely() */
		return NULL;
	}

	b = check_list(ht[hash_block(block)].list, block);
	//pthread_mutex_unlock(&hash_mutex);
	
	return b;
}
