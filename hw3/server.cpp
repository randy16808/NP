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

#define CMD_MAXLINE 1024
#define MAXLINE 4096
#define MAXFILE 10
#define LISTENQ 1024
#define MAXCLIENT 20
#define MAXCLIENT_NAME 5
#define MAXHOST 10
#define NOTHING 0
#define SEND 1
#define RECEIVE 2

using namespace std;
ssize_t writen(int fd, const void* vptr, size_t n);
ssize_t readline(int fd, void* vptr, size_t maxlen);
static ssize_t my_read(int fd, char* ptr);

typedef struct{
    int used ;
    char name[20];
    int host_cnt;
    int file_cnt;
    int host_index[MAXHOST];
    int progress[MAXHOST];      //file transfer
    int file[MAXFILE];
    char file_name[MAXFILE][30];
} name_t;

typedef struct{
    int fd;
    char name[20];
    int order; //the order in the host of each client name
    int group;
    int sending;
    int receiving;
    long recv_length;
    long recv_now;
    long send_length;
    long send_now;
    char send[MAXLINE];
    char receive[MAXLINE];
    char send_name[30];
    char receive_name[30];
    char *SendFront;
    char *SendBack;
    char *RecvFront;
    char *RecvBack; 
    int file[MAXFILE];
    int file_cnt;
    int read_end;
    int send_file;
    FILE * pRecv;
    FILE * pSend;
} client_t;

name_t NameGroup[MAXCLIENT_NAME]; 
client_t clients[MAXCLIENT];

void NameGroup_init();
void clients_init();
int NameGroup_add(char * username, int index);
void AddFile(int index, int group, char * filename);
int check(int index);
int download_file(int index);
int send_write(int index,int n);
void client_exit(int index, int group);

