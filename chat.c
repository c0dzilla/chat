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
#include <stdbool.h>

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
	char welcome_admin[] = "Welcome admin! Mention your codename: ";
	char *password = "sherbetlemon";
	char* command;
	char client_ip[40];
	char msg[50];
	char msg_copy[50];
	char ban_message[100];
	int nicked_users[10];
	int super_users[10];
	char nicks_list[10][50];
	char sender_name[100];
	char user_password[10][25];
	int user_count = 0;
	int super_user_count = 0;
	bool is_super_user, is_nick;
	
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
						printf("socket:%d connection closed.\n", i);
						is_super_user = false;
						for (j=0;j<super_user_count;j++){
							if (super_users[j] == i){
								is_super_user = true;
								break;
							}
						}
						if (is_super_user){
							for (k=j+1;k<super_user_count;k++){
								super_users[k-1] = super_users[k];
							}
							super_user_count--;
						}
						for (j=0;j<user_count;j++){
							is_nick = false;
							if (nicked_users[j] == i){
								is_nick = true;
								break;
							}
						}
						if (is_nick){
							for (k=j+1;k<user_count;k++){
								nicked_users[k-1] = nicked_users[k];
								strcpy(nicks_list[k-1],nicks_list[k]);
							}
							user_count--;
						}
						close(i);
						FD_CLR(i, &master);	
					}
					if (nbytes > 0){
						is_nick = false;
						for (j=0;j<user_count;j++){
							if (nicked_users[j] == i){
								is_nick = true;
								break;
							}
						}
						if (!is_nick){
							msg[strlen(msg)-1] = '\0';
							is_super_user = false;
							for (j=0;j<super_user_count;j++){
								if (super_users[j] == i){
									is_super_user = false;
									break;
								}
							}
							if (is_super_user){
								nicked_users[user_count] = i;
								strcpy(nicks_list[user_count], msg);
								user_count++;
							}
							else{
								if (strcmp(msg, password) == 0){
									send(i, welcome_admin, strlen(welcome_admin)+1, 0);
									super_users[super_user_count] = i;
									super_user_count++;
									printf("Added %d as super user\n", i);
								}
								else{
									nicked_users[user_count] = i;
									strcpy(nicks_list[user_count], msg);
									user_count++;
								}
							} 
						}
						else{
							is_super_user = false;
							for (j=0;j<super_user_count;j++){
								if  (super_users[j] == i){
									is_super_user = true;
								}
							}
							if (is_super_user){
								strcpy(msg_copy, msg);
								command = strtok(msg_copy, " ");
								if (strcmp(command, "ban") == 0){
									command = strtok(NULL, "\n");
									is_nick = false;
									for (j=0;j<user_count;j++){
										if (strcmp(nicks_list[j], command) == 0){
											is_nick = true;
											break;
										}
									}
									if (is_nick){
										is_super_user = false;
										for (k=0;k<super_user_count;k++){
											if (super_users[k] == nicked_users[j]){
												is_super_user = true;
											}
										}
										if (!is_super_user){
											strcpy(ban_message, nicks_list[j]);
 											close(nicked_users[j]);
 											FD_CLR(nicked_users[j], &master);
											strcat(ban_message, " has been banned!\n");
											printf("socket:%d user has been banned.\n", nicked_users[j]);
											for (k=j+1;k<user_count;k++){
												nicked_users[k-1] = nicked_users[k];
												strcpy(nicks_list[k-1], nicks_list[k]);
											}
											user_count--;
											/*
											for (k=0;k<=fdmax;k++){
												if (FD_ISSET(k, &master)){
													send(k, ban_message, strlen(ban_message)+1, 0);
												}
											}
											*/
										}		
									}
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
					}
					memset(msg, '\0', strlen(msg));
				}
			}
		}
	}	
	
	return 0;
}