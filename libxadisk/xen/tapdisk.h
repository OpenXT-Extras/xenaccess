/* tapdisk.h
 *
 * Generic disk interface for blktap-based image adapters.
 *
 * (c) 2006 Andrew Warfield and Julian Chesterfield
 * 
 * Some notes on the tap_disk interface:
 * 
 * tap_disk aims to provide a generic interface to easily implement new 
 * types of image accessors.  The structure-of-function-calls is similar
 * to disk interfaces used in qemu/denali/etc, with the significant 
 * difference being the expectation of asynchronous rather than synchronous 
 * I/O.  The asynchronous interface is intended to allow lots of requests to
 * be pipelined through a disk, without the disk requiring any of its own
 * threads of control.  As such, a batch of requests is delivered to the disk
 * using:
 * 
 *    td_queue_[read,write]()
 * 
 * and passing in a completion callback, which the disk is responsible for 
 * tracking.  The end of a back is marked with a call to:
 * 
 *    td_submit()
 * 
 * The disk implementation must provide a file handle, which is used to 
 * indicate that it needs to do work.  tapdisk will add this file handle 
 * (returned from td_get_fd()) to it's poll set, and will call into the disk
 * using td_do_callbacks() whenever there is data pending.
 * 
 * Two disk implementations demonstrate how this interface may be used to 
 * implement disks with both asynchronous and synchronous calls.  block-aio.c
 * maps this interface down onto the linux libaio calls, while block-sync uses 
 * normal posix read/write.
 * 
 * A few things to realize about the sync case, which doesn't need to defer 
 * io completions:
 * 
 *   - td_queue_[read,write]() call read/write directly, and then call the 
 *     callback immediately.  The MUST then return a value greater than 0
 *     in order to tell tapdisk that requests have finished early, and to 
 *     force responses to be kicked to the clents.
 * 
 *   - The fd used for poll is an otherwise unused pipe, which allows poll to 
 *     be safely called without ever returning anything.
 * 
 */

#ifndef TAPDISK_H_
#define TAPDISK_H_

#include <stdint.h>
#include <syslog.h>
#include "blktaplib.h"

/*If enabled, log all debug messages to syslog*/
#if 1
#define DPRINTF(_f, _a...) syslog( LOG_DEBUG, _f , ## _a )
#else
#define DPRINTF(_f, _a...) ((void)0)
#endif

/* Things disks need to know about, these should probably be in a higher-level
 * header. */
#define MAX_SEGMENTS_PER_REQ    11
#define SECTOR_SHIFT             9
#define DEFAULT_SECTOR_SIZE    512

/* This structure represents the state of an active virtual disk.           */
struct td_state {
	void *private;
	void *drv;
	void *blkif;
	void *image;
	void *ring_info;
	void *fd_entry;
	char backing_file[1024]; /*Used by differencing disks, e.g. qcow*/
	unsigned long      sector_size;
	unsigned long long size;
	unsigned int       info;
};

/* Prototype of the callback to activate as requests complete.              */
typedef int (*td_callback_t)(struct td_state *s, int res, int id, void *prv);

/* Structure describing the interface to a virtual disk implementation.     */
/* See note at the top of this file describing this interface.              */
struct tap_disk {
	const char *disk_type;
	int private_data_size;
	int (*td_open)        (struct td_state *s, const char *name);
	int (*td_queue_read)  (struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *prv);
	int (*td_queue_write) (struct td_state *s, uint64_t sector,
			       int nb_sectors, char *buf, td_callback_t cb,
			       int id, void *prv);
	int (*td_submit)      (struct td_state *s);
	int *(*td_get_fd)      (struct td_state *s);
	int (*td_close)       (struct td_state *s);
	int (*td_do_callbacks)(struct td_state *s, int sid);
};

typedef struct disk_info {
	int  idnum;
	char name[50];       /* e.g. "RAMDISK" */
	char handle[10];     /* xend handle, e.g. 'ram' */
	int  single_handler; /* is there a single controller for all */
	                     /* instances of disk type? */
#ifdef TAPDISK
	struct tap_disk *drv;	
#endif
} disk_info_t;

void debug_fe_ring(struct td_state *s);

extern struct tap_disk tapdisk_aio;
extern struct tap_disk tapdisk_sync;
extern struct tap_disk tapdisk_vmdk;
extern struct tap_disk tapdisk_ram;
extern struct tap_disk tapdisk_qcow;
extern struct tap_disk tapdisk_log_sync;
extern struct tap_disk tapdisk_log_aio;

#define MAX_DISK_TYPES  20
#define MAX_IOFD        2

#define DISK_TYPE_AIO   0
#define DISK_TYPE_SYNC  1
#define DISK_TYPE_VMDK  2
#define DISK_TYPE_RAM   3
#define DISK_TYPE_QCOW  4
#define DISK_TYPE_LOG_SYNC 5
#define DISK_TYPE_LOG_AIO 6


/*Define Individual Disk Parameters here */
static disk_info_t aio_disk = {
	DISK_TYPE_AIO,
	"raw image (aio)",
	"aio",
	0,
#ifdef TAPDISK
	&tapdisk_aio,
#endif
};

static disk_info_t log_sync_disk = {
    DISK_TYPE_LOG_SYNC,
    "raw image (log-sync)",
    "log-sync",
    0,
#ifdef TAPDISK
    &tapdisk_log_sync,
#endif
};

static disk_info_t log_aio_disk = {
    DISK_TYPE_LOG_AIO,
    "raw image (log-aio)",
    "log-aio",
    0,
#ifdef TAPDISK
    &tapdisk_log_aio,
#endif
};

static disk_info_t sync_disk = {
	DISK_TYPE_SYNC,
	"raw image (sync)",
	"sync",
	0,
#ifdef TAPDISK
	&tapdisk_sync,
#endif
};

static disk_info_t vmdk_disk = {
	DISK_TYPE_VMDK,
	"vmware image (vmdk)",
	"vmdk",
	1,
#ifdef TAPDISK
	&tapdisk_vmdk,
#endif
};

static disk_info_t ram_disk = {
	DISK_TYPE_RAM,
	"ramdisk image (ram)",
	"ram",
	1,
#ifdef TAPDISK
	&tapdisk_ram,
#endif
};

static disk_info_t qcow_disk = {
	DISK_TYPE_QCOW,
	"qcow disk (qcow)",
	"qcow",
	0,
#ifdef TAPDISK
	&tapdisk_qcow,
#endif
};

/*Main disk info array */
static disk_info_t *dtypes[] = {
	&aio_disk,
	&sync_disk,
	&vmdk_disk,
	&ram_disk,
	&qcow_disk,
    &log_sync_disk,
    &log_aio_disk
};

typedef struct driver_list_entry {
	struct blkif *blkif;
	struct driver_list_entry **pprev, *next;
} driver_list_entry_t;

typedef struct fd_list_entry {
	int cookie;
	int  tap_fd;
	int  io_fd[MAX_IOFD];
	struct td_state *s;
	struct fd_list_entry **pprev, *next;
} fd_list_entry_t;

int qcow_create(const char *filename, uint64_t total_size,
		const char *backing_file, int flags);

#endif /*TAPDISK_H_*/
