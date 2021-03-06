#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#define PORT 4000
#define MAXLINE 1450

void dg_echo(int sockfd, struct sockaddr * pcliaddr, socklen_t clilen);

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        fputs("Cannot create socket.\n", stderr);
        exit(2);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family  = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
    if( bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
    {
        fputs("Cannot bind.\n", stderr);
         exit(2);
    }

    dg_echo(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
    return 0;
  
}

void dg_echo(int sockfd, struct sockaddr * pcliaddr, socklen_t clilen)
{
    int        n;
    socklen_t  len;
    char   recvline[MAXLINE];
    
    char  ack[6];
    char * syn; 
    int  ACK = 0;
    char * name;
    long size=0, now_size=0;
    int read_cnt = 0, tmp =0;

    for ( ; ; ) {
        len = clilen;
        n = recvfrom(sockfd, recvline, MAXLINE+7, 0, pcliaddr, &len);
        recvline[n]=0;
        syn = strtok(recvline, " \n");
        if(ACK == atoi(syn))   //the expected packet
        {
            sprintf(ack,"%d",ACK++);
            sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
            name = strtok(NULL, " \n");
            size = atoi(strtok(NULL, " \n")); 
            break;  
        }
    }
    FILE *pFile=fopen(name,"w");
    while(now_size < size)
    {
        len = clilen;
        n = recvfrom(sockfd, recvline, MAXLINE+7, 0, pcliaddr, &len);
        recvline[n]=0;
        syn = strtok(recvline, " ");
        if(ACK == atoi(syn)) 
        {
            tmp = strlen(syn)+1;
            write(fileno(pFile),recvline+tmp,n-tmp);
            sprintf(ack,"%d",ACK++);
            now_size+= n-tmp;
        }
        sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
    }
    fclose(pFile);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while(1) //readable_timeo(sockfd,5) == 0  ->timeout
    {
        if(n < 0)
        {
            if(errno == EWOULDBLOCK)
            {
                printf("timeout\n");
                 sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
            }
            else 
            {
                printf("recvfrom error\n");
                sendto(sockfd, ack, strlen(ack), 0, pcliaddr, len);
            }
        }
        return;
    }
    return ;

}

int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return (select(fd+1, &rset, NULL, NULL, &tv));
}