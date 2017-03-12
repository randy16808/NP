#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdarg.h>      
#include <syslog.h>
#include <sys/select.h>
#define SA struct sockaddr
#define MAXLINE 4096
#define LISTENQ 1024
#define PORT 4000


ssize_t writen(int fd, const void* vptr, size_t n);
ssize_t readline(int fd, void* vptr, size_t maxlen);
static ssize_t my_read(int fd, char* ptr);


void str_echo(int sockfd)
{
    ssize_t n;
    char line[MAXLINE];
    for( ; ;)
    {
        n=readline(sockfd, line, MAXLINE);
        if(n==0) return;
        writen(sockfd, line, n);    
    }
}

int main(int argc, char** argv)
{
    int i,j,maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    char name[FD_SETSIZE][13]; 
    char anonymous[]="anonymous"; 
    int error = 1;
    int port[FD_SETSIZE]; char* addr[FD_SETSIZE];
    ssize_t n; fd_set rset,allset; char line[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in servaddr,cliaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);
    maxfd=listenfd; maxi=-1;
    for(i=0; i<FD_SETSIZE; i++) client[i]=-1;
    FD_ZERO(&allset); FD_SET(listenfd, &allset);
    for( ; ;)
    {
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if(FD_ISSET(listenfd, &rset))
        {
            clilen=sizeof(cliaddr);
            connfd=accept(listenfd,(SA *)&cliaddr, &clilen);
            for(i=0; i<FD_SETSIZE; i++) 
            { 
                if(client[i]<0) 
                {
                    client[i]=connfd; 
                    strcpy(name[i],"anonymous"); 
                    break;
                } 
            }
            if(i == FD_SETSIZE) {perror("too many clients"); exit(1);}
            FD_SET(connfd,&allset);
            if(connfd > maxfd) maxfd = connfd;
            if(i > maxi) maxi = i;
            for(i=0; i<=maxi; i++) 
            { 
                
                if(client[i] == connfd )
                {
                    char str[16]; char hello [60];
                    addr[i] = (char *)inet_ntop(AF_INET,&cliaddr.sin_addr, str, sizeof(str));
                    port[i] = ntohs(cliaddr.sin_port);
                    sprintf(hello, "[Server] Hello, anonymous! From: %s/%d\n",addr[i],port[i]);
                    hello[strlen(hello)]='\0';
                    writen(client[i],hello, strlen(hello));
                } 
                else if(client[i] != -1) 
                {
                    char welcome[28]; 
                    sprintf(welcome, "[Server] Someone is coming!\n");
                    welcome[strlen(welcome)]='\0';
                    writen(client[i],welcome, strlen(welcome));
                }
            }
            if(--nready <= 0) continue;
        }

        for(i=0; i<=maxi; i++)
        {
            if((sockfd=client[i]) < 0) continue;
            if(FD_ISSET(sockfd,&rset))
            {
                if( (n=readline(sockfd,line,MAXLINE)) ==0 )
                {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] =-1;
                    for( j=0; j<=maxi; j++) 
                    { 
                        if(client[j] != -1) 
                        {
                            char hello[60];
                            sprintf(hello, "[Server] %s is offline.\n",name[i]);
                            hello[strlen(hello)]='\0';
                            writen(client[j],hello, strlen(hello));
                        }
                    }
                }
                else
                {
                    if(line[0]=='w' && line[1]=='h' && line[2]=='o' && line[3]=='\n')
                    {
                        error=0;
                        for( j=0; j<=maxi; j++) 
                        { 
                            if(client[j] == sockfd)
                            {
                                char user[50];
                                sprintf(user, "[Server] %s %s/%d ->me\n",name[j],addr[j],port[j]);
                                writen(sockfd,user, strlen(user));
                                //[Server] <USERNAME> <CLIENT IP>/<CLIENT PORT>
                                //[Server] <SENDER USERNAME> <CLIENT IP>/<CLIENT PORT> ->me
                            }
                            else if(client[j] != -1) 
                            {
                                char user[50];
                                sprintf(user, "[Server] %s %s/%d\n",name[j],addr[j],port[j]);
                                writen(sockfd,user, strlen(user));
                            }
                        }
                    }
                    
                    else if(line[0]=='n' && line[1]=='a' && line[2]=='m' && line[3]=='e' && line[4]==' ')
                    {   
                        error=0; char str[50];
                        strcpy(str, line+5); str[strlen(str)-1]='\0'; 
                        if(!strcmp(str,anonymous)) 
                        {
                            char rename[50];
                            sprintf(rename, "[Server] ERROR: Username cannot be anonymous.\n");
                            writen(sockfd,rename, strlen(rename));
                        }
                        else  //unique
                        {
                            int  flag = 1;
                            for( j=0; j<=maxi; j++) 
                            { 
                                if(client[j] != -1 && strcmp(name[j],str)) continue; 
                                else { flag = 0; break;}
                            }
                            if(!flag)
                            {
                                char rename[50];
                                sprintf(rename, "[Server] ERROR: %s has been used by others.\n",str);
                                writen(sockfd,rename, strlen(rename));
                            }
                            else //2-12 english letter
                            {
                                int length = 0; flag =1;
                                for(j=0; j<strlen(str);j++)
                                {
                                    if(str[j]=='\n') break;
                                    else if( (str[j]>='a' && str[j]<='z') || (str[j]>='A' && str[j]<='Z')) length++;
                                    else {flag = 0; break;}
                                }
                                if(flag && length > 1 && length <= 12) //right form
                                {
                                    int tmp = 0;
                                    for(j=0; j<=maxi; j++) 
                                    { 
                                        
                                        if(client[j] == sockfd )
                                        {
                                            char rename[50];
                                            sprintf(rename, "[Server] You're now known as %s.\n",str);
                                            writen(sockfd,rename, strlen(rename));
                                        } 
                                        else if(client[j] != -1) 
                                        {
                                            char rename[50];
                                            sprintf(rename, "[Server] <%s> is now known as %s.\n",name[i],str);
                                            writen(client[j],rename, strlen(rename));
                                        }
                                    }
                                    strcpy(name[i],str);

                                } 
                                else //[Server] ERROR: Username can only consists of 2~12 English letters
                                {
                                    char rename[50];
                                    sprintf(rename, "[Server] ERROR: Username can only consists of 2~12 English letters\n");
                                    writen(sockfd,rename, strlen(rename));
                                }  
                            }
                        }
                    }
                    else if(line[0]=='t' && line[1]=='e' && line[2]=='l' && line[3]=='l' && line[4]==' ')
                    {
                        error=0;
                        if(!strcmp(name[i],anonymous))
                        {
                            char send[50];
                            sprintf(send, "[Server] ERROR: You are anonymous.\n");
                            writen(sockfd,send, strlen(send));
                        }
                        else if(line[5]=='a' && line[6]=='n' && line[7]=='o' && line[8]=='n' && line[9]=='y'
                            && line[10]=='m' && line[11]=='o' && line[12]=='u' && line[13]=='s' && line[14]==' ') 
                        {
                            char send[50];
                            sprintf(send, "[Server] ERROR: Username cannot be anonymous.\n");
                            writen(sockfd,send, strlen(send));
                        }
                        else
                        {
                            char b[13];
                            for(j=0; j<13; j++) b[j]=0;
                            for(j=0; j<13; j++) 
                            {
                                if(line[5+j] == ' ') {break;} 
                                b[j]=line[5+j]; 
                            }
                            int find = 0; int tmp = j;
                            if(j<13) 
                                for(j=0; j<=maxi; j++)  
                                { 
                                    if(!strcmp(b,name[j])) 
                                        {find = 1; break;}

                                }    
                            if(find)
                            {
                                char recv[MAXLINE], s[MAXLINE]; strcpy(s,line+5+tmp);
                                sprintf(recv, "[Server] %s tell you %s",name[i],s);
                                n = writen(client[j],recv, strlen(recv));
                                if(n>0)
                                {
                                    char send[50];
                                    sprintf(send, "[Server] SUCCESS: Your message has been sent.\n");
                                    writen(sockfd,send, strlen(send));
                                }
                            }
                            else
                            {
                                char send[50];
                                sprintf(send, "[Server] ERROR: The receiver doesn't exist.\n");
                                writen(sockfd,send, strlen(send));
                            }
                        }   
                    }
                    else if(line[0]=='y' && line[1]=='e' && line[2]=='l' && line[3]=='l' && line[4]==' ')
                    {
                        error = 0; char str[MAXLINE]; strcpy(str,line+5);
                        char yell[MAXLINE];
                        sprintf(yell,"[Server] %s yell %s", name[i], str);
                        for(j=0; j<=maxi; j++)  
                        {
                            if(client[j]!=-1)  writen(client[j], yell, strlen(yell));
                        }
                    }
                    else
                    {
                        char Error[50];
                        sprintf(Error,"[Server] ERROR: Error command.\n");
                        writen(sockfd, Error, strlen(Error));
                    }
                } 
                if(--nready <= 0) break;

            }
        }
    }

}

ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char* ptr;

    ptr = (char *)vptr;
    nleft = n;
    while(nleft > 0)
    {
        if((nwritten = write(fd, ptr,nleft)) <= 0)
        {
            if(nwritten<0 && errno==EINTR) nwritten = 0;
            else return(-1);
        }
        nleft -= nwritten; ptr += nwritten;
    }
    return(n);
}

static int read_cnt;
static char*read_ptr;
static char read_buf[MAXLINE];
static ssize_t 
my_read(int fd, char* ptr)
{
    if(read_cnt <= 0)
    {
        again:
        if( (read_cnt=read(fd,read_buf,sizeof(read_buf)))<0)
        {
            if(errno==EINTR) goto again; 
            return -1;
        }
        else if(read_cnt == 0) return 0;
        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;
    return 1;
}

ssize_t readline(int fd, void*vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;
    ptr = (char*)vptr;
    for(n=1 ; n<maxlen ; n++)
    {
        if((rc=my_read(fd, &c)) == 1) { *ptr=c;ptr++; if(c=='\n') break;}
        else if(rc == 0) {*ptr=0; return (n-1);}
        else return (-1);
    }
    *ptr = 0;
    return n;
}
