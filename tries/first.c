#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Necessary in my local mac machine for sockaddr_in struct
#include <netinet/in.h>

#define MAX_MSG_SIZE 1000000

int maxfd;
fd_set current, write_set, read_set;

typedef struct s_client {
	int id;
	char msg[MAX_MSG_SIZE];
} t_client;

t_client clients[1024];

char send_buffer[MAX_MSG_SIZE], recv_buffer[MAX_MSG_SIZE];

void err_msg(char *str) {
	if (str)
		write(2, str, strlen(str));
	else
		write(2, "Error\n", 6);
	exit(1);

}

void send_to_all(except_fd) {
	for (int fd = 0; fd < maxfd; fd++)
	{
		if (FD_ISSET(fd, &write_set) && fd != except_fd)
		{
			if (send(fd, send_buffer, strlen(send_buffer), 0) < 0)
				err_msg(NULL);
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2)
		err_msg("Wrong number of arguments\n");
	
	int port = atoi(argv[1]);

	int server_fd = maxfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		err_msg(NULL);
	
	struct sockaddr_in serv;
	bzero(&serv, sizeof(serv));
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(port);
	serv.sin_family = AF_INET;

	if (bind(server_fd, (const struct sockaddr*)&serv, sizeof(serv))  < 0)
		err_msg(NULL);
	
	if (listen(server_fd, 50) < 0)
		err_msg(NULL);

	FD_ZERO(&current);
	FD_SET(server_fd, &current);

	struct sockaddr_in client;
	socklen_t client_len = sizeof(client);

	int clients_id = 0;
	while (1)
	{
		read_set = write_set = current;
		if (select(maxfd + 1, &read_set, &write_set, NULL, NULL) == -1)
			write_to_stderr(NULL);

		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &read_set))
			{
				if (fd == server_fd)
				{
					int client_fd = accept(server_fd, (const struct sockaddr*)&serv, client_len);
					if (client_fd < 0)
						err_msg(NULL);
					clients[client_fd].id = clients_id++; 
					if (client_fd > maxfd) maxfd = client_fd;
					FD_SET(client_fd, &current);

					sprintf(send_buffer, "server: client %d just arrived\n", clients[client_fd].id);
					send_to_all(client_fd);
					break;
				} 
				else
				{
					int ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
					if (ret > 0)
					{
						recv_buffer[ret] = '\0';
						
					}
					else
					{
						sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
						send_to_all(fd);
						FD_CLR(fd, &current);
						close(fd);
						bzero(&clients[fd].msg, strlen(clients[fd].msg));
					}				
				}
			break;
			}
		}
	}
}