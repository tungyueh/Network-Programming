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
#include<sys/select.h>

ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft)) <= 0) {
			if(nwritten <0 && errno == EINTR)
				nwritten = 0;
			else 
				return (-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;

	ptr = vptr;
	for(n = 1; n < maxlen; n++){
		again:
		if((rc = read(fd, &c, 1)) == 1){
			*ptr++ = c;
			if(c == '\n')
				break;
		}else if(rc == 0){
			*ptr = 0;
			return (n-1);
		}else{
			if(errno == EINTR)
				goto again;
			return (-1);
		}
	}

	*ptr = 0;
	return n;
}

void str_cli(FILE *fp, int sockfd)
{
	char sendline[4096], recvline[4096];
	fd_set rset;
	int maxfdp1;

	FD_ZERO(&rset);
	for(;;){
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = fileno(fp)>sockfd ? (fileno(fp)+1):(sockfd+1);
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &rset)){
			if(readline(sockfd, recvline, 4096) == 0){
				printf("str_cli:server terminated permaturely\n");
				exit(0);
			}
			//to determine which type of msg that the server send
			char *getok;		//use to get strtok 
			getok = strtok(recvline," \n");
			if(!strcmp(getok,"/serv")){
				printf("[Server] ");
				getok = strtok(NULL,"\n");
				fputs(getok, stdout);
				printf("\n");

			}else if(!strcmp(getok,"/msg")){
				getok = strtok(NULL,"\n");
				fputs(getok, stdout);
				printf("\n");

			}else{
				fputs(getok, stdout);
				getok = strtok(NULL,"\n");
				fputs(getok, stdout);
				printf("\n");
			}
		}

		if(FD_ISSET(fileno(fp), &rset)){
			if(fgets(sendline, 4096, fp) == NULL)
				return;
			char *getok,*t,line[4096];		//use to get strtok 
			getok = strtok(sendline," \n");
			bzero(line,sizeof(line));
			if(getok == NULL)
				writen(sockfd,"\n",1);
			else{
				if(!strcmp(getok,"/quit"))
					exit(0);
				t = strtok(NULL,"\n");
				if(t == NULL)
					sprintf(line,"%s\n",sendline);
				else
					sprintf(line,"%s %s\n",getok,t);
				writen(sockfd, line, strlen(line));
			}
		}
	}
}

int main(int argc, char **argv)
{
	int sockfd, n;
	struct sockaddr_in servaddr;
	char line[4096];
	int servport;//server port
	char *servIP;//server IP

	if(argc != 3){	
		char *getok;
		for(;;){
ConnectTo:
			printf("It is not connected to any server\n");
			printf("Please use /connect <IP address> <Port number>\n");
			bzero(&line, sizeof(line));
			fgets(line, 4096, stdin);
			getok = strtok(line," \n");			//eat '/connect' or '/quit'
			if(getok == NULL)
				continue;
			if(!strcmp(getok,"/quit"))
				exit(0);
			else if(!strcmp(getok,"/connect")){		//determine is command OK?
				servIP = strtok(NULL," ");		//get server IP
				if(servIP == NULL)
					continue;
				getok = strtok(NULL,"\n");		//get server port number
				if(getok == NULL)
					continue;
				servport = atoi(getok);			//type transformation 
				break;
			}
			else
				continue;
		}
	}
	else{
		servIP = argv[1];
		servport = atoi(argv[2]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);
	inet_pton(AF_INET, servIP, &servaddr.sin_addr);

	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		goto ConnectTo;

	str_cli(stdin, sockfd);
}
