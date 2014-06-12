#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "xadisk.h"


int main(int argc, char **argv){
        xadisk_t *x;
        xadisk_obj_t *obj;
        int i;
        char *dir = "/tmp/bm\0\0\0\0";
        char c;

//        x = xadisk_init(1, "/root/Xen/Images/gentoo.2006-1.img\0");
        x = xadisk_init(1, "/opt/xen_images/fedora-pv/fedora.img\0");
        obj = xadisk_set_watch(x, argv[1]);

        xadisk_activate(x, "/dev/xen/tapfifo0\0");

        dup2(1, x->fifo_fd);

        pthread_join(x->thread, NULL);
        xadisk_unset_watch(x, obj);
        xadisk_destroy(x);

        return 0;
}
