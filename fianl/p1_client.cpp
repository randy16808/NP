#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
using namespace std;
#define MAX_BUF_SIZE 1024
#define ALARM_TIME 3 

sockaddr_in addr_remote;
socklen_t addr_len;
int fd;
int stdineof=0;
void* receive(void*);
void main_loop();
void bufclear(char* buf);
void processNumber(int n); 
ssize_t readline(int fd, void *vptr, size_t maxlen);

void sig_child(int signo){
		printf("terminated\n");
		return;
}

int main(int argc, char** argv)
{
	if(argc < 3){
		printf("Usage : server [port]\n");
		return 1;
	}
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	addr_remote.sin_family = AF_INET;
	addr_remote.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1],&addr_remote.sin_addr);
	addr_len = sizeof(addr_remote);
	
	pid_t pid;
	char sendline[MAX_BUF_SIZE], recvline[MAX_BUF_SIZE];
	char input[MAX_BUF_SIZE];
	if((pid=fork())==0){ //child
		char recv[MAX_BUF_SIZE];
		int read_n;
		while(true){
			bufclear(recv);
			addr_len = sizeof(addr_remote);
			read_n = recvfrom(fd, recv, MAX_BUF_SIZE, 0,(sockaddr*)&addr_remote, &addr_len );
			if(read_n <0){
				cout << "recv error" << endl;
				continue;
			}
			else if(read_n ==0){
				printf("close \n");
				kill(getppid(), SIGTERM);
				exit(0);
			}
			else {
				printf("recv %s\n",recv);
			}
		}		
	}
	else{ //parent
		int num;
		char buf[MAX_BUF_SIZE];
		while(cin >> num){
			bufclear(buf);
			snprintf(buf, MAX_BUF_SIZE,"%d",num);
			sendto(fd, buf, strlen(buf),0,(sockaddr*)&addr_remote,addr_len);
			printf("send: %d\n",num);
		}
		char com[1024];
		sprintf(com,"kill -9 %d",pid);
		system(com);
	//	bufclear(buf);
	//	snprintf(buf, MAX_BUF_SIZE,"0");
	//	sendto(fd, buf, strlen(buf),0,(sockaddr*)&addr_remote,addr_len);
		close(fd);
		return 0;
	}
	
	return 0;
}

void bufclear(char* buf){
	bzero(buf,MAX_BUF_SIZE);
}

ssize_t readline(int fd, void *vptr, size_t maxlen){
	ssize_t n, rc;
	char c, *ptr;
	ptr = (char*) vptr;
	for(n=1; n<maxlen ; n++){
		if((rc=read(fd, &c, 1)==1)){
			*ptr++ = c;
			if(c == '\n') break;
		}
		else if(rc == 0){
			*ptr =0;
			return (n-1);
		}
		else return -1;
	}
	*ptr =0;
	return n;
}