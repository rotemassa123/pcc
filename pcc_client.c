#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>

void sendBuff(char* data_buff, int fd)
{
    int total_sent = 0;
    int len = strlen(data_buff);
    int sent_this_iteration;

    // keep looping until nothing left to write
    while( len > total_sent )
    {
        sent_this_iteration = write(fd, data_buff + total_sent, len - total_sent);

        if(sent_this_iteration < 0){ printf("write to fd failed\n"); exit(-1); }
        total_sent  += sent_this_iteration;
    }
}

char* readFromFd(int fd, int size_to_read){
    int read_so_far = 0;
    char * buff = malloc(size_to_read);
    while(size_to_read > read_so_far){
        bytes_read = read(fd, buff + read_so_far, sizeof(buff) - 1);
    }

    return buff;
}

int GetfileSize(char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

void printConnectionDetails(sockaddr_in my_addr, sockaddr_in peer_addr){
    // print socket details
    getsockname(sockfd, (struct sockaddr*) &my_addr,   &addrsize);
    getpeername(sockfd, (struct sockaddr*) &peer_addr, &addrsize);
    printf("Client: Connected. \n"
           "\t\tSource IP: %s Source Port: %d\n"
           "\t\tTarget IP: %s Target Port: %d\n",
           inet_ntoa((my_addr.sin_addr)),    ntohs(my_addr.sin_port),
           inet_ntoa((peer_addr.sin_addr)),  ntohs(peer_addr.sin_port));
}

int main(int argc, char *argv[])
{
    if(argc != 3){ printf("MUGA BUGA WRONG ARGUEMENTS"); exit(-1); }

    struct sockaddr_in serv_addr; // where we Want to get to
    struct sockaddr_in my_addr;   // where we actually connected through
    struct sockaddr_in peer_addr; // where we actually connected to

    int  sockfd     = -1;
    FILE * filefd;
    int  bytes_read =  0;
    char buff[1024];
    int file_size;
    errno_t err;

    char * server_ip = argv[1];
    unsigned short server_port = atoi(argv[2]);
    char * file_path = argv[3];

    err = fopen_s(&filefd, file_path, "r");

    socklen_t addrsize = sizeof(struct sockaddr_in );

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        { printf("\n Error : Could not create socket \n"); exit(-1); }

    printf("Client: socket created %s:%d\n", inet_ntoa((my_addr.sin_addr)), ntohs(my_addr.sin_port));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    printf("Client: connecting...\n");
    if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed. %d \n", errno);
        exit(-1);
    }

    printConnectionDetails(my_addr, peer_addr);

    //write size of file to server
    if((file_size = GetfileSize(file_path)) < 0) { printf("couldn't read file size!\n"); exit(-1); }
    file_size = htonl(file_size);
    sendBuff((char *)&file_size, sockfd);

    // write data from file to server
    while((bytes_read = read(filefd, buff, sizeof(buff) - 1)) > 0)
    {
        buff[bytes_read] = '\0';
        sendBuff(buff, sockfd);
    }

    readFromFdAndPrintToScreen(sockfd);

    close(sockfd);
    exit(0);
}