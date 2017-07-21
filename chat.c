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
	printf("Server is running...\n");
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints, *res;
	int listener, new_fd;
	fd_set master, read_fds;
	int fdmax, nbytes;
	/*
	loop variables
	*/
	int i,j,k;

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
	char* super_keyword = "root";
	char client_ip[40];
	char msg[50];
	int nicked_users[10];
	int super_user = -1;
	char nicks_list[10][50];
	char sender_name[100];
	int user_count = 0;
	int nick_given;

	while (1){
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(4);
		}
		for (i=0;i<=fdmax;i++){
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
						printf("socket:%d closed connection.\n", i);
						for (j=0;j<user_count;j++){
							if (nicked_users[j] == i){
								break;
							}
						}
						for (k=j+1;k<user_count;k++){
							nicked_users[k-1] = nicked_users[k];
							strcpy(nicks_list[k-1],nicks_list[k]);
						}
						if (i==super_user){
							super_user = -1;
						}
						user_count--;
						close(i);
						FD_CLR(i, &master);	
					}
					if (nbytes > 0){
						nick_given = 0;
						for (j=0;j<user_count;j++){
							if (nicked_users[j] == i){
								nick_given = 1;
								break;
							}
						}
						if (nick_given == 0){
							nicked_users[user_count] = i;
							strcpy(nicks_list[user_count], msg);
							nicks_list[user_count][strlen(nicks_list[user_count])-1] = '\0';
							if (strcmp(nicks_list[user_count], super_keyword) == 0){
								super_user = i;
								printf("Super user allocated.\n");
							} 
							user_count++;
						}
						else{
							for (j=0;j<user_count;j++){
								if (nicked_users[j] == i){
									strcpy(sender_name, nicks_list[j]);
									strcat(sender_name, ": ");
									strcat(sender_name, msg);
									strcpy(msg, sender_name);
									memset(sender_name, '\0', strlen(sender_name));
									break;
								}
							}
							for (j=0; j<=fdmax; j++){
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