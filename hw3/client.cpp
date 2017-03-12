#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>      
#include <syslog.h>
#include <algorithm>
using namespace std;
#define CMD_MAXLINE 1024
#define MAXLINE 4096
#define Input_MAXLINE 1024
#define NOTHING 0
#define SEND 1
#define RECEIVE 2

ssize_t writen(int fd, const void* vptr, size_t n);
ssize_t readline(int fd, void* vptr, size_t maxlen);
static ssize_t my_read(int fd, char* ptr);
void show_progress(long now, long file);

int main(int argc, char** argv)
{
    int sockfd, datafd;
    struct sockaddr_in servaddr;
    if (argc != 4) 
    {
        fprintf(stderr,"Usage: %s <ip> <port> <username>\n", argv[0]);
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        fputs("Fail to create socket.\n",stderr);
        exit(2);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        printf("inet_pton error for %s", argv[1]);
        exit(1);
    }
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("connect error\n");
        exit(1);
    }
    
    char send[MAXLINE], receive[MAXLINE];
    char input[Input_MAXLINE];
    char cmd_stdin[CMD_MAXLINE], cmd_sock[CMD_MAXLINE];
//when connect to server, tell server its name
    sprintf(send,"username %s",argv[3]);
    write(sockfd, send, strlen(send));
    printf("Welcome to the dropbox-like server! : <%s>\n",argv[3]);

    pid_t p = getpid(); 

    int maxfdp1, val, stdineof, sending=0, receiving=0;
    int i,j,k;
    ssize_t n, nwritten;
    fd_set rset, wset;

    val = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
    val = fcntl(fileno(stdin), F_GETFL, 0);
    fcntl(0, F_SETFL, val | O_NONBLOCK);
    val = fcntl(fileno(stdout), F_GETFL, 0);
    fcntl(1, F_SETFL, val | O_NONBLOCK);

    int count =0 ;
    char *tok=NULL;
    
    char send_name[30], receive_name[30];
    stdineof = 0;
    
    FILE *pFile;
    struct stat fileStat;
    long send_size = 0, send_now=0;
    long recv_size=0, recv_now=0;

    maxfdp1 = sockfd + 1;
    for( ; ;)
    {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(fileno(stdin), &rset);  //read from stdin
        FD_SET(sockfd, &rset); 
        if(sending)
            FD_SET(sockfd, &wset);  //data to write to socket
        
        select(maxfdp1, &rset, &wset, NULL, NULL);

        if(FD_ISSET(fileno(stdin), &rset))
        {
            if( (n=read(fileno(stdin), input, Input_MAXLINE)) < 0 ){
                if(errno != EWOULDBLOCK){
                    printf("read error on stdin");
                    exit(1);
                }
            } else if(n == 0){
                stdineof = 1;
            } else{
                tok = strtok(input," \n\r");
                if(tok == NULL) continue;
                else if(strcmp(tok,"/exit") == 0)
                {
                    close(sockfd);
                    return 0;
                }
                else if(strcmp(tok, "/sleep") == 0)
                {
                    tok = strtok(NULL," \n\r");
                    if(tok == NULL)
                    {
                        printf("command wrong\n");
                        continue;
                    }
                    else
                    {
                        int sleep_cnt = atoi(tok);
                        printf("Client starts to sleep\n");
                        for(i=0; i<sleep_cnt; i++)
                        {
                            sleep(1);
                            printf("Sleep %d\n",i+1);
                        }
                        printf("Client wakes up\n");
                        
                    }
                }
                else if(strcmp(tok, "/put") == 0)
                {
                    tok = strtok(NULL," \n\r");
                    if(tok == NULL)
                    {
                        printf("command wrong\n");
                        continue;
                    }
                    else
                    {
                        strcpy(send_name, tok);
                        if(stat(send_name,&fileStat) < 0) {printf("file error\n"); break;}
                        send_size = fileStat.st_size;
                        sprintf(send,"put %s %ld\n",send_name,send_size);
                        write(sockfd,send,strlen(send));
                        pFile = fopen(send_name, "r");
                        sending = 1;
                        send_now = 0;
                        printf("Uploading file: <%s>\n",send_name);
                        FD_SET(sockfd, &wset);
                    }
                }
                else
                {
                    printf("commnad wrong\n");
                    continue;
                }
            }
        }
        if(FD_ISSET(sockfd, &rset))
        {
            if(receiving)
            {   
                n = read(sockfd, receive, min((long)MAXLINE,recv_size-recv_now));
                if(n < 0)
                {

                }
                else if(n)
                {
                    nwritten = fwrite(receive, sizeof(char), n, pFile);
                    recv_now += nwritten;
                    show_progress(recv_now, recv_size);
                    if(recv_now == recv_size)
                    {
                        receiving = 0;
                        printf("Progress: 100\%\n");
                        printf("Download <%s> complete!\n",receive_name);
                        fclose(pFile);
                    }
                }
            }
            else
            {
                n = readline(sockfd, receive, CMD_MAXLINE);
                //printf("receive : %s\n",receive);
                if(n == 0)
                {
                    printf("server close the connection\n");
                    exit(0);
                }
                else
                {
                    tok = strtok(receive, " ");
                    if(strcmp(tok, "get") == 0)
                    {
                        tok = strtok(NULL, " ");
                        strcpy(receive_name,tok);
                        char tmp[30];
                        sprintf(tmp,"%d_%s",(int)p,tok);
                        pFile = fopen(tmp, "w");
                        tok = strtok(NULL, " ");
                        recv_size = atoi(tok);  
                        recv_now = 0;
                        receiving = 1;
                        printf("Downloading file: <%s>\n",receive_name);         
                    }
                }
            }
        }
        if(FD_ISSET(sockfd, &wset) && sending)
        {
            n = fread(send,sizeof(char),MAXLINE,pFile);
            nwritten = write(sockfd,send,n);
            if(nwritten < 0)
            {
                if(errno != EWOULDBLOCK)
                {
                    printf("write error to socket\n");
                    exit(1);
                }
            }
            else
            {
                send_now += nwritten;
                show_progress(send_now, send_size);
                if(send_now == send_size)
                {
                    sending = 0;
                    printf("Progress: 100\%\n");
                    printf("Upload <%s> complete!\n",send_name);
                    fclose(pFile);
                }
            }
        }

    }

    exit(0);
}



void show_progress(long now, long file)
{
    int percent = now * 100 / file;
    printf("Progress: %d\%\r",percent);
    fflush(stdout);
    return ;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
    ptr = (char*)vptr;
    for(n = 1; n < maxlen ; n++)
    {
        if((rc=read(fd,&c,1)) == 1)
        {
            *ptr++=c;
            if(c == '\n') break;
        }
        else if(rc == 0)
        {
            *ptr = 0;
            return (n-1);
        }   
        else return -1;
    }
    *ptr = 0;
    return n;
}