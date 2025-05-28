#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUF_SIZE 1024
#define NAME_LEN 32

int main() {
    int server_fd, client_fd, max_sd, activity, new_socket;
    int client_sockets[MAX_CLIENTS] = {0};
    char *usernames[MAX_CLIENTS] = {0};
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUF_SIZE];
    fd_set readfds;
    int port;

    // Prompt for port
    printf("Enter port to listen on: ");
    if (scanf("%d", &port) != 1) {
        fprintf(stderr, "Invalid port. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", port);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0)
                FD_SET(client_sockets[i], &readfds);
            if (client_sockets[i] > max_sd)
                max_sd = client_sockets[i];
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_socket < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("New connection: socket %d, IP %s, port %d\n",
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUF_SIZE - 1);
                if (valread <= 0) {
                    getpeername(sd, (struct sockaddr *)&address, &addrlen);
                    printf("Client %s disconnected\n", usernames[i] ? usernames[i] : "(unnamed)");
                    close(sd);
                    free(usernames[i]);
                    usernames[i] = NULL;
                    client_sockets[i] = 0;
                } else {
                    buffer[valread] = '\0';
                    if (!usernames[i]) {
                        // first message is username
                        char namebuf[NAME_LEN];
                        strncpy(namebuf, buffer, NAME_LEN - 1);
                        namebuf[strcspn(namebuf, "\r\n")] = '\0';
                        usernames[i] = strdup(namebuf);
                        printf("User '%s' joined\n", usernames[i]);
                    } else {
                        // broadcast message
                        printf("%s: %s", usernames[i], buffer);
                        char msg[BUF_SIZE + NAME_LEN];
                        snprintf(msg, sizeof(msg), "%s: %s", usernames[i], buffer);
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] > 0 && j != i)
                                send(client_sockets[j], msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
