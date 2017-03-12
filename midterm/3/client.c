#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define LISTENQ 1024
#define MAXLINE 4096
#define BUFF_MAX 1024
#define CMD_MAX 1024


ssize_t readline(int fd, void * vptr, size_t maxlen);

int main(int argc, char * argv[])
{
    if(argc!=3)
    {
        fprintf(stderr,"Usage: %s <server_host> <port>\n", argv[0]);
        exit(1);
    }

    struct hostent *host = gethostbyname(argv[1]);
    if(host == NULL)
    {
        fprintf(stderr,"Cannot get host by %s.\n", argv[1]);
        exit(2);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1)
    {
        fputs("Fail to create socket.\n",stderr);
        exit(2);
    }
    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        fputs("Fail to connect.\n",stderr);
        exit(2);
    }

    while(1)
    {
        char cmd [BUFF_MAX]; bzero(cmd,CMD_MAX);
        char buff [BUFF_MAX]; bzero(buff,BUFF_MAX);
        int i, n;
        fgets(cmd, CMD_MAX, stdin);
        if(strcmp(cmd,"\n")==0)continue;
        strcpy(buff,cmd);
        //write(sock_fd, cmd, strlen(cmd));

        char * command = strtok(cmd, " \n");
        if(strcmp(command, "PUT") == 0)
        {
            FILE *pFile; 
            char * name1 = strtok(NULL, " \n");
            if(name1 == NULL ){ printf("Please input correct command\n"); continue; }
            char * name2 = strtok(NULL, " \n");
            if(name2 == NULL ){ printf("Please input correct command\n"); continue; }
            pFile = fopen(name1, "r");
            if(pFile == NULL) { printf("Source file open failed\n"); continue; }
            write(sock_fd, buff, strlen(buff));
            fseek(pFile,0,SEEK_END);
            long lSize = ftell(pFile);
            rewind(pFile);
            char tmp[20];
            for(i=0; i<20; i++) tmp[i] = 0;
            sprintf(tmp,"%ld\n",lSize);
            write(sock_fd, tmp, strlen(tmp));
            fclose(pFile); pFile=fopen(name1,"r");
            long read_cnt=0;
            while(read_cnt = read(fileno(pFile),buff,BUFF_MAX))
            {
                if(read_cnt < 0){ printf("client read error\n");break;}
                else 
                {
                    n= write(sock_fd, buff, read_cnt);
                    if(n<0)printf("client write error\n");
                }    
            }
            fclose(pFile);
            printf("PUT %s %s succeeded\n",name1, name2);
        }
        else if(strcmp(command, "GET") == 0)
        {
            FILE *pFile; 
            char * name1 = strtok(NULL, " \n");
            if(name1 == NULL ){ printf("Please input correct command\n"); continue; }
            char * name2 = strtok(NULL, " \n");
            if(name2 == NULL ){ printf("Please input correct command\n"); continue; }
            write(sock_fd, buff, strlen(buff));
            n = readline(sock_fd, buff,BUFF_MAX);
            if(strcmp(buff,"file doesn't exist\n") == 0)
            {
                printf("file doesn't exist\n"); 
                continue ; 
            }
            pFile = fopen(name2, "w");
            if(pFile == NULL) { printf("Source file open failed\n"); continue; }
            long length_n=atoi(buff);
            long result = 0, read_cnt = 0;
            while( result < length_n )
            {
                read_cnt= read(sock_fd,buff,BUFF_MAX);
                if(read_cnt < 0){ printf("client read error\n");break;}
                else write(fileno(pFile),buff,read_cnt );
                result += read_cnt;
            }
            fclose(pFile);
            printf("GET %s %s succeeded\n",name1, name2);
        }
        else if(strcmp(command, "LIST") == 0)
        {
            write(sock_fd, buff, strlen(buff));
            int read_cnt= 0;
            while((read_cnt= readline(sock_fd,buff,BUFF_MAX)) && strcmp(buff, "Finish List\n"))
            {
                if(read_cnt < 0) { printf("client read error\n");break;}
                else if(strcmp(buff, "zzzz.txt\n")) write(1,buff,read_cnt);
            }
            printf("LIST succeeded\n");
        }
        else if(strcmp(command, "EXIT") == 0)
        {
            write(sock_fd, buff, strlen(buff));
            close(sock_fd); return 0;
        }
        else
        {
            printf("wrong input\n");
        }
    }

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
