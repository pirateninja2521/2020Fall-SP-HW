#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/stat.h>
#include <sys/types.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id; //customer id
    int adultMask;
    int childrenMask;
} Order;

int handle_read(request* reqP) {
    char buf[512];
    read(reqP->conn_fd, buf, sizeof(buf));
    memcpy(reqP->buf, buf, strlen(buf));
    return 0;
}

int handle_order(request* reqP, Order *preOrder){
    char buf[512];
    const char delimeter[] = " ";
    read(reqP->conn_fd, buf, sizeof(buf));
    char *token = strtok(buf, delimeter);
    if (strcmp("adult", token)==0){
        token = strtok(NULL, delimeter);
        if (atoi(token)<=0 || atoi(token)>preOrder[reqP->id].adultMask){
            return -1;
        }
        else{
            preOrder[reqP->id].adultMask -= atoi(token);
            printf("order success\n");
            sprintf(buf, "Pre-order for %d successed, %d adult mask(s) ordered.\n", preOrder[reqP->id].id, atoi(token));
            write(reqP->conn_fd, buf, strlen(buf)); 
        }
    }
    else if (strcmp("children", token)==0){
        token = strtok(NULL, delimeter);
        if (atoi(token)<=0 || atoi(token)>preOrder[reqP->id].childrenMask){
            return -1;
        }
        else{
            preOrder[reqP->id].childrenMask -= atoi(token);
            printf("order success\n");
            sprintf(buf, "Pre-order for %d successed, %d children mask(s) ordered.\n", preOrder[reqP->id].id, atoi(token));
            write(reqP->conn_fd, buf, strlen(buf)); 
        }
    }
    else{
        return -1;
    }
    return 0;
}

int find_id(int in, Order *preOrder){
    for (int i=0; i<20; i++){
        if (preOrder[i].id==in)
            return i;
    }
    return -1;
}

int check_lock(int fd_pre, int ID){
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = sizeof(Order)*ID;
    lock.l_len = sizeof(Order);
    fcntl(fd_pre, F_GETLK, &lock);
    if (lock.l_type == F_UNLCK){
        return 0;
    }
    else return -1;
}

int set_lock(int fd_pre, int ID, int type){
    struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = sizeof(Order)*ID;
    lock.l_len = sizeof(Order);
    return (fcntl(fd_pre, F_SETLK, &lock));
}
void set_fl(int fd, int flag){
    int val = fcntl(fd, F_GETFL, 0);
    if (val <0){
        printf("fcntl getfl error");
    }
    val |= flag;
    if (fcntl(fd, F_SETFL, val)<0){
        printf("fcntl setfl error");
    }
}

void reset_request(int fd, int *client_status, fd_set* readfds,request* reqP){
    client_status[fd]=0;
    FD_CLR(fd, readfds);
    close(reqP->conn_fd);
    free_request(reqP);
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));
    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    // Open preorderRecord file
    int fd_pre = open("preorderRecord", O_RDWR);
    ssize_t n;
    if (fd_pre <0){
        printf("Can't open preorderRecord");
        return -1;
    }
    else{
        fprintf(stderr,"successfully opened, fd: %d\n", fd_pre);
    }
    Order preOrder[20];
    if ((n = read(fd_pre, preOrder, sizeof(Order)*20))<0){
        printf("Read error");
        return -1;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(svr.listen_fd, &readfds);
    int client_status[maxfd];
    for (int i=0; i<maxfd; i++) client_status[i]=0;
    int lock_id[20]={0};
    while (1) {
        // TODO: Add IO multiplexing
        fd_set tempfds = readfds;
        printf("start!\n");
        if (select(maxfd, &tempfds, NULL, NULL, NULL)<0){
            printf("server error!\n");
            continue;
        }
        for (int fd = 0; fd < maxfd; fd++){
            if (!FD_ISSET(fd,&tempfds)) continue;
            printf("current fd： %d\n", fd);
            if (fd == svr.listen_fd){
                // Check new connection
                clilen = sizeof(cliaddr);
                conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                if (conn_fd < 0) {
                    if (errno == EINTR || errno == EAGAIN) continue;  // try again
                    if (errno == ENFILE) {
                        (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                        continue;
                    }
                    ERR_EXIT("accept");
                }
                set_fl(conn_fd, O_NONBLOCK);
                FD_SET(conn_fd, &readfds);
                client_status[conn_fd] = 1;
                requestP[conn_fd].conn_fd = conn_fd;
                strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                sprintf(buf, "Please enter the id (to check how many masks can be ordered):\n");
                write(requestP[conn_fd].conn_fd, buf, strlen(buf)); 

            }
            else if (client_status[fd]==1){
                // TODO: handle requests from clients
                handle_read(&requestP[fd]); // parse data from client to requestP[fd].buf
                fprintf(stderr, "%s", requestP[fd].buf);
                int ID = requestP[fd].id = find_id(atoi(requestP[fd].buf), preOrder);
#ifdef READ_SERVER      
                // sprintf(buf,"%s : %s",accept_read_header,requestP[fd].buf);

                if (ID>=0){
                    if (check_lock(fd_pre, ID)<0 || lock_id[ID]){
                        sprintf(buf, "Locked.\n");
                    }
                    else{
                        sprintf(buf, "You can order %d adult mask(s) and %d children mask(s).\n", preOrder[ID].adultMask, preOrder[ID].childrenMask);
                    }
                }
                else{
                    sprintf(buf, "Operation failed.\n");
                }
                write(requestP[fd].conn_fd, buf, strlen(buf));
                reset_request(fd, client_status, &readfds, &requestP[fd]);
#else 
                // sprintf(buf,"%s : %s",accept_write_header,requestP[fd].buf);
                if (ID>=0){
                    if (check_lock(fd_pre, ID)<0 || lock_id[ID]){
                        sprintf(buf, "Locked.\n");
                        write(requestP[fd].conn_fd, buf, strlen(buf));
                        reset_request(fd, client_status, &readfds, &requestP[fd]);
                    }
                    else{
                        set_lock(fd_pre, ID, F_WRLCK);
                        lock_id[ID] = 1;
                        sprintf(buf, "You can order %d adult mask(s) and %d children mask(s).\nPlease enter the mask type (adult or children) and number of mask you would like to order:\n", preOrder[ID].adultMask, preOrder[ID].childrenMask);
                        write(requestP[fd].conn_fd, buf, strlen(buf)); 
                        client_status[fd]=2;
                    }
                }
                else{
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[fd].conn_fd, buf, strlen(buf));
                    reset_request(fd, client_status, &readfds, &requestP[fd]);
                }
#endif
            }
            else if (client_status[fd]==2){
                int ID = requestP[fd].id;
                printf("connid %d ,id %d\n", requestP[fd].conn_fd, requestP[fd].id);
                if (handle_order(&requestP[fd], preOrder)<0){
                    sprintf(buf, "Operation failed.\n");
                    write(requestP[fd].conn_fd, buf, strlen(buf));
                }
                else{
                    if (lseek(fd_pre, sizeof(Order)*ID, SEEK_SET)>=0){
                        write(fd_pre, &preOrder[ID], sizeof(Order));
                    }
                }
                lock_id[ID] = 0;
                set_lock(fd_pre, ID, F_UNLCK);
                reset_request(fd, client_status, &readfds, &requestP[fd]);
            }
        }
    }
    free(requestP);
    close(fd_pre);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
