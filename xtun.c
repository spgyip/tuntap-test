#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#define c2h(c) ((c)<=9?('0'+(c)):('a'+(c)-10))

#define TYPE_TUN 0
#define TYPE_TAP 1

const char* type_string(int type) {
    if(type==TYPE_TUN) {
        return "tun";
    } else if(type==TYPE_TAP) {
        return "tap";
    }
    return "unknown";
}

int tun_alloc(int type, const char *name) {
    struct ifreq ifr;
    int fd, err;

    fd=open("/dev/net/tun", O_RDWR);
    if(fd<0) {
        printf("Open dev/net/tun error\n");
        return -1;
    }

    memset(&ifr, 0x00, sizeof(ifr));

    if(type==TYPE_TUN)
        ifr.ifr_flags = IFF_TUN;
    else 
        ifr.ifr_flags = IFF_TAP;
    ifr.ifr_flags |= IFF_NO_PI;

    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    err = ioctl(fd, TUNSETIFF, &ifr);
    if(err<0) {
        printf("ioctl TUNSETIFF error\n");
        close(fd);
        return -1;
    }
    return fd;
}

void dumphex(char *buf, int size) {
    printf("dumhex:: size=%d\n", size);
    char *output = malloc(size*2+1);
    memset(output, 0x00, size*2+1);
    for(int i=0, j=0; i<size; i++, j+=2) {
        output[j] = c2h(0x0f&(buf[i]>>4));
        output[j+1] = c2h(0x0f&buf[i]);
    }
    printf("%s\n", output);
    free(output);
}

/* UDP remote fd
 * UDP remote address
 */ 
int rfd = 0;
struct sockaddr_in raddr;

/* TUN device fd
 */
int tfd = 0;

void* routine_forward(void *arg) {
    printf("routine_forward running...\n");
    char buf[1600];
    ssize_t bytes;
    for(;;) {
        bytes = read(tfd, buf, sizeof(buf));
        if(bytes<0) {
            printf("[routine_forward] read error from tun: %s\n", strerror(errno));
            continue;
        }
        printf("[routine_forward] Read %lu bytes from tun\n", bytes);
        dumphex(buf, bytes);
        sendto(rfd, buf, bytes, 0, (struct sockaddr *)&raddr, sizeof(raddr));
    }
}

void* routine_backward(void *arg) {
    printf("routine_backward running...\n");

    char buf[1600];
    ssize_t bytes;

    struct sockaddr_in addr;
    socklen_t len;
    for(;;) {
        bytes = recvfrom(rfd, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &len);
        if(bytes<0) {
            printf("[routine_backward] Recvfrom error\n");
            continue;
        }
        printf("[routine_backward] Read %lu bytes from UDP\n", bytes);
        dumphex(buf, bytes);
        write(tfd, buf, bytes);
    }
}

int main(int argc, char* argv[]) {
    if(argc<3) {
        printf("Usage: %s [if name] [server ip]\n", argv[0]);
        return 1;
    }

    const char *tun_name = argv[1];
    const char *server_ip = argv[2];
    
    // 1. Create UDP tunnel
    // 1.1 socket
    rfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(rfd<0) {
        printf("Create socket error\n");
        return 2;
    }

    // 1.2 bind address
    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(5001);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(rfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr))<0) {
        printf("Bind socket error\n");
        return 2;
    }
    
    // 1.3 remote address
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(5001);
    inet_aton(server_ip, &raddr.sin_addr);

    printf("Create udp socket success, fd %d\n", rfd);

    // 1.4 Create tun
    tfd = tun_alloc(TYPE_TUN, tun_name);
    if(tfd<0) {
        printf("tun_alloc fail\n");
        return 2;
    }
    printf("tun_alloc success, fd %d\n", tfd);
    
    pthread_t tid1, tid2;
    if(pthread_create(&tid1, NULL, routine_forward, 0)<0) {
        printf("Thread create routine_forward error\n");
        return 1;
    }
    if(pthread_create(&tid2, NULL, routine_backward, 0)<0) {
        printf("Thread create routine_backward error\n");
        return 1;
    }

    pthread_join(tid1, 0);
    pthread_join(tid2, 0);
    printf("Main thread exit\n");

    return 0;
}
