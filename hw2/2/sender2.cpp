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

int readable_timeo(int fd, int sec,int usec);
void dg_cli(FILE *fp, int sockfd, const struct sockaddr * pservaddr, socklen_t servlen,char* name1, char* name2);

static void sig_alarm(int signo)
{
    return;
}

int main(int argc, char **argv)
{
        int  sockfd;
        struct sockaddr_in      servaddr;

        if(argc!=5)
        {
            fprintf(stderr,"Usage: %s <server_host> <port>\n", argv[0]);
            exit(1);
        }
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(atoi(argv[2]));
        inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd == -1)
        {
            fputs("Fail to create socket.\n",stderr);
            exit(2);
        }
        
        dg_cli(stdin, sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr), argv[3],argv[4]);
        exit(0);
}

void dg_cli(FILE *fp, int sockfd, const struct sockaddr * pservaddr, socklen_t servlen,char* name1, char* name2)
{
        int     n;
        char    sendline[MAXLINE+7], recvline[MAXLINE], buffer[MAXLINE];
        bzero(sendline,MAXLINE+7); bzero(recvline,MAXLINE); bzero(buffer,MAXLINE);
        socklen_t len;
        struct sockaddr* preply_addr;
        
        signal(SIGALRM,sig_alarm);
        siginterrupt(SIGALRM,1);

        char* ack;
        char syn[6];
        int SYN=0;
        int read_cnt=0;
        
        struct stat fileStat;
        if(stat(name1,&fileStat) < 0) {printf("file error\n"); return;}
        long lSize = fileStat.st_size, now_size=0;

        sprintf(sendline,"%d %s %ld\n",SYN, name2,lSize);
        read_cnt = strlen(sendline)+1;
        
        FILE *pFile=fopen(name1,"r");

        while (now_size < lSize+1) 
        {
            sendto(sockfd, sendline, read_cnt+strlen(syn), 0, pservaddr, servlen);
           
            if(readable_timeo(sockfd, 1,0) == 0)
            {
               // printf("time out \n");
            }
            else
            {
                n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr, &len);
                recvline[n] = 0;  
                ack = strtok(recvline, " \n");

                if(SYN == atoi(ack)) 
                {
                    if(now_size == lSize) break;
                    read_cnt = read(fileno(pFile),buffer,MAXLINE);
                    if(read_cnt < 0) { printf("client read error\n");break;}
                    else if(read_cnt == 0) {printf("end of file\n");break;}
                    else
                    {
                        now_size += read_cnt;
                        sprintf(syn,"%d ",++SYN);
                        int i=0,j=0;
                        for(;i<strlen(syn) ; i++) sendline[i] = syn[i];
                        for(; j<MAXLINE; i++,j++) sendline[i] = buffer[j];
                    }
                } 
            }

        }

        fclose(pFile);
        return;
}

int readable_timeo(int fd, int sec,int usec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return (select(fd+1, &rset, NULL, NULL, &tv));
}