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

#define p 5

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
	int i,n,fin,fileeof;
	int recv[p],result[p];
	socklen_t len;
	char mesg[4096], buffer[p][4096], Aack[5], seq[5];
	FILE *pf;
	pf = fopen("output_file", "wb");
	len = clilen;

	int ack[p];

	fin = p;
	fileeof = 0;

	for(i = 0;i<p;i++){
		ack[i] = i;
		recv[i]= 0;
	}

	for(;;){
start:
		if(fileeof){
		printf("finish\n");
			//if(readable_timeo(sockfd, 5) == 0)
			break;
		}

		n = recvfrom(sockfd, mesg, 4096, 0, pcliaddr, &len);

		memcpy(seq,(mesg + n - 5),5);
		for(i = 0;i<p;i++){
			if(atoi(seq) == ack[i]){
				if(n == 4096){
				result[i] = n;
				memcpy(buffer[i], mesg, (n-5));
				recv[i] = 1;
				sprintf(Aack,"%d",ack[i]);
				sendto(sockfd, Aack, 5, 0, pcliaddr, len);
				break;
				}
				else{
					if((n>5)&&(n<4096)){
						result[i] = n;
						memcpy(buffer[i],mesg,(n-5));
					}
					sprintf(Aack,"%d",ack[i]);
					sendto(sockfd, Aack, 5, 0, pcliaddr, len);
					recv[i] = 1;
					if(n == 5)
						if(fin > i)
							fin = i;
				}
			}
		}
		for(i = 0;i<p;i++){
			if(recv[i] == 0){
				goto start;
			}
		}

		for(i = 0;i<fin;i++){
			fwrite(buffer[i], 1, (result[i] - 5), pf);
			recv[i]= 0;
		}
		if(fin != p){
			fileeof = 1;
		}
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
