#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//Note to reader: Credit given to Professor lucero's 2600 slides & geeksforgeeks.org
//socket steps:
//Socket() get file descriptor for socket connection
//bind() bind socket to address on server host
//listen() listen for client requests
//accept() accept client request
//NOTE FOR RUNNING: Ensure that the localhost on the client end is set to the host user@"login-01"
int fdArray[8];
pthread_t id2;
pthread_t readthread;
pthread_t thread[8];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *fp;
int running = 1;
int t_used[8] = {0};

void report(const char* msg, int terminate){
	perror(msg);
	if(terminate){
	exit(-1);
	}
}

void broadcast(char *buffer){
	pthread_mutex_lock(&mutex);
	fp = fopen("chatlog.txt", "a");
	fprintf(fp, buffer);
        fprintf(fp, "\n:");
	for(int i = 0; i < 8; i++){
	if(fdArray[i] != 0){
	send(fdArray[i], buffer, strlen(buffer), 0);
	}
	}
	fclose(fp);
	pthread_mutex_unlock(&mutex);
}

void *serverInput(){
	char string[20];
	scanf("%s", string);
	if(strcmp(string, "exit") == 0){
	printf("Program ended sucessfully\n");
	
	for(int j = 0; j < 8; j++){
                if(t_used[j]){
                        //if(pthread_join(thread[j], NULL)!= 0){
                        //return 2;
                        //}
                }
        }
	
        //pthread_join(id2, NULL);
	fclose(fp);
	exit(0);
	}
}

void *serverFunctions(void *clientfdp){
	int *clientfd = (int *)(clientfdp);
	char username[128];
	char user[128];
	char userTag[128];
	char leaveMsg[128];
	recv(*clientfd, username, sizeof(username), 0);
	strcpy(user, username);
	strcat(username, " joined\n");
	broadcast(username);
	printf("%s\n", username);
        strcpy(username, user);
	strcat(username, ": ");
	strcpy(userTag, username);
	
	int running = 1;
	while(running){
	char buffer[128];
	bzero(buffer, 128);
	pthread_mutex_lock(&mutex);
	read(*clientfd, buffer, sizeof(buffer));
	pthread_mutex_unlock(&mutex);
	if(strcmp(buffer, "exit") == 0){
	strcpy(leaveMsg, user);
	strcat(user, " left\n");
	broadcast(user);
	strcpy(user, leaveMsg);
	printf("%s left\n", user);
	break;
	}
	strcat(username, buffer);
	strcpy(buffer, username);
	broadcast(buffer);
	printf("%s\n", buffer);
	strcpy(username, userTag);
	}
	int fd_forEach = *clientfd;
	pthread_mutex_lock(&mutex);
	*clientfd = 0;
	pthread_mutex_unlock(&mutex);
	close(fd_forEach);
	return NULL;	
	
}

int main(){
	int PortNumber = 5311;
	//Maximum connections set to 8
	int MaxConnects = 8;
	char buffer[128];
	//file descriptor int creation
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
	//terminates if fd less than 0
	report("socket", 1);
	}

	//binds sever local address in memory
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PortNumber);
	
	//binding below
	if(bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0){
		report("bind", 1);
	}
	//listening below
	if(listen(fd, MaxConnects) < 0){
	
	report("listen", 1);
	}
	fprintf(stderr, "Listening on port %i for clients...\n", PortNumber);
	//below turns client handling into an array
	//change to while running later
	pthread_create(&id2, NULL, serverInput, NULL);
	while(1){
	struct sockaddr_in caddr;
        int len = sizeof(caddr);
        int client_fd = accept(fd, (struct sockaddr*) &caddr, &len);
        if(client_fd < 0){
        report("accept", 0);
        continue;
        }
        //delete later, to test client connectivity
        fprintf(stderr, "Client connected from %s:%d\n",
                inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
	pthread_mutex_lock(&mutex);
	//add it here so that it can be accessed for server cnt
	int i = 0;
	for(i = 0; i < 8; i++){
		if(fdArray[i] == 0){
		fdArray[i] = client_fd;
		break;
		}
	}
	pthread_mutex_unlock(&mutex);

	if(i == 8){
	const char *full = "Sorry, server is full\n";
	send(client_fd, full, strlen(full), 0);
	close(client_fd);
	continue;
	}
	pthread_t id;
	if(pthread_create(&id, NULL, serverFunctions, (void*) &fdArray[i]) != 0){
	perror("creation issue");
	pthread_mutex_lock(&mutex);
	fdArray[i] = 0;
	pthread_mutex_unlock(&mutex);
	close(client_fd);
	continue;
	}	
	}
	for(int j = 0; j < 8; j++){
		if(t_used[j]){
			if(pthread_join(thread[j], NULL)!= 0){
			return 2;
			}
		}
	}
	pthread_join(id2, NULL);
	printf("%s", "\nProgram ended succesfully\n");
	return 0;
}

