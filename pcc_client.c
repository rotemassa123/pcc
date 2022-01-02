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

int GetfileSize(char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

int main(int argc, char *argv[])
{
    if(argc != 4){ printf("MUGA BUGA WRONG ARGUEMENTS\n"); exit(-1); }

    struct sockaddr_in serv_addr;

    int  sockfd     = -1;
    FILE * filefd;
    uint32_t file_size, file_size_to_send, num_read_from_server;

    char * server_ip = argv[1];
    unsigned short server_port = atoi(argv[2]);
    char * file_path = argv[3];

    filefd = fopen(file_path, "r");

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        { perror("Error : Could not create socket \n"); exit(1); }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        { perror("\n Error : Connect Failed. %d \n"); exit(1); }

    //write size of file to server
    if((file_size = GetfileSize(file_path) - 1) < 0) { perror("couldn't get file size!\n"); exit(1); }

    char * buff = malloc(file_size);
    if (fread(buff, 1, file_size, filefd) != file_size){
        fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
        exit(1);
    }

    file_size_to_send = htonl(file_size);
    char * file_size_buff = (char *)&file_size_to_send;

    //send file_size to server
    int sent = 0;
    int sent_this_iteration;
    while( sizeof(uint32_t) > sent )
    {
        sent_this_iteration = write(sockfd, file_size_buff + sent, sizeof(uint32_t) - sent);
        if(sent_this_iteration < 0){ perror("write of file size to socket failed\n"); exit(-1); }
        sent  += sent_this_iteration;
    }

    // write data from file to server
    sent = 0;
    while( file_size > sent )
    {
        sent_this_iteration = write(sockfd, buff + sent, file_size - sent);
        if(sent_this_iteration < 0){ perror("write file data to socket failed\n"); exit(-1); }
        sent  += sent_this_iteration;
    }

    //read count of printable chars from server
    char * count_of_printable_chars_str = (char *)&num_read_from_server;
    int read_this_time;
    int read_so_far = 0;
    while(sizeof(uint32_t ) > read_so_far){
        read_this_time = read(sockfd, count_of_printable_chars_str + read_so_far, sizeof(uint32_t ) - read_so_far);
        read_so_far += read_this_time;
    }

    num_read_from_server = ntohl(num_read_from_server);
    printf("# of printable characters: %u\n", num_read_from_server);

    close(sockfd);
    exit(0);
}