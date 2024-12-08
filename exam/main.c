#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Necessary in my local mac machine for sockaddr_in struct
#include <netinet/in.h>

#define MAX_MSG_SIZE 1000000

typedef struct s_client {
	int id;
	char msg[MAX_MSG_SIZE];
} t_client;

t_client    clients[1024];
int 		maxfd;
char 		send_buffer[MAX_MSG_SIZE];
char 		recv_buffer[MAX_MSG_SIZE];
fd_set read_set;
fd_set write_set;
fd_set current;

void write_to_stderr(char *msg) {
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal Error", 11);
	write(2, "\n", 2);
	exit(1);
}

void send_to_clients(int except)
{
	for (int fd = 0; fd <= maxfd; fd++) {
		if (FD_ISSET(fd, &write_set) && fd != except) {
			if (send(fd, send_buffer, strlen(send_buffer), 0) == -1)
                write_to_stderr(NULL);
		}
	}
}


int main(int argc, char *argv[]) {
	if (argc != 2)
		write_to_stderr("Wrong number of arguments");

	int port_to_bind = atoi(argv[1]);

	// Server Socket Creation/Configuration
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		write_to_stderr(NULL);
	maxfd = server_fd;
	
	struct sockaddr_in  server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// ----

	// Bind to port
    server_addr.sin_port = htons(port_to_bind);

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		write_to_stderr(NULL);
	// ----
	
	// Check if works
	if (listen(server_fd, 50) < 0)
		write_to_stderr(NULL);
	// ----

	// Clients Socket Creation/Configurations
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	// ----

	FD_ZERO(&current); // Initialize the fd_set
    FD_SET(server_fd, &current); // Add server_fd to set

	// Accept and Handle clients connections
	int clients_id = 0;
	while (1) {
		read_set = write_set = current; // copy the set for select
		if (select(maxfd + 1, &read_set, &write_set, NULL, NULL) == -1)
			write_to_stderr(NULL);

		for (int fd = 0; fd <= maxfd; fd++) {
			if (FD_ISSET(fd, &read_set)) { // Check if fd is ready
				if (fd == server_fd) {
					int client_fd = accept(server_fd, (struct sockaddr *)&server_addr, &client_len);
					if (client_fd < 0)
						write_to_stderr(NULL);
					if (client_fd > maxfd) maxfd = client_fd;
					clients[client_fd].id = clients_id++;
					FD_SET(client_fd, &current);

					sprintf(send_buffer, "server: client %d just arrived\n", clients[client_fd].id);
					send_to_clients(client_fd);
					break;
				} else {
					int ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
					if (ret > 0) {
						recv_buffer[ret] = '\0';
						for (int i = 0; i < ret; i++) {
							// Append each character to the client's message buffer
							size_t len = strlen(clients[fd].msg);
							if (len < sizeof(clients[fd].msg) - 1) {
								clients[fd].msg[len] = recv_buffer[i];
								clients[fd].msg[len + 1] = '\0';
							}

							// Check if the character is a newline (end of message)
							if (recv_buffer[i] == '\n') {
								clients[fd].msg[len] = '\0'; // Terminate the message
								sprintf(send_buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
								send_to_clients(fd); // Send the message to all clients

								// Clear the client's message buffer for the next message
								bzero(&clients[fd].msg, strlen(clients[fd].msg));
							}							
						}
						break;
					} else {
						sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
						send_to_clients(fd);
						FD_CLR(fd, &current);
						close(fd);
						bzero(&clients[fd].msg, strlen(clients[fd].msg));
					}
				}
			break;
			}
		}
	}

	return (0);
}