/* block-logsync.c
*
* Good old synchronous driver, but now we are logging reads and writes to a FIFO, 
* hoping that someone is listening on the other end :P 
*
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "tapdisk.h"

#define FIFO_DIR "/dev/xen"
#define FIFO_NAME "tapfifo"
#define BUFSIZE 5009

struct tdsync_state {
	int fd;
	int poll_pipe[2]; /* dummy fd for polling on */
};
struct tdsync_state;
extern int tdsync_open (struct td_state *s, const char *name);
extern int tdsync_queue_read(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private);
extern int tdsync_queue_write(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private);
extern int tdsync_submit(struct td_state *s);
extern int *tdsync_get_fd(struct td_state *s);
extern int tdsync_close(struct td_state *s);
extern int tdsync_do_callbacks(struct td_state *s, int sid);

int fifo_fd;
char *fifo_path;

int tdlogsync_open(struct td_state *s, const char *name)
{
	asprintf(&fifo_path,"%s/%s%d", FIFO_DIR, FIFO_NAME, ((blkif_t *)s->blkif)->minor);
	if((mkfifo(fifo_path,S_IRWXU|S_IRWXG|S_IRWXO)) != 0){
		perror("Error creating FIFO:");
	}
	if((fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK))!=0)
		perror("Error opening FIFO:");
	
	return tdsync_open(s, name);
}

int tdlogsync_close(struct td_state *s)
{
	close(fifo_fd);
	if(unlink(fifo_path)!=0)
		perror("Error removing FIFO:");
	
	return tdsync_close(s);
}

int tdlogsync_queue_write(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private)
{
	unsigned char data[BUFSIZE], *p;

	/* Marshalling the data...there must be a better way to do this! */

	p = data;
	(p++)[0] = 1; /* Write operation */
	memcpy(p, &sector, 8); p += 8;
	memcpy(p, &nb_sectors, 4); p += 4;
	//memcpy(p, ((tapdev_info_t *)(s->ring_info))->mem, 4); p += 4;
	memcpy(p, buf, 4096);
	
	/* Send it through the FIFO */

	if(write(fifo_fd, data, BUFSIZE) == -1)
		perror("Error writing to FIFO:");

	return tdsync_queue_write(s, sector, nb_sectors, buf, cb, id, private);

}


int tdlogsync_queue_read(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private){

	return tdsync_queue_read(s, sector, nb_sectors, buf, cb, id, private);

}

struct tap_disk tapdisk_log_sync = {
	"tapdisk_log_sync",
	sizeof(struct tdsync_state),
	tdlogsync_open, 		/* Opening the FIFO */
	tdlogsync_queue_read, 		/* Logging block reads */
	tdlogsync_queue_write, 		/* Logging block writes */
	tdsync_submit,
	tdsync_get_fd,
	tdlogsync_close, 		/* Closing the FIFO */
	tdsync_do_callbacks,
};
