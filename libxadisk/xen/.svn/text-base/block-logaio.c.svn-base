#include <errno.h>
#include <libaio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "tapdisk.h"

#define FIFO_DIR "/dev/xen"
#define FIFO_NAME "tapfifo"
#define BUFSIZE 5009

#define REQUEST_ASYNC_FD 1

#define MAX_AIO_REQS (MAX_REQUESTS * MAX_SEGMENTS_PER_REQ)

struct pending_aio {
	td_callback_t cb;
	int id;
	void *private;
};

struct tdaio_state {
	int fd;
	
	/* libaio state */
	io_context_t       aio_ctx;
	struct iocb        iocb_list  [MAX_AIO_REQS];
	struct iocb       *iocb_free  [MAX_AIO_REQS];
	struct pending_aio pending_aio[MAX_AIO_REQS];
	int                iocb_free_count;
	struct iocb       *iocb_queue[MAX_AIO_REQS];
	int                iocb_queued;
	int                poll_fd; /* NB: we require aio_poll support */
	struct io_event    aio_events[MAX_AIO_REQS];
};

#define IOCB_IDX(_s, _io) ((_io) - (_s)->iocb_list)

//struct tdsync_state;
extern int tdaio_open (struct td_state *s, const char *name);
extern int tdaio_queue_read(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private);
extern int tdaio_queue_write(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private);
extern int tdaio_submit(struct td_state *s);
extern int *tdaio_get_fd(struct td_state *s);
extern int tdaio_close(struct td_state *s);
extern int tdaio_do_callbacks(struct td_state *s, int sid);

int fifo_fd;
char *fifo_path;

int tdlogaio_open(struct td_state *s, const char *name)
{
	asprintf(&fifo_path,"%s/%s%d", FIFO_DIR, FIFO_NAME, ((blkif_t *)s->blkif)->minor);
	if((mkfifo(fifo_path,S_IRWXU|S_IRWXG|S_IRWXO)) != 0){
		perror("Error creating FIFO:");
	}
	if((fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK))!=0)
		perror("Error opening FIFO:");
	
	return tdaio_open(s, name);
}

int tdlogaio_close(struct td_state *s)
{
	close(fifo_fd);
	if(unlink(fifo_path)!=0)
		perror("Error removing FIFO:");
	
	return tdaio_close(s);
}

int tdlogaio_queue_write(struct td_state *s, uint64_t sector,
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

	return tdaio_queue_write(s, sector, nb_sectors, buf, cb, id, private);

}


int tdlogaio_queue_read(struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *private){

	return tdaio_queue_read(s, sector, nb_sectors, buf, cb, id, private);

}

struct tap_disk tapdisk_log_aio = {
	"tapdisk_log_aio",
	sizeof(struct tdaio_state),
	tdlogaio_open, 		/* Opening the FIFO */
	tdlogaio_queue_read, 		/* Logging block reads */
	tdlogaio_queue_write, 		/* Logging block writes */
	tdaio_submit,
	tdaio_get_fd,
	tdlogaio_close, 		/* Closing the FIFO */
	tdaio_do_callbacks,
};
