#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#define SIZE 4096
#define PORT 2728
#define BACKLOG 10

void getFileURL(char *route, char *fileURL);
void getMimeType(char *file, char *mime);
void handleSignal(int signal);

void getTimeString(char *buffer);

int server_socket;
int clientSocket;

char *request;

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    signal(SIGINT, handleSignal);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
               &(int){1}, sizeof (int));

    if (bind(server_socket,
             (struct sockaddr *) &server_address,
             sizeof (server_address)) < 0) {
        fprintf(stderr, "Error binding %d to %d: %s\n",
                         server_socket, server_address.sin_addr.s_addr,
                         strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, BACKLOG) < 0) {
        fprintf(stderr, "Error listening to %d: %s\n",
                        server_socket, strerror(errno));
        exit(EXIT_FAILURE);
    }

    char hostBuffer[NI_MAXHOST];
    char serviceBuffer[NI_MAXSERV];
    int error = getnameinfo((struct sockaddr *)&server_address,
                            sizeof(server_address),
                            hostBuffer, sizeof(hostBuffer),
                            serviceBuffer, sizeof(serviceBuffer), 0);

    if (error) {
        fprintf(stderr, "Error getting name info from %d: %s\n",
                        server_address.sin_addr.s_addr, gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    printf("\nServer is listening on http://%s:%s/\n\n",
           hostBuffer, serviceBuffer);

    while (true) {
        char request[SIZE];
        char method[10], route[100];

        clientSocket = accept(server_socket, NULL, NULL);
        read(clientSocket, request, SIZE);

        sscanf(request, "%s %s", method, route);
        printf("%s %s", method, route);

        if (strcmp(method, "GET") != 0) {
            const char response[] = "HTTP/1.1 400 Bad Request\r\n\n";
            send(clientSocket, response, sizeof(response), 0);
        } else {
            char fileURL[100];

            getFileURL(route, fileURL);

            FILE *file = fopen(fileURL, "r");
            if (!file) {
                const char response[] = "HTTP/1.1 404 Not Found\r\n\n";
                send(clientSocket, response, sizeof(response), 0);
            } else {
                char resHeader[SIZE];
                char timeBuf[100];
                getTimeString(timeBuf);

                char mimeType[32];
                getMimeType(fileURL, mimeType);

                sprintf(resHeader,
                        "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\n\n",
                        timeBuf, mimeType);
                int headerSize = strlen(resHeader);

                printf(" %s", mimeType);

                fseek(file, 0, SEEK_END);
                long fsize = ftell(file);
                fseek(file, 0, SEEK_SET);

                char *resBuffer = (char *)malloc(fsize + headerSize);
                strcpy(resBuffer, resHeader);

                char *fileBuffer = resBuffer + headerSize;
                fread(fileBuffer, fsize, 1, file);

                send(clientSocket, resBuffer, fsize + headerSize, 0);
                free(resBuffer);
                fclose(file);
            }
        }
        close(clientSocket);
        printf("\n");
    }
    exit(EXIT_SUCCESS);
}

void getFileURL(char *route, char *fileURL) {
    char *question = strrchr(route, '?');
    if (question)
        *question = '\0';

    if (route[strlen(route) - 1] == '/') {
        strcat(route, "index.html");
    }

    strcpy(fileURL, "htdocs");
    strcat(fileURL, route);

    const char *dot = strrchr(fileURL, '.');
    if (!dot || dot == fileURL) {
        strcat(fileURL, ".html");
    }

    return;
}

void getMimeType(char *file, char *mime) {
    const char *dot = strrchr(file, '.');

    if (dot == NULL)
        strcpy(mime, "text/html");
    else if (strcmp(dot, ".html") == 0)
        strcpy(mime, "text/html");
    else if (strcmp(dot, ".css") == 0)
        strcpy(mime, "text/css");
    else if (strcmp(dot, ".js") == 0)
        strcpy(mime, "application/js");
    else if (strcmp(dot, ".jpg") == 0)
        strcpy(mime, "image/jpeg");
    else if (strcmp(dot, ".png") == 0)
        strcpy(mime, "image/png");
    else if (strcmp(dot, ".gif") == 0)
        strcpy(mime, "image/gif");
    else
        strcpy(mime, "text/html");

    return;
}

void handleSignal(int signal) {
    if (signal == SIGINT) {
        printf("\nShutting down server...\n");

        close(clientSocket);
        close(server_socket);

        if (request != NULL)
            free(request);

        exit(0);
    }
    return;
}

void getTimeString(char *buffer) {
    time_t now = time(NULL);
    struct tm tm = *gmtime(&now);
    strftime(buffer, sizeof (buffer), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return;
}
