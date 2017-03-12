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
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/ip_icmp.h>
using namespace std;
//#define IFNAMSIZ 16
#define ALARM_TIME 3 
/*
struct ini_info* get_ifi_info(int family, int doaliases){
	struct ifi_info* ifi, *ifihead, **ifipnext;
	int sockfd,len,lastlen,myflags, idx=0, hlen=0;
	char * ptr, *buf, lastname[IFNAMSIZ], *cptr, *haddr, *sdlname;
	struct ifconf ifc;
	struct ifreq* ifr, ifrcopy;
	struct sockaddr_in * sinptr;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	lastlen =0;
	len = 100 * sizeof(struct ifreq);
	for( ; ;){
		buf = (char*)malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if(ioctl(sockfd ,SIOCGIFCONF, &ifc)<0){
			if(errno != EINVAL || lastlen !=0){
				printf("error\n");
				exit(1);
			}
		}
		else {
			if(ifc.ifc_len == lastlen) break;
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);
		free(buf);
	}
	ifihead = NULL;
	ifipnext = &ifihead;
	lastname[0] = 0;
	sdlname = NULL;
	for(ptr=buf; ptr<buf+ifc.ifc_len;){
		ifr =(struct ifreq*) ptr;
		
		if(ifr->ifr_addr.sa_family != family) continue;
		myflags =0;
		if((cptr=strchr(ifr->ifr_name, ':'))  != NULL) *cptr =0;
		if(strncmp(lastname, ifr->ifr_name, IFNAMSIZ )==0){
			if(doaliases==0) continue;
			//myflags = IFI_ALIAS;
		}
		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);
		ifrcopy = *ifr;
		ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		flags = ifrcopy.ifr_flags;
		//if((flags & IFF_UP) == 0 )
		ioctl(sockfd, SIOCGIFMTU, &ifrcopy);
		ifi->ifi_mtu = ifrcopy.ifr_mtu;
		
	}
	return ;
}
*/
int main(int argc, char** argv)
{
		
	//struct ifi_info* ifi, *ifihead;
	struct ifconf ifc;
	struct ifreq* ifr, ifrcopy;
	struct sockaddr* sa;
	u_char *ptr;
	int i, family, doaliases;
	//if(argc < 3){
	//	printf("Usage : server [port]\n");
	//	return 1;
	//}
	doaliases = 2;
	family=AF_INET;
	int sockfd =socket(AF_INET, SOCK_DGRAM, 0);
	ioctl(sockfd ,SIOCGIFCONF, &ifrcopy);
	char lastname[16];
	sprintf(lastname,"lo");
	memcpy(ifrcopy.ifr_name,lastname,16);
	ioctl(sockfd, SIOCGIFMTU, &ifrcopy);
	printf(" MTU: %d\n", ifrcopy.ifr_mtu);
	ifrcopy.ifr_mtu= atoi(argv[1]);
	ioctl(sockfd, SIOCSIFMTU, &ifrcopy);
	//ioctl(sockfd, SIOCGIFMTU, &ifrcopy);
	printf(" MTU: %d\n", ifrcopy.ifr_mtu);

	/*for(ifihead = ifi= get_ifi_info(family,doaliases); ifi != NULL ; ifi= ifi->ifi_next)
	{
		printf("%s: ",ifi->ifi_name);
		if(ifi->ifi_mtu != 0)
			printf(" MTU: %d\n", ifi->ifi_mtu);
		
	}
	
	free_ifi_info(ifihead);
*/	exit(0);
	
	return 0;
}
