gcc -fPIC -g -c -Wall xadisk_core.c
gcc -fPIC -g -c -Wall xadisk_hash.c
gcc -fPIC -g -c -Wall xadisk_parse.c
gcc -fPIC -g -c -Wall xadisk_linux_ext2.c

gcc -shared -Wl,-soname,libxadisk.so -o libxadisk.so xadisk_core.o xadisk_hash.o xadisk_parse.o xadisk_linux_ext2.o -lc

gcc -lpthread -L. libxadisk.so -o monitor monitor.c
