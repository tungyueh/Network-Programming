#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0) {
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
	for(n = 1; n < maxlen; n++) {
again:
		if((rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if(c == '\n')
				break;
		} else if(rc == 0) {
			*ptr = 0;
			return (n-1);
		} else {
			if(errno = EINTR) 
				goto again;
			return (-1);
		}
	}

	*ptr = 0;
	return (n);
}

int main(int argc, char **argv) 
{
	int i,maxi,maxfd,listenfd,connfd, sockfd;
	int nready,client[FD_SETSIZE];
	ssize_t n;
	fd_set rset,allset;
	char line[4096], cname[1024][20];//client user name don't use char* cname[1024]
	struct sockaddr_in cliaddr,servaddr;
	socklen_t clilen;
	int islog[FD_SETSIZE];//to know who has name
	for(i = 0; i < FD_SETSIZE; i++)//0 is client don't have name
		islog[i] = -1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(22822);

	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, 1024);

	printf("Server listen\n");

	maxfd = listenfd;
	maxi = -1;
	for(i = 0; i < FD_SETSIZE; i++) 
		client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for(;;) {
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if(FD_ISSET(listenfd, &rset)) {
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
			writen(connfd, "/serv What's your name? \n", 25);	//ask client name
			for(i = 0; i < FD_SETSIZE; i++){ 
				if(client[i] < 0) {
					client[i] = connfd;
					break;
				}
			}
			if(i == FD_SETSIZE)
				printf("Too many clients");
			FD_SET(connfd, &allset);
			if(connfd > maxfd)
				maxfd = connfd;
			if(i > maxi)
				maxi = i;
			if(--nready <= 0)
				continue;
		}
		for(i = 0; i <= maxi ; i++) {
			if((sockfd = client[i]) < 0)
				continue;
			if(FD_ISSET(sockfd, &rset)) {
				int j;
				char msg[4096];//msg:record message,tname:tmp name 
				if((n = readline(sockfd, line, 4096)) == 0) {
closeconnect:
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					if(islog[i] > 0){//is client[i] have name?
						for(j = 0; j < FD_SETSIZE; j++)//tell other client offline msg
							if((client[j] > 0) && (islog[j] > 0)){
								bzero(msg,sizeof(msg));
								sprintf(msg,"/serv %s is offline.\n",cname[i]);
								writen(client[j],msg,strlen(msg));
							}			
						bzero(cname[i],sizeof(cname[i]));//clear client name
						islog[i] = -1;//clear log record
					}
				} else{
					//distinguish the type which client send
					char *getok,*t;				//use to get strtok
					bzero(msg,sizeof(msg));
					getok = strtok(line," \n");
					if(getok == NULL){
						if(islog[i] < 0)
							writen(sockfd, "/serv A username can only consist of 2~12 digits or English letters.\n", 69);
						else{	
							sprintf(msg,"/msg %s SAID: \n",cname[i]);
							for(j = 0;j < FD_SETSIZE; j++)
								if((client[j] > 0) && (islog[j] > 0))
									writen(client[j], msg, strlen(msg));
						}
					}
					else{
						if(!strcmp(getok,"/who")){				//support /who command
							for(j = 0; j < FD_SETSIZE; j++)
								if((client[j] > 0) && (islog[j] > 0)){
									bzero(msg,sizeof(msg));
									struct sockaddr_in cli;
									socklen_t cli_len = sizeof(cli);
									char cliip[20],cliport[20];
									getpeername(client[j], (struct sockaddr*)&cli, &cli_len);
									inet_ntop(AF_INET, &cli.sin_addr, cliip, sizeof(cliip));
									sprintf(msg,"/serv %s %s:%d\n",cname[j],cliip,ntohs(cli.sin_port));
									writen(sockfd, msg, strlen(msg));
								}						
						}else if(!strcmp(getok,"/nick")){		//change client name
							getok = strtok(NULL,"\n");
							if(getok == NULL){
								writen(sockfd, "/serv Username can only consist of 2~12 digits or English letters.\n", 67);
							}else{
								int len = strlen(getok);		//check client name
								if((len < 2) || (len > 12)){		//length of name is OK?
									writen(sockfd, "/serv Username can only consist of 2~12 digits or English letters.\n", 67);
									break;
								}
								for(j = 0; j < len; j++){			//format of name is OK?
									if(!isalnum(getok[j])){
										writen(sockfd, "/serv Username can only consist of 2~12 digits or English letters.\n", 67);
										break;
									}
								}
								if(j == len) {
									for(j = 0;j < FD_SETSIZE; j++){	//check name have been used?
										if((client[j] > 0) && (i!=j)){
											if(!strcmp(getok,cname[j])){
												bzero(msg,sizeof(msg));
												sprintf(msg,"/serv %s has been used by others.\n",getok);
												writen(sockfd, msg, strlen(msg));
												break;
											}
										}	
									}
									if(j == FD_SETSIZE){		//no name is the same
										bzero(msg,sizeof(msg));	//tell client is successful change his name
										sprintf(msg,"/serv You're now known as %s.\n",getok);
										writen(sockfd, msg, strlen(msg));
										for(j = 0;j < FD_SETSIZE; j++)	//tell other client i am change name
											if((client[j] > 0) && (islog[j] > 0) && (i != j)){
												bzero(msg,sizeof(msg));	
												sprintf(msg,"/serv %s is known as %s.\n",cname[i],getok);
												writen(client[j], msg, strlen(msg));
											}
										strcpy(cname[i],getok);
									}	
								}
							}
						}else if(!strcmp(getok,"/quit")){				//client want to leave connect
							goto closeconnect;
						}else{
							if(islog[i]<0){
								int len = strlen(getok) /*- 1*/;//check client name
								if((len < 2) || (len > 12)){					//length of name is OK?
									writen(sockfd, "/serv A username can only consist of 2~12 digits or English letters.\n", 69);
									break;
								}
								for(j = 0; j < len; j++){						//format of name is OK?
									if(!isalnum(getok[j])){
										writen(sockfd, "/serv A username can only consist of 2~12 digits or English letters.\n", 69);
										break;
									}
								}
								if(j == len) {
									for(j = 0;j < FD_SETSIZE; j++){	//check name have been used?
										if(client[j] > 0){
											if(!strcmp(getok,cname[j])){
												writen(sockfd, "/serv This name has been used by others.\n", 41);
												break;
											}
										}	
									}
									if(j == FD_SETSIZE){		//no name is the same
										struct sockaddr_in serv;//get client ip port
										socklen_t serv_len = sizeof(serv);
										char servip[20],servport[20];
										getsockname(connfd, (struct sockaddr*)&serv, &serv_len);
										inet_ntop(AF_INET, &serv.sin_addr, servip, sizeof(servip));
										bzero(msg,sizeof(msg));
										sprintf(msg,"/serv hello %s, welcome! ServerIP:%s:%d\n",getok,servip,ntohs(serv.sin_port));
										writen(sockfd,msg,strlen(msg));
										strcpy(cname[i],getok);//store name
										islog[i] = 1;//log success
										for(j = 0; j < FD_SETSIZE; j++){//let other client know new client is added
											if((client[j] > 0) && (islog[j] > 0) && (i != j)){
												bzero(msg,sizeof(msg));
												sprintf(msg,"/serv %s is online.\n",getok);
												writen(client[j],msg,strlen(msg));
											}
										}
									}
								}
							}else{
								t = strtok(NULL,"\n");	//is there has left string?
								if(t == NULL){			//no
									bzero(msg,sizeof(msg));
									sprintf(msg,"/msg %s SAID: %s\n",cname[i],getok);
								}
								else{					//yes
									bzero(msg,sizeof(msg));
									sprintf(msg,"/msg %s SAID: %s %s\n",cname[i],getok,t);
								}
								for(j = 0;j < FD_SETSIZE; j++)
									if((client[j] > 0) && (islog[j] > 0))
										writen(client[j], msg, strlen(msg));
							}
						}
					}
				}	
				if(--nready <= 0)
					break;
			}
		}
	}
}