int main(int argc, char** argv)
{
    int listenfd, sockfd, datafd;
    struct sockaddr_in servaddr;
    
    if (argc != 2) {
        fprintf(stderr,"Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd == -1) {
            fputs("Cannot create socket.\n", stderr);
            exit(2);
    }
    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
            fputs("Cannot bind.\n", stderr);
            exit(2);
    }
    if(listen(listenfd, LISTENQ) == -1) {
            fputs("Cannot listen.\n", stderr);
            exit(2);
    }
    
    int val = fcntl(listenfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, val | O_NONBLOCK);    
    
    struct stat fileStat;
    int maxfd = listenfd +1, nready=0; 
    ssize_t n, nwritten, i,j,k;
    fd_set rset, wset, allset;
    FD_ZERO(&allset); FD_ZERO(&rset); FD_ZERO(&wset);
    FD_SET(listenfd, &allset);

    int namemax=-1;
    NameGroup_init();
    clients_init();

    char *tok=NULL;

    for( ; ;)
    {
        rset = allset;
        FD_ZERO(&wset);
        for(i=0 ; i<MAXCLIENT ; i++) {
            if(check(i)) FD_SET(clients[i].fd, &wset);
        }

        nready = select(maxfd, &rset, &wset, NULL, NULL);
        
       if(FD_ISSET(listenfd, &rset))
       {
            struct sockaddr_in client_addr;
            socklen_t sock_len = sizeof(client_addr);
            int conn_fd = accept(listenfd, (struct sockaddr *)&client_addr, &sock_len);
            
            val = fcntl(conn_fd, F_GETFL, 0);
            fcntl(conn_fd, F_SETFL, val | O_NONBLOCK);
            
            for(i=0; i < FD_SETSIZE; i++)
                if(clients[i].fd < 0) {clients[i].fd = conn_fd; break;}
            if(i == FD_SETSIZE) { fputs("Too many clients!\n", stderr); exit(2); }
            if(conn_fd >= maxfd) { maxfd = conn_fd + 1; }
            FD_SET(conn_fd, &allset);
            --nready;
       }

       for(i=0; i<=MAXCLIENT && nready ; i++)
       {
            if(FD_ISSET(clients[i].fd, &rset))
            {
                if(clients[i].receiving)
                {
                   
                    n = read(clients[i].fd,clients[i].receive, min(MAXLINE,(int)(clients[i].recv_length-clients[i].recv_now)));
                    if(n <0)
                    {
                        if(errno != EWOULDBLOCK)
                        {
                            printf("read error on socket\n");
                            exit(1);
                        }
                    }
                    else if(n == 0)
                    {

                    }
                    else
                    {
                        nwritten = fwrite(clients[i].receive, sizeof(char), n, clients[i].pRecv);
                        clients[i].recv_now += nwritten;
                        if(clients[i].recv_length == clients[i].recv_now)
                        {
                            clients[i].receiving = 0;
                            fclose(clients[i].pRecv);
                            printf("receive sucess\n");
                            AddFile(i, clients[i].group , clients[i].receive_name);
                        }    
                    } 
                }
                else //client send command
                {
                    n = readline(clients[i].fd, clients[i].receive, CMD_MAXLINE);
                    if(n == 0)
                    {
                        printf("someone close the connection\n");
                        FD_CLR(clients[i].fd, &allset);
                        client_exit(i , clients[i].group);
                        continue;
                    }
                    else
                    {
                        printf("%s\n",clients[i].receive);
                        tok = strtok(clients[i].receive, " ");
                        if(strcmp(tok,"username") == 0)
                        {
                            tok = strtok(NULL, " ");
                            NameGroup_add(tok, i);
                            printf("success\n");
                        }
                        else if(strcmp(tok, "put") == 0)
                        {
                            tok = strtok(NULL, " ");
                            //strcpy(clients[i].receive_name,tok);
                            char tmp[30];
                            sprintf(tmp,"%s_%s",clients[i].name,tok);
                            strcpy(clients[i].receive_name,tmp);
                            clients[i].pRecv = fopen(tmp, "w");
                            tok = strtok(NULL, " ");
                            clients[i].recv_length = atoi(tok);  
                            clients[i].recv_now = 0;         
                            clients[i].receiving = 1;
                        }
                    }    
                }
            }
            if(FD_ISSET(clients[i].fd, &wset))
            {
                if(!clients[i].sending)
                {
                    int File = clients[i].send_file = download_file(i);
                    if(File == -1) continue;
                    strcpy(clients[i].send_name, NameGroup[clients[i].group].file_name[File]);
                    clients[i].pSend = fopen(clients[i].send_name, "r");
                    clients[i].SendFront = clients[i].SendBack = clients[i].send;
                    clients[i].read_end = 0;
                    if(stat(clients[i].send_name,&fileStat) < 0) {printf("file error\n"); exit(1);}
                    clients[i].send_length = fileStat.st_size;
                    int tmp = strlen(clients[i].name)+1;
                    sprintf(clients[i].send,"get %s %ld\n",clients[i].send_name+tmp,clients[i].send_length);
                    clients[i].SendBack += strlen(clients[i].send);
                    nwritten = write(clients[i].fd, clients[i].SendFront, strlen(clients[i].send));
                    clients[i].SendFront += nwritten;
                    clients[i].sending = 1;
                }
                if(!clients[i].read_end)
                {
                    n = fread(clients[i].SendBack,1,&clients[i].send[MAXLINE]-clients[i].SendBack,clients[i].pSend);
                    if(n < 0)
                    {
                        printf("read file error\n");
                        exit(0);
                    }
                    else if(n == 0) 
                    { clients[i].read_end = 1;
                        printf("write succes\n");
                    }    
                    else  clients[i].SendBack += n;
                }
                int write_mode = send_write(i,n);
                if(write_mode == -1) exit(1);
                else if(write_mode == 1)  //write file success
                {
                    clients[i].file_cnt++;
                    clients[i].file[clients[i].send_file] = 1;
                    clients[i].send_file = 1;
                    clients[i].sending = 0;
                } 
            }
       }
    }
    exit(0);
}

void NameGroup_entry_init(int index)
{
    int i=0,j=0;
    NameGroup[index].host_cnt = 0;
    NameGroup[index].file_cnt = 0;
    NameGroup[index].used = 0;
    for(j=0 ; j < MAXHOST; j++)
    {
        NameGroup[index].host_index[j] = -1;
        NameGroup[index].progress[j] = -1;
        NameGroup[index].file[j] = 0; 
    }
}

