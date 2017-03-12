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

#define LISTENQ 1024
#define MAXLINE 4096
#define BUFF_MAX 1024
#define CMD_MAX 100
#define PORT 7000

ssize_t readline(int fd, void *vptr, size_t maxlen);
void calc(int fd);

int main(int argc, char * argv[])
{
	int listen_fd;
		
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(PORT);
	char cmd [CMD_MAX]; int n=0, i=0;
    char buff[BUFF_MAX];
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
   	{
	    fputs("Fail to create socket.\n",stderr);
    	    exit(2);
    	}
    	if(bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1)
    	{
	    fputs("Fail to bind.\n",stderr);
            exit(2);
    	}
	if(listen(listen_fd, LISTENQ) == -1)
	{
	    fputs("Cannot listen.\n",stderr);
	    exit(2);
	}	

    while(1) 
    {
		struct sockaddr_in client_addr;
		int sock_len = sizeof(client_addr);
		int conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &sock_len);
		mkdir("first",0777); chdir("first");
		calc(conn_fd);
		chdir(".."); char command[20];
		sprintf(command,"rm -rf first"); system(command);
	}
}

void calc(int fd) {
	char cmd [BUFF_MAX+1]; 
    char buff [BUFF_MAX+1]; 
	int n,i=0,a,conn_fd = fd;
	char command[20];  

	while(1) 
	{
		bzero(cmd,CMD_MAX+1);
		bzero(buff,BUFF_MAX+1);
		n = read(conn_fd, cmd, CMD_MAX);
		if(n == 0) { close(fd); return; }
		cmd[n] = '\0';

		char *opcode = strtok(cmd, " \n");
		if(strcmp(opcode, "EXIT") == 0) 
		{ 
			close(fd);
			return;
		}
		else if(strcmp(opcode, "PUT") == 0) 
		{
			char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) continue ; 
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) continue ; 
			FILE* pFile = fopen(name2, "w");
			char length [20]; bzero(length,20);
			n = readline(fd, length, 20);
	        long length_n=atoi(length);
	        long result = 0, read_cnt = 0;
            while( result < length_n )
            {
            	read_cnt= read(conn_fd,buff,BUFF_MAX);
                if(read_cnt < 0){ printf("server read error\n");break;}
                else write(fileno(pFile),buff,read_cnt );
                result += read_cnt;
            }
			fclose(pFile);
		} 
		else if(strcmp(opcode, "GET") == 0) 
		{

			FILE *pFile;  size_t result;
        	char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) continue ; 
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) continue ; 
 			pFile = fopen(name1, "r"); 
 			if(pFile == NULL) 
 			{
 				printf("file doesn't exist\n"); 
 				char end[20] = "file doesn't exist\n";
            	write(conn_fd, end, strlen(end));   
 				continue ; 
 			} 
		    fseek(pFile,0,SEEK_END);
			long lSize = ftell(pFile);
   			rewind(pFile);
		    char tmp[20];
			for(i=0; i<20; i++) tmp[i] = 0;
   			sprintf(tmp,"%ld\n",lSize);
		    write(conn_fd, tmp, strlen(tmp));
		    long read_cnt = 0;
		    fclose(pFile);  pFile = fopen(name1, "r");
		    while(read_cnt = read(fileno(pFile),buff,BUFF_MAX))
	    	{
                if(read_cnt < 0) {printf("server read wrong\n"); break;}
                else 
                { 
                    n = write(conn_fd, buff, read_cnt);
                    if(n < 0) { printf("server write error\n"); break;}
                }     
            } 

    		fclose(pFile);
		}
		else if(strcmp(opcode, "LIST") == 0) 
		{
			system("ls > zzzz.txt");
			FILE *pFile = fopen("zzzz.txt", "r");
			int read_cnt=0 ;
			while(read_cnt = readline(fileno(pFile),buff,BUFF_MAX))
	    	{
                if(read_cnt < 0) {printf("server read wrong\n"); break;}
                else 
                { 
                    n = write(conn_fd, buff, read_cnt);
                    if(n < 0) { printf("server write error\n"); break;}
                }
            }
            char end[12] = "Finish List\n";
            write(conn_fd, end, strlen(end));       
			fclose(pFile);
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
