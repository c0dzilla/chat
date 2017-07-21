#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10
#define PORT "3490"

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int c, char* argv[]){
	printf("Server is running\n");
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res;
	int listener, new_fd;
	fd_set master, read_fds;
	int fdmax, nbytes;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PORT, &hints, &res);

	listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(listener, res->ai_addr, res->ai_addrlen);
	listen(listener, BACKLOG);
	FD_SET(listener, &master);
	fdmax = listener;

	char welcome[] = "Welcome to the chat!\nEnter username: ";
	char client_ip[40];
	char msg[50];
	int nicked_users[10];
	char nicks_list[10][50];
	int user_count = 0;
	int nick_given;

	while (1){
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		for (int i=0;i<=fdmax;i++){
			if (FD_ISSET(i, &read_fds)){
				if (i == listener){
					addr_size = sizeof their_addr;
					new_fd = accept(listener, (struct sockaddr*)&their_addr, &addr_size);	
					if (new_fd == -1){
						perror("accept");
					}
					else{
						inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), client_ip, sizeof client_ip);
						printf("Established connection to %s\n", client_ip);
						FD_SET(new_fd, &master);
						if (new_fd>fdmax){
							fdmax = new_fd;
						}
						if (send(new_fd, welcome, strlen(welcome)+1, 0) == -1){
							perror("send");
						}
					}
				} 
				else{
					nbytes = recv(i, msg, sizeof msg, 0);
					if (nbytes < 0){
						perror("recv");
					}
					if (nbytes == 0){
						printf("socket:%d closed connection", i);
						close(i);
						FD_CLR(i, &master);	
					}
					if (nbytes > 0){
						nick_given = 0;
						for (int j=0;j<user_count;j++){
							if (nicked_users[j] == i){
								nick_given = 1;
								break;
							}
						}
						if (nick_given == 0){ 
							nicked_users[user_count] = i;
							strcpy(nicks_list[user_count], msg);
							user_count++;
						}
						else{
							for (int j=0; j<=fdmax; j++){
								if (FD_ISSET(j, &master)){
									if (j!=i && j!=listener){
										if (send(j, msg, strlen(msg)+1, 0) == -1){
											perror("send");
										}
									}
								}
							}
						}
					}
					memset(msg, '\0', strlen(msg));
				}
			}
		}
	}	
	
	return 0;
}