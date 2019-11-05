#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd, err;

    fd=open("/dev/net/tun", O_RDWR);
    if(fd<0) {
        printf("Open dev/net/tun error\n");
        return -1;
    }

    memset(&ifr, 0x00, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    err = ioctl(fd, TUNSETIFF, &ifr);
    if(err<0) {
        printf("ioctl TUNSETIFF error\n");
        close(fd);
        return -1;
    }
    strcpy(dev, ifr.ifr_name);
    return fd;
}

int main(int argc, char* argv[]) {
    if(argc<2) {
        printf("Usage: %s name\n", argv[0]);
        return 1;
    }

    char name[256] = {0};
    strncpy(name, argv[1], sizeof(name)-1);

    int fd = tun_alloc(name);
    if(fd<0) {
        printf("tun_alloc fail\n");
        return 2;
    }
    printf("tun_alloc success, fd %d\n", fd);
    return 0;
}
