#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <signal.h>


int pcc_total[126];
int connection = -1;
int has_recieved_sigint = 0;

int ReadOrWriteError(int num){
    if( errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE ){
        fprintf(stderr, "Read/Write data to/from client error: %s\n", strerror(errno));
        return errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE;
    }
    else if(num == 0){
        fprintf(stderr, "client died unexpectedly: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}

void printPcc(){
    for(int i = 32; i < 126; i++)
        printf("char '%c' : %u times\n", i, pcc_total[i]);
    exit(0);
}

void handler(){
    if(connection >= 0)
        has_recieved_sigint = 1;
    else{
        printPcc();
        exit(0);
    }
}

int updatePcc(char * data_buff, int file_size){
    int count = 0;
    int b;
    for(int i = 0; i < file_size; i++){
        if(!has_recieved_sigint){
            b = (int) data_buff[i];
            if(b >= 32 && b <= 126){
                count++;
                pcc_total[b]++;
            }
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
        if(ReadOrWriteError(read_this_time)) { return; }
        read_so_far += read_this_time;
    }
    int file_size = ntohl(num_read_from_client);

    //read file data from client
    char * data_buff = malloc(file_size);
    read_so_far = 0;
    while(file_size > read_so_far){
        read_this_time = read(fd, data_buff + read_so_far, file_size - read_so_far);
        if(ReadOrWriteError(read_this_time)){ return; }
        read_so_far += read_this_time;
    }

    int count_of_printable_chars;

    count_of_printable_chars = htonl(updatePcc(data_buff, file_size));
    char * count_of_printable_chars_buff = (char *)&count_of_printable_chars;
    int sent = 0;
    int sent_this_iteration;
    while( sizeof(uint32_t) > sent )
    {
        sent_this_iteration = write(fd, count_of_printable_chars_buff + sent, sizeof(uint32_t) - sent);
        if(ReadOrWriteError(sent_this_iteration)){ return; }
        if(sent_this_iteration < 0){ perror("write of file size to socket failed\n"); exit(-1); }
        sent  += sent_this_iteration;
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2){ printf("wrong argument count!\n"); exit(-1); }
    memset(&pcc_total, 0, sizeof(pcc_total));

    struct sigaction sigint; //pretty much copied from HW 2
    sigint.sa_handler = &handler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sigint, 0) != 0) {
        fprintf(stderr, "Signal handle registration failed. Error: %s\n", strerror(errno));
        exit(1);
    }

    int rt = 1;
    int listenfd  = -1;
    short port = atoi(argv[1]);

    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

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
    { printf("\n Error : Bind Failed. %s \n", strerror(errno)); return 1; }

    if(0 != listen(listenfd, 10))
    { printf("\n Error : Listen Failed. %s \n", strerror(errno)); return 1; }

    while( 1 )
    {
        if(has_recieved_sigint)
            printPcc();

        // Accept a connection.
        connection = accept(listenfd, (struct sockaddr*) &peer_addr, &addrsize);
        if(connection < 0) { printf("Accept Failed. %s \n", strerror(errno)); return 1; }

        handleConnection(connection);

        // close socket
        close(connection);
        connection = -1;
    }
}