void NameGroup_init()
{
    int i=0,j=0,k=0;
    for( ; i<MAXCLIENT_NAME ; i++)
    {
        NameGroup_entry_init(i);
    }
    return ;
}

void clients_entry_init(int i)
{
    bzero(clients[i].send,MAXLINE);
    bzero(clients[i].receive, MAXLINE);
    bzero(clients[i].send_name,30);
    bzero(clients[i].receive_name,30);
    bzero(clients[i].name,20);
    clients[i].SendFront = clients[i].SendBack = clients[i].send;
    clients[i].RecvFront = clients[i].RecvBack = clients[i].receive;
    clients[i].sending = clients[i].receiving = clients[i].file_cnt= clients[i].read_end =0;
    clients[i].send_file = -1;
    clients[i].fd = clients[i].order = clients[i].group = -1;
    clients[i].recv_length = clients[i].recv_now = clients[i].send_length = clients[i].send_now = 0;
    int j=0;
    for( ; j<MAXFILE ; j++)  clients[i].file[j] = 0;
    return ;
}
void clients_init()
{
    int i, j;
    for(i=0 ; i<MAXCLIENT ; i++)
    {
        clients_entry_init(i);
    }
    return ;
}

int NameGroup_add(char * username, int index)
{
    int i=0,j=0,find=0;
    //printf("%s %s\n",NameGroup[i].name,username);
    for( ; i < MAXCLIENT_NAME ; i++)
    {
        if(!NameGroup[i].used)
        {   
            strcpy(NameGroup[i].name, username);
            NameGroup[i].used = 1; find = 1; break;
        }
        else if(strcmp(NameGroup[i].name,username)==0)
        {
            find = 1; break;
        }
    }
    if(find)
    {
        NameGroup[i].host_cnt++;
        for(j=0 ; j < MAXHOST; j++)
        {
            if(NameGroup[i].host_index[j] == -1)
            {
                NameGroup[i].host_index[j] = index;
                break;
            }
        }
        clients[index].group = i;
        clients[index].order = j;
        strcpy(clients[index].name, username);
        return 1;
    }
    return -1;
}

void AddFile(int index, int group, char * filename)
{
    int count = NameGroup[group].file_cnt;
    clients[index].file[count] = 1;
    clients[index].file_cnt++;
    NameGroup[group].file[count] = 1;
    strcpy(NameGroup[group].file_name[count], filename); //insert file name, file_cnt+1
    NameGroup[group].file_cnt++;
    //printf("group file count: %d\n",NameGroup[group].file_cnt);
    //printf("file name: %s\n",NameGroup[group].file_name[count]);
    //int i=0;
   // printf("=====================\n");
    //for(; i<NameGroup[group].file_cnt; i++)
     //   printf("name: %s\n",NameGroup[group].file_name[i]);
    //printf("=====================\n");
    return ;
}


int check(int index)
{
    int group = clients[index].group;
    if(NameGroup[group].file_cnt != clients[index].file_cnt) return 1;
    else return 0;
}

int download_file(int index)
{
    int i=0, group = clients[index].group;
    for( ; i< MAXFILE ; i++)
    {
        if(clients[index].file[i] != NameGroup[group].file[i]) 
        {
            //printf("write [%s]\n",NameGroup[group].file_name[i]);
            return i;
        }    
    }
    return -1;
}

int send_write(int index,int n)
{
    int nwritten;
    nwritten = write(clients[index].fd, clients[index].SendFront, clients[index].SendBack-clients[index].SendFront);
    if(nwritten < 0)
    {
        if(errno != EWOULDBLOCK)
        {
            printf("write error to socket\n");
            return -1;
        }
    }
    else
    {
        clients[index].recv_now += nwritten;
        clients[index].SendFront+= nwritten;
        if(clients[index].SendFront == clients[index].SendBack) {
            clients[index].SendFront=clients[index].SendBack=clients[index].send;
            if(clients[index].read_end) return 1; 
        }
    }
    return 0;       
}
                       
void client_exit(int index, int group)
{
    int i;
    for(i=0; i<MAXHOST ; i++)
    {
        if(NameGroup[group].host_index[i] == index)
        {
            NameGroup[group].host_index[i] = -1;
            NameGroup[group].host_cnt--;
            clients_entry_init(index);
            return ;
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