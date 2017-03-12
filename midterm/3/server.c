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

int calc(char *cmd, int conn_fd) ;
ssize_t readline(int fd, void * vptr, size_t maxlen);

int main(int argc, char *argv[]) {

		int listen_fd;
		int max_fd;
		int clients[FD_SETSIZE];
		int clientid[FD_SETSIZE];
		int client_maxi;

		/* Set all of clients to -1 */
		memset(clients, 0xFF, sizeof(clients));
		memset(clientid, 0xFF, sizeof(clientid));
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

		fd_set all_set, r_set;
		FD_ZERO(&all_set);
		FD_SET(listen_fd, &all_set);
		max_fd = listen_fd + 1;
		char command[20];
		char dir[5]="0";
		
		while(1) 
		{
			r_set = all_set;
			int nready = select(max_fd, &r_set, NULL, NULL, NULL);

			if(FD_ISSET(listen_fd, &r_set)) 
			{
				struct sockaddr_in client_addr;
				int sock_len = sizeof(client_addr);
				int conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &sock_len);

				int i,a,b=0;
				for(i=0; i<FD_SETSIZE; i++) 
					if(clients[i] < 0)  { clients[i] = conn_fd; break; }

				if(i == FD_SETSIZE) { fputs("Too many clients!\n", stderr); exit(2); }
				if(i > client_maxi) { client_maxi = i;}
				if(conn_fd >= max_fd) { max_fd = conn_fd + 1; }
				FD_SET(conn_fd, &all_set);
				--nready;
				while(a = mkdir(dir,0777)) { sprintf(dir, "%d", ++b);}
				clientid[i] = b; 
				//sprintf(command,"mkdir %d",i); system(command); //use client index to create dir
			}

			int i;
			for(i=0; i<=client_maxi && nready; i++) 
			{
				if(FD_ISSET(clients[i], &r_set)) 
				{
					--nready;
					char cmd[CMD_MAX + 1];
					int n;
					//sprintf(command,"%d",i);
					sprintf(dir, "%d", clientid[i]); 
					chdir(dir);
					n = read(clients[i], cmd, CMD_MAX);
					cmd[n] = '\0';
					if(n == 0 || !calc(cmd,clients[i])) 
					{
						/* Client closed itself or */
						/* Client has entered EXIT */
						close(clients[i]);
						FD_CLR(clients[i], &all_set);
						chdir(".."); 
						sprintf(command,"rm -rf %d",clientid[i]); system(command);
						clients[i] = -1; clientid[i]=-1;
					}
					else {chdir(".."); } 
				}
			}
		}
}


int calc(char *cmd, int conn_fd) 
{
		char *opcode = strtok(cmd, " \n");
		if(strcmp(opcode, "EXIT") == 0) { return 0;}
		int n;
		char buff [BUFF_MAX+1]; bzero(buff,BUFF_MAX+1);
		if(strcmp(opcode, "PUT") == 0) 
		{
			char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) return 1; 
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) return 1; 
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
        	char *name1 = strtok(NULL, " \n"); if(name1 == NULL ) return 1; 
			char *name2 = strtok(NULL, " \n"); if(name2 == NULL ) return 1; 
 			pFile = fopen(name1, "r"); 
 			if(pFile == NULL)
 			{
 				printf("file doesn't exist\n"); 
 				char end[20] = "file doesn't exist\n";
            	write(conn_fd, end, strlen(end));
            	return 1; 
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

		return 1;
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
