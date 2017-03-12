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

#define PORT 7000
#define CMD_MAX 1024
#define BUFF_MAX 1024
#define LISTENQ 1024

int calc(int conn_fd);
ssize_t readline(int fd, void * vptr, size_t maxlen);

int main(int argc, char *argv[]) {
		/* This line can prevent for creating zombie process */
		signal(SIGCHLD, SIG_IGN);

		int listen_fd;

		struct sockaddr_in listen_addr;
		memset(&listen_addr, 0, sizeof(listen_addr));
		listen_addr.sin_family = AF_INET;
		listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		listen_addr.sin_port = htons(PORT);

		listen_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(listen_fd == -1) {
				fputs("Cannot create socket.\n", stderr);
				exit(2);
		}

		if(bind(listen_fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == -1) {
				fputs("Cannot bind.\n", stderr);
				exit(2);
		}

		if(listen(listen_fd, LISTENQ) == -1) {
				fputs("Cannot listen.\n", stderr);
				exit(2);
		}

		while(1) {
				struct sockaddr_in client_addr;
				int sock_len = sizeof(client_addr);
				int conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &sock_len);

				if(fork() == 0) {
						close(listen_fd);
						int n,a,b=0; char dir[5]="0"; char command[20];
						while(a = mkdir(dir,0777)) { sprintf(dir, "%d", ++b);}
						chdir(dir);
						calc(conn_fd);
						close(conn_fd);
						chdir(".."); 
						sprintf(command,"rm -rf %s",dir); system(command);
						puts("Client has closed the connection.");
						fflush(stdout);
						exit(0);
				}
				else
				{ 
					close(conn_fd);
				}
		}
}

int calc(int conn_fd) 
{
	while(1)
	{	
		char cmd[CMD_MAX + 1];
		bzero(cmd,CMD_MAX + 1);
		int n;
		n = read(conn_fd, cmd, CMD_MAX);
		if(n == 0) { return 0;}
		cmd[n] = '\0';
		char *opcode = strtok(cmd, " \n");
		if(strcmp(opcode, "EXIT") == 0) { return 0;}
		char buff [BUFF_MAX+1]; bzero(buff,BUFF_MAX+1);
		if(strcmp(opcode, "PUT") == 0) 
		{
			char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) continue; 
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) continue;
			FILE* pFile = fopen(name2, "w");
			char length [20]; bzero(length,20);
			n = readline(conn_fd, length, 20);
	        long length_n=atoi(length);
	        long result = 0;
            while( result < length_n )
            {
            	n = read(conn_fd,buff,BUFF_MAX);
                if(n < 0){ printf("server read error\n");break;}
                else write(fileno(pFile),buff,n);
                result += n;
            }
			fclose(pFile);
		} 
		else if(strcmp(opcode, "GET") == 0) 
		{
			FILE *pFile;  size_t result;
        	char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) continue;
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) continue; 
 			pFile = fopen(name1, "r"); 
 			if(pFile == NULL)
 			{
 				printf("file doesn't exist\n"); 
 				char end[20] = "file doesn't exist\n";
            	write(conn_fd, end, strlen(end));
            	continue;
 			} 
		    fseek(pFile,0,SEEK_END);
			long lSize = ftell(pFile);
   			rewind(pFile);
		    char length [20]; bzero(length,20);
   			sprintf(length,"%ld\n",lSize);
		    write(conn_fd, length, strlen(length));
		    long read_cnt = 0;
		    fclose(pFile);  pFile = fopen(name1, "r");
		    while(n = read(fileno(pFile),buff,BUFF_MAX))
	    	{
                if(n < 0) {printf("server read wrong\n"); break;}
                else 
                { 
                    n = write(conn_fd, buff, n);
                    if(n < 0) { printf("server write error\n"); break;}
                }     
            }       
    		fclose(pFile);
		}
		else if(strcmp(opcode, "LIST") == 0) 
		{
			system("ls > zzzz.txt"); 
			FILE *pFile = fopen("zzzz.txt", "r");
			while(n = readline(fileno(pFile),buff,BUFF_MAX))
	    	{
                if(n < 0) {printf("server read wrong\n"); break;}
                else 
                { 
                    n = write(conn_fd, buff, n);
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
