#include <stdio.h>		// Standard input/output functions
#include <stdlib.h>		// Standard library functions (malloc, free, etc.)
#include <unistd.h>		// UNIX standard functions (close, etc.)
#include <string.h>		// String manipulation functions
#include <sys/socket.h> // Socket API
#include <netinet/in.h> // Internet address family
#include <errno.h>		// Error handling
#include <signal.h>		// Signal handling for clean termination

/*

struct s_client

6 lignes variables

7 fonctions :
void cleanup();
void sigint_handler(int sig);
void err(char *msg);
char *str_join(char *buf, char *add);
int extract_message(char **buf, char **msg);
void send_to_all(int except);
int main(int ac, char **av);

*/

// Client structure to keep track of connected clients
typedef struct s_client
{
	int id;	   // Unique identifier for the client
	char *msg; // Buffer to store partial messages
} t_client;

// Global variables
t_client clients[1024];					   // Array to store client information (indexed by file descriptor)
fd_set read_set, write_set, current;	   // File descriptor sets for select()
int maxfd = 0;							   // Highest file descriptor for select()
int gid = 0;							   // Global ID counter for client identification
int serverfd;							   // Server socket file descriptor
char send_buffer[4096], recv_buffer[4096]; // Buffers for sending and receiving data

/**
 * Cleanup function: frees allocated memory and closes all sockets
 * Called when the server is shutting down
 */
void cleanup()
{
	// Free all client message buffers and close client connections
	for (int fd = 0; fd <= maxfd; fd++)
	{
		if (FD_ISSET(fd, &current) && fd != serverfd)
		{
			if (clients[fd].msg)
				free(clients[fd].msg);
			close(fd);
		}
	}

	// Close server socket
	if (serverfd > 0)
		close(serverfd);
}

/**
 * Signal handler for SIGINT (Ctrl+C)
 * Ensures clean termination when the user terminates the server
 */
void sigint_handler(int sig)
{
	cleanup();
	exit(0);
}

/**
 * Error handling function
 * Prints an error message and exits the program
 */
void err(char *msg)
{
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

/**
 * Concatenates a string to an existing buffer
 * Allocates new memory for the combined string and frees the old buffer
 *
 * @param buf The existing buffer (may be NULL)
 * @param add The string to append
 * @return A new buffer containing the combined string
 */
char *str_join(char *buf, char *add)
{
	char *newbuf;
	int len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (strlen(add) + len + 1));
	if (newbuf == 0)
		err(NULL);
	newbuf[0] = 0; // NB
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

/**
 * Extracts a complete message from a buffer (ending with newline)
 * Updates the buffer and sets msg to point to the extracted message
 *
 * @param buf Pointer to the buffer containing received data
 * @param msg Pointer to store the extracted message
 * @return 1 if a message was extracted, 0 if no complete message, -1 on error
 */
int extract_message(char **buf, char **msg)
{
	char *newbuf;
	int i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			// Found a newline - extract the message
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1); // Copy remaining data after newline
			*msg = *buf;				  // Set msg to point to the entire original buffer
			(*msg)[i + 1] = 0;			  // Null-terminate the message after the newline
			*buf = newbuf;				  // Update buf to point to the remaining data
			return (1);
		}
		i++;
	}
	return (0); // No complete message found
}

/**
 * Sends a message to all connected clients except the specified one
 *
 * @param except File descriptor of client to exclude from broadcast
 */
void send_to_all(int except)
{
	for (int fd = 0; fd <= maxfd; fd++)
	{
		// Send to all connected clients except the server and the specified client
		if (FD_ISSET(fd, &current) && fd != serverfd && fd != except)
			if (send(fd, send_buffer, strlen(send_buffer), 0) < 0)
				err(NULL);
	}
}

/**
 * Main function - initializes and runs the chat server
 */
int main(int ac, char **av)
{
	// Set up signal handler for clean termination
	signal(SIGINT, sigint_handler);

	// Check command line arguments
	if (ac != 2)
		err("Wrong number of arguments");

	struct sockaddr_in serveraddr;
	socklen_t len = sizeof(serveraddr);

	// Create server socket
	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1)
		err(NULL);

	// Initialize file descriptor sets
	maxfd = serverfd;
	FD_ZERO(&current);
	FD_SET(serverfd, &current); // FD_SET

	// Initialize client array and server address structure
	memset(clients, 0, sizeof(clients));
	memset(&serveraddr, 0, sizeof(serveraddr));

	// Set up server address
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	serveraddr.sin_port = htons(atoi(av[1]));		// Port from command line argument

	// Bind socket to the address and port
	if (bind(serverfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
		err(NULL);

	// Start listening for connections
	if (listen(serverfd, 128) == -1)
		err(NULL);

	printf("Server listening on port %s...\n", av[1]);

	// Main server loop
	while (1)
	{
		// Prepare file descriptor sets for select()
		read_set = current;
		write_set = current;

		// Wait for activity on any of the file descriptors
		if (select(maxfd + 1, &read_set, &write_set, NULL, NULL) < 0)
			continue;

		// Check each file descriptor for activity
		for (int fd = 0; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &read_set)) // & READ_SET
			{
				if (fd == serverfd)
				{
					// New connection request
					int clientfd = accept(serverfd, (struct sockaddr *)&serveraddr, &len);
					if (clientfd < 0)
						continue; // CONTINUE AND NOT ERR NUL

					// Add new client to the set
					FD_SET(clientfd, &current);
					clients[clientfd].id = gid++;
					clients[clientfd].msg = NULL;

					// Update maxfd if needed
					if (clientfd > maxfd)
						maxfd = clientfd;

					// Notify all clients about the new connection
					sprintf(send_buffer, "server: client %d just arrived\n", clients[clientfd].id);
					printf("%s", send_buffer); // Debug output
					send_to_all(clientfd);
				}
				else
				{
					// Data from an existing client
					int ret = recv(fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
					if (ret > 0)
					{
						// Data received
						recv_buffer[ret] = '\0';
						clients[fd].msg = str_join(clients[fd].msg, recv_buffer);

						// Extract and process complete messages
						char *message = NULL;
						while (extract_message(&clients[fd].msg, &message) == 1)
						{
							// Broadcast the message to all other clients
							sprintf(send_buffer, "client %d: %s", clients[fd].id, message);
							printf("%s", send_buffer); // Debug output
							send_to_all(fd);
							free(message); // NB
						}
					}
					else
					{
						// Client disconnected
						sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
						printf("%s", send_buffer); // Debug output
						send_to_all(fd);

						// Clean up client resources
						if (clients[fd].msg)
							free(clients[fd].msg);
						clients[fd].msg = NULL;

						FD_CLR(fd, &current);
						close(fd);
					}
				}
			}
		}
	}
}