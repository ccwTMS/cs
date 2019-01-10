/*
 * This program is free software; you can redistribute it and/or modify  * 
 * it under the terms of the GNU General Public License as published by  * 
 * the Free Software Foundation; either version 2 of the License, or     * 
 * (at your option) any later version.                                   * 
 *                                                                       * 
 * This program is distributed in the hope that it will be useful,       * 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * 
 * GNU General Public License for more details.                          * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
//#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <getopt.h>	
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

int quit=0;
#define OPTCHARS	"bs:i:p:w:f:uv"
extern char *optarg; //a argument is followed by option.
extern int optind, opterr, optopt; //last argv-element, errno meg, return optionif error is occured.

char *port="1234";

int sock_type=SOCK_STREAM;
struct sockaddr_in6 udp_sock;

void usage(void)
{
	printf("-p port_number	: \n");
	printf("-i ip           : \n");
	printf("-s package_size : \n");
	printf("-v              : \n");
	printf("-w write_path   : \n");
	printf("-f write_path   : write file test only.\n");
	printf("-u		: udp protocol\n");
	printf("-b		: broadcast in IPv6\n");
}

void end_handler(int sig)
{
	if(sig == SIGINT)
		quit=1;
}

extern int h_errno;

int network_init( char * arg )
{	
	int sock;
	struct addrinfo addr, *raddr, *raddrHead;
	int ret;

	memset(&addr, 0, sizeof(struct addrinfo));

	addr.ai_family = AF_UNSPEC;
	addr.ai_socktype = sock_type;

	ret = getaddrinfo( arg, port, &addr, &raddrHead );
	if(ret < 0){
		printf("getaddrinfo error: [%s]\n", gai_strerror(ret));
		return -1;
	}

	raddr = raddrHead;
	{
	while(raddr){
		printf("ai_family:%d, ai_socktype:%d, ai_protocol:%d, ai_addrlen:%d\n",
			raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol, raddr->ai_addrlen);
		printf("in:%d, in6:%d\n", sizeof(struct sockaddr_in), sizeof(struct sockaddr_in6));
#if 1
		switch(raddr->ai_family){
			case PF_INET:{
				struct sockaddr_in *pIP4=(struct sockaddr_in *)raddr->ai_addr;				
				char IPBuf[NI_MAXHOST];
				struct in_addr ipv4_addr;
				printf("IPBn:[%x]\n", pIP4->sin_addr.s_addr);

				inet_ntop(raddr->ai_family, &pIP4->sin_addr, IPBuf, raddr->ai_addrlen); 
				printf("IPBp:[%s]\n", IPBuf);
				
				inet_pton(raddr->ai_family, IPBuf, &ipv4_addr);
				
				printf("IPBn:[%x]\n", ipv4_addr.s_addr);
				
				break;}
			case PF_INET6:{
				struct sockaddr_in6 *pIP6=(struct sockaddr_in6 *)raddr->ai_addr;
				char IPBuf[NI_MAXHOST];
				struct in6_addr ipv6_addr;
				int i;

				printf("IPBn:[");
				for(i=0;i<16;i++){
					printf("%02x", pIP6->sin6_addr.s6_addr[i]);
					if(i%2==1 && i!=15)
						printf(":");
				}
				printf("]\n");


				inet_ntop(raddr->ai_family, &pIP6->sin6_addr, IPBuf, raddr->ai_addrlen); 
				printf("IPBp:[%s]\n", IPBuf);
				
				inet_pton(raddr->ai_family, IPBuf, &ipv6_addr);
				
				printf("IPBn:[");
				for(i=0;i<16;i++){
					printf("%02x", ipv6_addr.s6_addr[i]);
					if(i%2==1 && i!=15)
						printf(":");
				}
				printf("]\n");


				break;}
				
		}
#endif
		char domainBuf[NI_MAXHOST];
		char portBuf[NI_MAXSERV];
		getnameinfo(raddr->ai_addr, raddr->ai_addrlen, domainBuf, sizeof(domainBuf), portBuf, sizeof(portBuf), NI_NUMERICHOST|NI_NUMERICSERV);
		printf("domain:[%s], port:[%s]\n", domainBuf, portBuf);
	
		raddr = raddr->ai_next;
	}
	}
	raddr = raddrHead;
	

	sock = socket( raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol );
	if( sock < 0 ){
		printf("Unable to create a socket.\n" );
		return -3;	
	}

	if(raddr->ai_socktype == SOCK_STREAM){
		if( connect( sock, raddr->ai_addr, raddr->ai_addrlen ) < 0 ){
			close(sock);
			return -2;
		}
	}else
	if(raddr->ai_socktype == SOCK_DGRAM){
		memcpy(&udp_sock, raddr->ai_addr, sizeof(struct sockaddr_in6));
	}
#if 1
	{
	int i;
	struct ifreq ifr;

	ifr.ifr_addr.sa_family=raddr->ai_family;

	gv_get_ifname2(&sock, &ifr);
	gv_get_macaddr(&sock, &ifr);
	printf("MAC[");
	for(i=0;i<6;i++){
		printf("%x", ifr.ifr_addr.sa_data[i]);
		if(i<6)
			printf(":");
	}
	printf("]\n");

	gv_get_ifaddr_v2();

	}
#endif

	freeaddrinfo(raddrHead);

	printf("Connected!\n");

	return sock;

}

int gv_get_ifname( int *fd, struct ifreq *ifr )
{
	ifr->ifr_ifindex = 2;
	if( ioctl(*fd, SIOCGIFNAME, ifr) != 0 ){
		perror("get ifname");
		return -1;
	}

	return 0;
}

int gv_get_ifname2( int *fd, struct ifreq *ifr )
{
	struct ifreq *ifrp;
	struct ifconf ifc;
	int i;

		ifc.ifc_len = (sizeof(struct ifreq)*8);
		ifc.ifc_buf = (char *)malloc(ifc.ifc_len);

		if( ioctl( *fd, SIOCGIFCONF, &ifc ) < 0){
			perror("FCONF error");
			free(ifc.ifc_buf);
			return -1;
		}
		ifrp = ifc.ifc_req;
	
	for(i=0;i<ifc.ifc_len;i+=sizeof(struct ifreq)){
		//dprintf("name[%d]=%s\n",i,ifrp->ifr_name);

		if( ioctl(*fd, SIOCGIFFLAGS, ifrp) < 0){
			perror("get flag error");
		}

		if(ifrp->ifr_flags & IFF_UP && strcmp( ifrp->ifr_name, "lo")){
			//dprintf("%s is up.\n",ifrp->ifr_name);
			strcpy(ifr->ifr_name, ifrp->ifr_name);
			//TMS: disabled for remote-update failed by wireless.
			//free(ifc.ifc_buf);
			//return 0;
		}
		
		//free(ifc.ifc_ifcu.ifcu_buf);
		ifrp++;
	}
	free(ifc.ifc_buf);
	return -2;
}

int gv_get_macaddr( int *fd, struct ifreq *ifr )
{
	int i;
	if( ioctl(*fd, SIOCGIFHWADDR, ifr) != 0){	
		perror("hw addr");
		return -1;
	}
	return 0;
}

//TMS: for IPv4 only
int gv_get_ifaddr( int *fd, struct ifreq *ifr )
{
	if( ioctl(*fd, SIOCGIFADDR, ifr) != 0){	
		perror("addr");
		return -1;
	}
	return 0;
}


int gv_get_ifaddr_v2( void )
{
	struct ifaddrs *ifa=NULL, *pifr=NULL;
	int i=0;
	int ret=0;

	ret = getifaddrs(&ifa);
	for(pifr=ifa; pifr!=NULL;pifr=pifr->ifa_next){
		if(pifr->ifa_addr->sa_family== PF_INET || pifr->ifa_addr->sa_family == PF_INET6){
			printf("ifa_name:%s, ifa_flags:%d, sa_family:%d\n", pifr->ifa_name, pifr->ifa_flags, pifr->ifa_addr->sa_family);
			switch(pifr->ifa_addr->sa_family){
				case PF_INET:{
					     struct sockaddr_in *pIPv4=(struct sockaddr_in *)pifr->ifa_addr;
					     printf("Ifrn:[%x]\n", pIPv4->sin_addr.s_addr);
					     break;}
				case PF_INET6:{
					      struct sockaddr_in6 *pIPv6=(struct sockaddr_in6 *)pifr->ifa_addr;
					      printf("Ifrn:[");
					      for(i=0;i<16;i++){
						      printf("%02x", pIPv6->sin6_addr.s6_addr[i]);
						      if(i%2==1 && i!=15)
							      printf(":");
					      }
					      printf("]\n");

					      break;}
			}
		}

	}
}

int main( int argc, char *argv[] )
{
	int ret=0;
	int imgfd;
	int sock;
	int i;
	int send_all_at_once = 0;
	unsigned long wrlen;
	unsigned long total_len=0;
	unsigned long imglen;
	time_t t_start, t_end;
	char *imgbuf=NULL;
	char *imgdata=NULL;
	ssize_t ret_recv;
	struct timeval now_time;
	time_t send_old_sec;
	int send_cnt=1;
	char *buf=NULL;
	unsigned int count=0;
	unsigned int t_pkgs=0;

	int op;
	unsigned long data_len=16384;
	char *domain=NULL;
	char *write_file_path=NULL;
	char *write_file_test=NULL;
	int fd=-1;
	pid_t mypid;
	socklen_t recvfromlen=0;

	signal(SIGINT, end_handler);

	if(argc == 1){
		usage();
		return -6;
	}

	while( (op = getopt(argc, argv, OPTCHARS)) != EOF ){
		switch(op){
			case 's': 
				data_len = strtol((const char *)optarg, NULL, 10);
				break;
			case 'p':
				port = optarg;
				break;
			case 'i':
				domain = optarg;
				break;
			case 'w': 
				write_file_path = optarg;
				break;
			case 'f': 
				write_file_test = optarg;
				break;
			case 'u':
				sock_type=SOCK_DGRAM;
				break;
			case 'b':
				domain="ff02::1";
				break;
			default:
				usage();
				return -1;
		}
	}

	mypid = getpid();


	buf = (char *)malloc(data_len);
	if(buf == NULL){
		printf("malloc failed!!, size=%d.\n",data_len);
		return -2;
	}
		
	if( write_file_test != NULL){
		memset(buf, 0x5a, data_len);
		fd = open(write_file_test, O_CREAT|O_RDWR);
		if (fd < 0){
			printf("file open failed.\n");
			ret = -4;
			goto _err_end;
		}
		do{
			write(fd, buf, data_len);
			lseek(fd, 0, SEEK_SET);
			
			gettimeofday(&now_time,NULL);
			if(send_old_sec == now_time.tv_sec){
				send_cnt++;
			}else{
				count+=1;
				t_pkgs+=send_cnt;
				//printf("[pid:%d](%u:%u:)    in_cnt:%d(%u kbps)\n",mypid, count,t_pkgs, send_cnt, (data_len/1024)*8*send_cnt);
				printf("[pid:%d](t:%u)    w_cnt:%d(%u kbps)\n",mypid, count, send_cnt, (data_len/1024)*8*send_cnt);
				send_old_sec=now_time.tv_sec;
				send_cnt=1;
			}

		}while(!quit);
		close(fd);
		free(buf);
		return 0;
	}

	sock = network_init( domain );
	if( sock < 0 ){
		ret = -3;
		goto _err_end;	
	}

	if(write_file_path != NULL){
		fd = open(write_file_path, O_CREAT|O_RDWR);
		if (fd < 0){
			printf("file open failed.\n");
			ret = -4;
			goto _err_end1;
		}
	}

	recvfromlen=sizeof(struct sockaddr_in6);

	if(sock_type == SOCK_DGRAM){
		printf("CC: client start...\n");
		ret_recv = sendto( sock, buf, data_len, 0, (struct sockaddr *)&udp_sock, (socklen_t)recvfromlen );
	}
	while(!quit){
		if(sock_type == SOCK_DGRAM){
			//printf("TMS: udp recvfrom start\n");
			ret_recv = recvfrom( sock, buf, data_len, MSG_WAITALL, (struct sockaddr *)&udp_sock, &recvfromlen );
			//printf("ret_recv:%d, [%s]\n", ret_recv, strerror(errno));
		}else
		if(sock_type == SOCK_STREAM)
			ret_recv = recvfrom( sock, buf, data_len, MSG_WAITALL, NULL, 0);

		if(ret_recv != data_len){
			printf("receives wrong data len(%s)\n", strerror(errno));
			goto _err_end1;
		}if(fd >=0){
			write(fd, buf, data_len);
		}

		gettimeofday(&now_time,NULL);
		if(send_old_sec == now_time.tv_sec){
			send_cnt++;
		}else{
			count+=1;
			t_pkgs+=send_cnt;
			//printf("[pid:%d](%u:%u:)    in_cnt:%d(%u kbps)\n",mypid, count,t_pkgs, send_cnt, (data_len/1024)*8*send_cnt);
			printf("[pid:%d](t:%u)    in_cnt:%d(%u kbps)\n",mypid, count, send_cnt, (data_len/1024)*8*send_cnt);
			send_old_sec=now_time.tv_sec;
			send_cnt=1;
		}

	}		

	ret = 0;

	close(fd);
_err_end1:
	close(sock);
_err_end:
	if(buf != NULL)
		free(buf);

	printf("ret:%d\n", ret);
	return ret;
}	
