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

static void sig_alrm(int signo)
{	
	return;
}

void dg_cli(int sockfd,const struct sockaddr *pservaddr, socklen_t servlen)
{
	int n;
	FILE* pf;
	long lsize;
	char buffer[4096], ack[2], Aseq[2];
	size_t result;

	pf = fopen("/tmp/input.tar", "rb");

	fseek(pf, 0, SEEK_END);
	lsize = ftell(pf);
	rewind(pf);
	
	signal(SIGALRM, sig_alrm);
	siginterrupt(SIGALRM, 1);

	int seq = 1;

	result = fread(buffer, 1, 4094, pf);
	sprintf(Aseq,"%d",seq);
	memmove(buffer + result, Aseq, 2);

	sendto(sockfd, buffer, (result + 2), 0, pservaddr, servlen);

	if(result < 4094)
		goto end;

	for(;;){
		if(seq)
			seq = 0;
		else
			seq = 1;
retrs:
		alarm(1);

		if((n = recvfrom(sockfd, ack, 2, 0, NULL, NULL)) < 0){
			if(errno == EINTR){
				sendto(sockfd, buffer, (result + 2), 0, pservaddr, servlen);
				goto retrs;
			}else
				printf("recvfrom error\n");
		}

		if(atoi(ack) != seq)
			goto retrs;

		result = fread(buffer, 1, 4094, pf);
		sprintf(Aseq,"%d", seq);
		memmove(buffer + result, Aseq, 2);

		sendto(sockfd, buffer, (result + 2), 0, pservaddr, servlen);
		
		if(result == 0)
			break;
	}
end:
	fclose(pf);
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if(argc != 3)
		printf("usage: udpcli <IPaddress>\n");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		
	struct timeval a,b;
	if(gettimeofday(&a,NULL) != 0){
		printf("GET TIME OF DAY FAILED!!!\n");
		exit(1);
	}

	dg_cli(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	if(gettimeofday(&b,NULL) != 0){
		printf("GET TIME OF DAY FAILED!!!\n");
		exit(1);
		
	}

	float timecost;
	timecost = (b.tv_sec - a.tv_sec) * 1000;
	timecost += (b.tv_usec - a.tv_usec) / 1000;

	printf("********************\nTotal time cost: %f ms\n********************\n",timecost);
	exit(0);
}
