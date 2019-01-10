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
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <getopt.h>	
#include <signal.h>

//for upr_disable_wdt()
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/watchdog.h>

int quit=0;
int quit_all=0;

#define OPTCHARS	"s:i:p:w:uv"
extern char *optarg; //a argument is followed by option.
extern int optind, opterr, optopt; //last argv-element, errno meg, return optionif error is occured.

int port=1234;


static int client;
static int connect_broken=0;

void usage(void)
{
	printf("-p port_number	: \n");
//	printf("-i ip           : \n");
	printf("-s package_size : \n");
	printf("-u		: udp type\n");
//	printf("-v              : \n");
//	printf("-w write_path   : \n");
}

void end_handler(int sig)
{
	if(sig == SIGINT)
		quit=1;
}


int main( int argc, char *argv[] )
{
	int ret = 0;
	char *buf;
	int sfd;
	unsigned long rdlen;
	char dotted_ip[INET6_ADDRSTRLEN];
	int listener;
	struct sockaddr_in6 sa;
	socklen_t sa_len;
	int crcval;
	int noerror = 1;
	struct timeval now_time;
	time_t send_old_sec;
	int send_cnt=1;
	unsigned int count=0;
	unsigned int t_pkgs=0;

	int op;
	unsigned long data_len=16384;
	char *domain=NULL;
	pid_t mypid;
	int snd_ret=0;
	int sock_type=SOCK_STREAM;

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
				port = strtol((const char *)optarg, NULL, 10);
				break;
			case 'i':
				domain = optarg;
				break;
			case 'u':
				sock_type=SOCK_DGRAM;
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
	memset( buf, 0xF, data_len );
	
	//listener = socket( PF_INET, SOCK_STREAM, IPPROTO_IP );
	listener = socket( PF_INET6, sock_type, 0 );
	if( listener < 0 ){
		printf("Unable to create a listener socket: %s\n", strerror(errno));
		ret = -3;
		goto _err_end;
	}

	sa_len = sizeof(sa);
	memset( &sa, 0, sa_len );
	//sa.sin_family = AF_INET;
	sa.sin6_family = AF_INET6;
	//sa.sin_port = htons(port);
	sa.sin6_port = htons(port);
	//sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin6_addr = in6addr_any;

	if( bind(listener, (struct sockaddr *)&sa, sa_len) < 0 ){
		printf("Unable to bind to port %i: %s\n", port, strerror(errno));
		ret = -4;
		goto _err_end1;
	}

	if(sock_type==SOCK_STREAM){
		if( listen(listener, 1) < 0){
			printf("Unable to listen: %s\n",strerror(errno));
			ret = -5;
			goto _err_end1;
		}
	}

	while(!quit_all){
_restart:
		if(sock_type==SOCK_STREAM){
			client = accept( listener, (struct sockaddr *)&sa, &sa_len );
			if( client < 0 ){
				printf("Unable to accept: %s\n", strerror(errno));
				continue;
			}

			printf("client %d connected.\n", client);
			char domainBuf[NI_MAXHOST];
			char portBuf[NI_MAXSERV];
			getnameinfo((struct sockaddr *)&sa, sa_len, domainBuf, sizeof(domainBuf), NULL, 0, NI_NUMERICHOST);
			printf("client(%d):[%s] connected, \n", client, domainBuf);
		}else
		if(sock_type==SOCK_DGRAM){
			client = listener;
			printf("SS: udp server waiting...\n");
			snd_ret = recvfrom( client, buf, data_len, 0, (struct sockaddr *)&sa, &sa_len );
			printf("ret:%d\n", snd_ret);
			printf("[%s]\n", strerror(errno));
		}
		while(!quit){
			if(sock_type==SOCK_DGRAM){
				//printf("SS: udp sendto first\n");
				snd_ret = sendto( client, buf, data_len, MSG_WAITALL, (struct sockaddr *)&sa, sa_len);
			}else
			if(sock_type==SOCK_STREAM)
				snd_ret = sendto( client, buf, data_len, MSG_WAITALL, NULL, 0);
			if(snd_ret != data_len){
				//printf("data sending not be finished(%d)\n", snd_ret);
				continue;
			}
			gettimeofday(&now_time,NULL);
			if(send_old_sec == now_time.tv_sec){
				send_cnt++;
			}else{
				count+=1;
				t_pkgs+=send_cnt;
				//printf("[pid:%d](%u:%u)        out_cnt:%d\n",mypid, count, t_pkgs, send_cnt);
				printf("[pid:%d](t:%u)    in_cnt:%d(%u kbps)\n",mypid, count, send_cnt, (data_len/1024)*8*send_cnt);
				send_old_sec=now_time.tv_sec;
				send_cnt=1;
			}

			
		}

		close(client);
	}

_err_end1:
	close(listener);
_err_end:
	if(buf != NULL)
		free(buf);
	return 0;
}

