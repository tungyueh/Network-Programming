#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/time.h>

#define p 5

int readable_timeo(int fd, int sec)
{
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return (select(fd + 1, &rset, NULL, NULL, &tv));
}

void dg_cli(int sockfd,const struct sockaddr *pservaddr, socklen_t servlen)
{
	int n,i,j, fileeof;
	int recv[p];
	FILE* pf;
	long lsize;
	char buffer[p][4096], ack[5], Aseq[5];
	size_t result[p];

	fileeof = 0;

	pf = fopen("a_big_file", "rb");

	int seq[p];

	for(i = 0;i<p;i++){
		seq[i] = i;
		recv[i] = 0;
	}

	for(i = 0; i<p; i++){
		result[i] = fread(buffer[i], 1, 4091, pf);
		sprintf(Aseq,"%d", seq[i]);
		memmove(buffer[i] + result[i], Aseq, 5);
		sendto(sockfd, buffer[i], (result[i] + 5), 0, pservaddr, servlen);
	}

	for(;;){
retrs:
		if(readable_timeo(sockfd, 1) == 0){
printf("timeout\n");
			for(i = 0;i<p;i++){
				if(recv[i] == 0)
					sendto(sockfd, buffer[i], (result[i] + 5), 0, pservaddr, servlen);
			}
			goto retrs;
		}else
			recvfrom(sockfd, ack, 5, 0, NULL, NULL);

		for(i = 0;i<p;i++){
			if(atoi(ack) == seq[i]){
				recv[i] = 1;
			}
		}

		for(i = 0;i<p;i++)
			if(recv[i] == 0){
				goto retrs;
			}

		if(fileeof == 1)
			break;

		for(i = 0;i<p;i++){
			result[i] = fread(buffer[i], 1, 4091, pf);
			sprintf(Aseq,"%d", seq[i]);
			memmove(buffer[i] + result[i], Aseq, 5);
			sendto(sockfd, buffer[i], (result[i] + 5), 0, pservaddr, servlen);
			
			recv[i] = 0;
			if(result[i] == 0){
				fileeof = 1;
			}
		}
	}
	fclose(pf);
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if(argc != 3)
		printf("usage: udpcli <IPaddress> <Port>\n");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct timeval a, b;
	//getstart time
	if(gettimeofday(&a,NULL)!=0){
		printf("GET TIME OF DAY FAILED!!!\n");
		exit(1);
	}

	dg_cli(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	//get end time
	if(gettimeofday(&b,NULL)!=0){
		printf("GET TIME OF DAY FAILED!!!\n");
		exit(1);

	}
	float cost;
	cost = (b.tv_sec - a.tv_sec) * 1000;
	cost += (b.tv_usec - a.tv_usec) / 1000;
	printf("*************\nTotal time cost : %f ms\n************\n", cost);
	exit(0);
}
