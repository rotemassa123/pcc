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

int updatePcc(char * data_buff, int file_size){
    int count = 0;
    int b;
    for(int i = 0; i < file_size; i++){
        b = (int) data_buff[i];
        if(b >= 32 && b <= 126){
            count++;
            pcc_total[b]++;
        }
    }

    return count;
}

void handleConnection(int fd){
    //read file_size from client
    uint32_t num_read_from_client;
    char * file_size_str = (char *)&num_read_from_client;
    int read_this_time;
    int read_so_far = 0;

    while(sizeof(uint32_t) > read_so_far){
        read_this_time = read(fd, file_size_str + read_so_far, sizeof(uint32_t) - read_so_far);
        read_so_far += read_this_time;
    }
    int file_size = ntohl(num_read_from_client);

    //read file data from client
    char * data_buff = malloc(file_size);
    read_so_far = 0;
    while(file_size > read_so_far){
        read_this_time = read(fd, data_buff + read_so_far, file_size - read_so_far);
        read_so_far += read_this_time;
    }

    int count_of_printable_chars;

    //if(!has_recieved_SIGINT)
    count_of_printable_chars = htonl(updatePcc(data_buff, file_size));
    char * count_of_printable_chars_buff = (char *)&count_of_printable_chars;
    int sent = 0;
    int sent_this_iteration;
    while( sizeof(uint32_t) > sent )
    {
        sent_this_iteration = write(fd, count_of_printable_chars_buff + sent, sizeof(uint32_t) - sent);
        if(sent_this_iteration < 0){ perror("write of file size to socket failed\n"); exit(-1); }
        sent  += sent_this_iteration;
    }
}

// MINIMAL ERROR HANDLING FOR EASE OF READING

int main(int argc, char *argv[])
{
    if(argc != 2){ printf("KAIZO KUNARA (wrong arguMeNTS)\n"); exit(-1); }
    memset(&pcc_total, 0, sizeof(pcc_total));

    int rt = 1;
    int listenfd  = -1;
    int connfd    = -1;
    short port = atoi(argv[1]);

    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    //char data_buff[1024];

    listenfd = socket( AF_INET, SOCK_STREAM, 0 );
    memset( &serv_addr, 0, addrsize );

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);


    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &rt, sizeof(int)) < 0){
        fprintf(stderr, "setsockopt Error: %s\n", strerror(errno));
        exit(1);
    }


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

        handleConnection(connfd);
        // close socket
        close(connfd);
    }
}