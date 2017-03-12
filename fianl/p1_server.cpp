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
#define MAX_BUF_SIZE 2048
#define ALARM_TIME 3 
sockaddr_in addr_remote,addr_self;
socklen_t addr_len;
int fd;
int sum;
bool init;
int count;
int timeout = 0;
void reset();
void timedout(int signo);
void main_loop();
void bufclear(char* buf);
void processNumber(int n); 

int main(int argc, char** argv)
{
	if(argc < 2){
		printf("Usage : server [port]\n");
		return 1;
	}
	signal(SIGALRM, timedout);
	siginterrupt(SIGALRM,1);
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	addr_self.sin_family = AF_INET;
	addr_self.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_self.sin_port= htons(atoi(argv[1]));
	bind(fd, (sockaddr*)&addr_self, sizeof(addr_self));
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(tv));
	main_loop();
	close(fd);
}

void timedout(int signo){
	cout << "Time out" << endl;
	reset();
}

void reset(){
	cout <<"Reset "<<endl;
	init = false;
	int count =0;
	timeout = 0;
	alarm(0);
}
void bufclear(char* buf){
	bzero(buf,MAX_BUF_SIZE);
}
void processNumber(int n){
	if(!init){
		init = true;
		//alarm(ALARM_TIME);
		sum=0;
		count =0;
	}
	++count;
	sum += n;
	timeout = 1;
	
	if(count >=2){
		char buf[MAX_BUF_SIZE];
		bufclear(buf);
		snprintf(buf,MAX_BUF_SIZE,"%d",sum);
		cout << "return sum: "<<sum << endl;
		sendto(fd,buf,strlen(buf),0,(sockaddr*)&addr_remote, sizeof(addr_remote));
		reset();
	}
}

void main_loop(){
	reset();
	char buf[MAX_BUF_SIZE];
	int read_n;
	while(true){
		bufclear(buf);
		addr_len = sizeof(addr_remote);
		read_n = recvfrom(fd, buf,MAX_BUF_SIZE,0,(sockaddr*)&addr_remote,&addr_len);
		if(read_n<0){
			if(errno == EINTR ){
				//interrupt
			}
			else if(errno==EAGAIN || errno==EWOULDBLOCK){
				if(timeout) {
					printf("timeout\n");
					reset();
				}
				timeout = 0;
				//printf("hello timeout\n");
			}
			else{
				timeout =0;
				cout << "Error occur, no = " << errno << endl;
			}
			continue;
		}
		else if(read_n == 0)
		{
			close(fd);
			printf("server get\n");
		}
		printf("recv: %s\n",buf);
		processNumber(atoi(buf));
	}
}