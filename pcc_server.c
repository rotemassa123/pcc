#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>


int pcc_total[126];

void handleConnection(int fd){
    int msg_len = ReadMsgLen(fd);

    char * buffer;
    readToBuffer(fd, buffer);

    int bytes_read;
    char buff[sizeof(unsigned int)];
    while((bytes_read = read(fd, buff, sizeof(buff) - 1)) > 0){
        buff[bytes_read] = '\0';
    }

    printf("# of printable characters: %u\n", strtol(str, &ptr, 10));
}

// MINIMAL ERROR HANDLING FOR EASE OF READING

int main(int argc, char *argv[])
{
    if(argc != 1){ printf("KAIZO KUNARA (wrong arguMeNTS)\n"); exit(-1); }
    memset(&pcc_total, 0, sizeof(pcc_total));

    int len       = -1;
    int n         =  0;
    int listenfd  = -1;
    int connfd    = -1;

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    char data_buff[1024];

    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    // INADDR_ANY = any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10000);

    if(0 != bind(listenfd, (struct sockaddr*) &serv_addr, addrsize))
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if(0 != listen(listenfd, 10))
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }

    while( 1 )
    {
        // Accept a connection.
        connfd = accept(listenfd, (struct sockaddr*) &peer_addr, &addrsize);

        if(connfd < 0)
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
        getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);
        printf( "Server: Client connected.\n"
                "\t\tClient IP: %s Client Port: %d\n"
                "\t\tServer IP: %s Server Port: %d\n",
                inet_ntoa( peer_addr.sin_addr ),
                ntohs(     peer_addr.sin_port ),
                inet_ntoa( my_addr.sin_addr   ),
                ntohs(     my_addr.sin_port   ) );

        handleConnection(connfd);
        // close socket
        close(connfd);
    }
}