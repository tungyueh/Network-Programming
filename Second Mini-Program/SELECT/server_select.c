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

int readable_timeo(int fd, int sec)
{
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return(select(fd + 1, &rset, NULL, NULL, &tv));
}

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int n;
	socklen_t len;
	char mesg[4096], buffer[4096], Aack[2], seq[2];
	FILE *pf;
	pf = fopen("received.tar", "wb");
	len = clilen;

	int ack = 1;

	for(;;){
		if((n > 2) && (n < 4096)){
			if(readable_timeo(sockfd, 5) == 0)
				break;
			else
				break;
		}

		n = recvfrom(sockfd, mesg, 4096, 0, pcliaddr, &len);

		memcpy(buffer, mesg, (n-2));
		memcpy(seq,(mesg + n - 2),2);

		if(ack != atoi(seq)){
			sprintf(Aack,"%d",ack);
			sendto(sockfd, Aack, 2, 0, pcliaddr, len);
			continue;
		}

		fwrite(buffer, 1, (n - 2), pf);
		if(ack)
			ack = 0;
		else 
			ack = 1;
		sprintf(Aack,"%d",ack);
		sendto(sockfd, Aack, 2, 0, pcliaddr, len);
	}
	fclose(pf);
	exit(0);
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	dg_echo(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
}
