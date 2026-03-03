#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
//Credit to sil@cpp.edu for working on parts of chat_client.c

void report(const char* msg, int terminate){
        perror(msg);
        if(terminate){
          exit(-1);
        }
}

//share flag between threads (helps with erros)
volatile int running = 1; 

//add mutex_lock
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// thread function to continuously recieve messages from the server
void* recieve_messages(void *arg) {
	int sockfd = *(int *)arg;
	char buffer[128];
	
	while (running) {
	bzero(buffer, sizeof(buffer));
	ssize_t n = read(sockfd, buffer, sizeof(buffer));
	if (n <= 0) {
		//server will closed connection 
		pthread_mutex_lock(&print_mutex);
		printf("connection closed by server.\n");
		pthread_mutex_unlock(&print_mutex);

		running = 0;
		break;
	}

	buffer[sizeof(buffer) - 1] = '\0';

	// print what comes from the server
	pthread_mutex_lock(&print_mutex);
	printf("%s\n", buffer);
	pthread_mutex_unlock(&print_mutex);
	
	// in case server sends "exit" stop
	if (strcmp(buffer, "exit") == 0) {
		running = 0;
		break;
	   }
	}
	return NULL;
} 

void serverFunctions(int sockfd){
	
	char username[128];
	bzero(username, sizeof(username));
	
	//fgets(username, sizeof(username), stdin);
	// Ask for the username
	pthread_mutex_lock(&print_mutex);
	printf("Enter a username: ");
	pthread_mutex_unlock(&print_mutex);
	scanf("%127s", username);

	// send username to the server
	send(sockfd, username, sizeof(username), 0);

	//recieves username message back from server
	recv(sockfd, username, sizeof(username), 0);
	pthread_mutex_lock(&print_mutex);
	printf("%s\n", username);	
	pthread_mutex_unlock(&print_mutex);

	// clear leftover line 
	int g;
	while ((g = getchar()) != '\n' && g != EOF) {
	// this will ignore/discard characters till end of line
	}

	// make a thread to recieve messages
	pthread_t recv_thread;
	if (pthread_create(&recv_thread, NULL, recieve_messages, &sockfd) != 0) {
	report("pthread_create", 1);
	}

	// main thread will handle user input and sending messages
	char input[128];
	while(running) {
	//initialize buffer stuff to 0
	bzero(input, sizeof(input));
	
	// reads a whole line from user
	if (fgets(input, sizeof(input), stdin) == NULL) {
	running = 0;
	break;
	}
	
	// strip new line at the end
	input[strcspn(input, "\n")] = '\0';

	// skips epty lines
	if (strlen(input) == 0) {
	continue;
	}
	
	//fixed size buffer
	char sendbuf[128];
	bzero(sendbuf, sizeof(sendbuf));
	strncpy(sendbuf, input, sizeof(sendbuf) - 1);

	// send to server
	write(sockfd, sendbuf, sizeof(sendbuf));
	
	// if user types 'exit' stop
	if (strcmp(input, "exit") == 0) {
	    running = 0;
	    break;
	  }
	}

	//wait for thread to finish
	pthread_join(recv_thread, NULL);
	
}
int main(){
	//port declaration here
	int PortNumber = 5311;
	//local host declaration below
	const char *Host = "login-01";

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
	report("socket", 1);
	}

	struct hostent* hptr = gethostbyname(Host);
	if(!hptr) report("gethostbyname", 1);//make sure hptr is not null
	if(hptr -> h_addrtype != AF_INET){
	report("bad address family", 1);
	}

	//connect to server
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = ((struct in_addr*) hptr->h_addr_list[0])->s_addr;
	saddr.sin_port = htons(PortNumber);

	//connect()
	//int connect(int socket, const struct sockaddr *address, socklen_t address_len);
	if (connect(sockfd, (struct sockaddr*) &saddr, sizeof(saddr)) < 0){
	report("connect", 1);

	}
	
	serverFunctions(sockfd);
	close(sockfd);
	
	//print a last message, after the chat ends no matter what why the chat ends 
	pthread_mutex_lock(&print_mutex);
	printf("Client Program Finished\n");
	pthread_mutex_unlock(&print_mutex);

	return 0;
}
