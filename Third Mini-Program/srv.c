#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<sys/errno.h>
#include<fcntl.h>

#define MAXLINE 3388890 
#define CLINUM  10

int main(int argc, char **argv)
{
	int i, maxi, maxfd, maxfdp1, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	int val;
	ssize_t n, nwritten;
	fd_set wset, rset;
	socklen_t clilen;
	struct sockaddr_in servaddr,cliaddr;

	char to[CLINUM][MAXLINE], fr[MAXLINE];

	memset(fr, 0, MAXLINE + 1);
	for(i = 0; i < CLINUM; i++)
		memset(to[i], 0, MAXLINE + 1);

	char *tooptr[CLINUM], *toiptr[CLINUM], *friptr, *froptr;

	friptr = froptr = fr;
	for(i = 0; i < CLINUM; i++)
		toiptr[i] = tooptr[i] = to[i];
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	listen(listenfd, 1024);

	maxfdp1 = 0;
	maxfd = listenfd;
	maxi = -1;

	for(i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;

	for(;;){

		for(i = 0; i < FD_SETSIZE; i++)
			if(client[i] > 0)
				if(client[i] > maxfdp1)
					maxfdp1 = client[i];
		
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		if(friptr < &fr[MAXLINE]){
			for(i = 0; i < FD_SETSIZE; i++)
				if(client[i] > 0){
printf("set client(%d) in rset\n",client[i]);
					FD_SET(client[i], &rset);
				}
		}

		for(i = 0; i < CLINUM; i++){
			if(client[i] > 0)
				if(tooptr[i] != toiptr[i]){
printf("set client(%d) in wset\n",client[i]);
					FD_SET(i + 4, &wset);
				}
		}
			

		if(maxfd  > maxfdp1)
			maxfdp1 = maxfd ;
		
		FD_SET(listenfd, &rset);

printf("selecting...%d\n",maxfdp1);
		select(maxfdp1 + 1, &rset, &wset, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
printf("connecting...\n");
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
			
			val = fcntl(connfd, F_GETFL, 0);
			fcntl(connfd, F_SETFL, val | O_NONBLOCK);

			for(i = 0; i < FD_SETSIZE; i++)
				if(client[i] < 0){
					client[i] = connfd;
printf("connect to client(%d)\n",connfd);
					break;
				}
		}

		for(i = 0 ; i < FD_SETSIZE ; i++){
			if(client[i] < 0)
				continue;
			if(FD_ISSET(client[i], &rset)){
printf("client(%d) ",client[i]);
				if((n = read(client[i], friptr, &fr[MAXLINE] - friptr)) < 0){
					if(errno != EWOULDBLOCK)
						printf("read error on socket\n");
				}else if(n == 0){
					close(client[i]);
					client[i] = -1;
					printf("client terminated\n");
				}else{
printf("read %d bytes\n",n);
					friptr += n;
					int j;
					for(j = 0; j < CLINUM; j++){
						if(client[j] > 0){
							strncpy(toiptr[j], froptr, n);
							toiptr[j] += n;
						}
					}

					froptr += n;
					if(froptr == friptr)
						froptr = friptr = fr;
				}
			}
		}

		for(i = 0; i < FD_SETSIZE; i++){
			if(client[i] < 0)
				continue;
			if(FD_ISSET(client[i], &wset) && ((n = toiptr[i] - tooptr[i]) > 0)){
printf("client(%d) ",client[i]);
				if((nwritten = write(client[i], tooptr[i], n)) < 0){
					if(errno != EWOULDBLOCK)
						printf("write error to socket\n");
				}else{
printf("write %d bytes\n",nwritten);
					tooptr[i] += nwritten;
					if(tooptr[i] == toiptr[i]){
						toiptr[i] = tooptr[i] = to[i];
					}
				}
			}
		}
	}
}


