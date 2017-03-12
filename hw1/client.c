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
#include <stdarg.h>      
#include <syslog.h>

#define MAXLINE 4096
#define PORT 4000
int     daemon_proc;
ssize_t writen(int fd, const void* vptr, size_t n);
ssize_t readline(int fd, void* vptr, size_t maxlen);
static ssize_t my_read(int fd, char* ptr);
void err_quit(const char *fmt, ...);
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);

void str_cli(FILE * fp, int sockfd)
{
    int maxfdp1,stdineof=0,n; 
    fd_set rset; 
    char sendline[MAXLINE], recvline[MAXLINE];
    FD_ZERO(&rset);
    for( ; ; )
    {
        FD_SET(fileno(fp),&rset);
        FD_SET(sockfd, &rset);
        if(fileno(fp)>sockfd)  maxfdp1 = fileno(fp) +1;
        else maxfdp1 = sockfd +1;
        select(maxfdp1, &rset,NULL, NULL,NULL);
        if(FD_ISSET(sockfd, &rset))
        {
            if( (n=readline(sockfd, recvline,MAXLINE)) == 0)
            {
                if(stdineof == 1) return;
                else err_quit("str_cli: server terminated prematurely");
            }
            else 
            {
                //write(fileno(stdout), buf, n);
                fputs(recvline,stdout);
               // fflush(stdout);
            }
        }
        if(FD_ISSET(fileno(fp), &rset))
        {
            if( fgets(sendline,MAXLINE,fp) == NULL)
            {
                return;
            }
            else
            {
               if(strcmp(sendline,"exit\n") == 0) return ; //close the connection
                else writen(sockfd, sendline, strlen(sendline));
            }
        }
    }
}

int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    
    if (argc != 3) err_quit("./client <SERVER IP> <SERVER PORT>");
    if((sockfd= socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        printf("socket error");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        printf("inet_pton error for %s", argv[1]);
    
    if(connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        printf("connect error\n");
        perror("error");
        exit(1);
    }
    str_cli(stdin, sockfd);
    exit(0);
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
        if((rc=read(fd, &c,1)) == 1) { *ptr++ = c;  if(c=='\n') break;}
        else if(rc == 0) {*ptr=0; return (n-1);}
        else return (-1);
    }
    *ptr = 0;
    return n;
}

void err_quit(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    int     errno_save, n;
    char    buf[MAXLINE + 1];

    errno_save = errno;     /* value caller might want printed */
#ifdef  HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);   /* safe */
#else
    vsprintf(buf, fmt, ap);                 /* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");

    if (daemon_proc) {
        syslog(level, buf);
    } else {
        fflush(stdout);     /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}
