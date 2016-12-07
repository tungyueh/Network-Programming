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

#define max(a,b) ((a) > (b) ? (a) : (b))

void cli_echo(int sockfd)
{
	int maxfdp1, val, stdineof;
	ssize_t n, nwritten;
	fd_set rset, wset;
	char to[33888990], fr[33888990];
	char *toiptr, *tooptr, *friptr, *froptr;

	val = fcntl(sockfd, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDOUT_FILENO, F_GETFL, 0);
	fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

	toiptr = tooptr = to;
	friptr = froptr = fr;
	stdineof = 0;

	maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;
	for(;;){
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		if(stdineof == 0 && toiptr < &to[4096])
			FD_SET(STDIN_FILENO, &rset);
		if(friptr < &fr[4096])
			FD_SET(sockfd, &rset);
		if(tooptr != toiptr)
			FD_SET(sockfd, &wset);
		if(froptr != friptr)
			FD_SET(STDOUT_FILENO, &wset);

		select(maxfdp1, &rset, &wset, NULL, NULL);

		if(FD_ISSET(STDIN_FILENO, &rset)){
			if((n = read(STDIN_FILENO, toiptr, &to[4096] - tooptr)) < 0){
				if(errno != EWOULDBLOCK)
					printf("read error on stdin\n");
			}else if(n == 0){
				stdineof = 1;
				if(tooptr == toiptr)
					shutdown(sockfd, SHUT_WR);
			}else{
				toiptr += n;
				FD_SET(sockfd, &wset);
			}
		}

		if(FD_ISSET(sockfd, &rset)){
			if((n = read(sockfd, friptr, &fr[4096] - friptr)) < 0){
				if(errno != EWOULDBLOCK)
					printf("read error on socket\n");
			}else if(n == 0){
				if(stdineof)
					return;
				else{
					printf("server terminated prematurely.\n");
					return;
				}
			}else{
				friptr += n;
				FD_SET(STDOUT_FILENO, &wset);
			}
		}

		if(FD_ISSET(STDOUT_FILENO, &wset) && ((n = friptr - froptr) > 0)){
			if((nwritten = write(STDOUT_FILENO, froptr, n)) < 0){
				if(errno != EWOULDBLOCK)
					printf("write error to stdout\n");
			}else {
				froptr += nwritten;
				if(froptr == friptr)
					froptr = friptr = fr;
			}
		}

		if(FD_ISSET(sockfd, &wset) && ((n = toiptr - tooptr) > 0)){
			if((nwritten = write(sockfd, tooptr, n)) < 0){
				if(errno != EWOULDBLOCK)
					printf("write error to socket\n");
			}else{
				tooptr += nwritten;
				if(tooptr == toiptr){
					toiptr = tooptr = to;
					if(stdineof)
						shutdown(sockfd, SHUT_WR);
				}
			}
		}
	}
}


int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;


	if(argc != 3)
		printf("Usage: client <IP address> <Port number>\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	cli_echo(sockfd);
}
