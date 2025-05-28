#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];
    fd_set readfds;
    int port;
    char username[BUF_SIZE];

    printf("Enter server IP (e.g., 127.0.0.1): ");
    if (!fgets(buffer, sizeof(buffer), stdin)) return 1;
    buffer[strcspn(buffer, "\r\n")] = '\0';
    char server_ip[BUF_SIZE];
    strncpy(server_ip, buffer, BUF_SIZE);

    printf("Enter port to connect to: ");
    if (scanf("%d", &port) != 1) {
        fprintf(stderr, "Invalid port. Exiting.\n");
        return -1;
    }
    getchar(); // consume newline

    printf("Enter your username: ");
    if (!fgets(username, sizeof(username), stdin)) return 1;
    username[strcspn(username, "\r\n")] = '\0';

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // send username as first message
    send(sock, username, strlen(username), 0);
    printf("Connected as %s. Type messages and hit Enter.\n", username);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(sock, &readfds);
        int max_sd = sock;

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(0, &readfds)) {
            if (fgets(buffer, BUF_SIZE, stdin)) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUF_SIZE - 1);
            if (valread <= 0) {
                printf("Server closed connection\n");
                break;
            }
            buffer[valread] = '\0';
            printf("%s", buffer);
        }
    }

    close(sock);
    return 0;
}

