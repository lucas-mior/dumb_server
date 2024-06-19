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

void get_file_url(char *route, char *file_url);
void get_mime_type(char *file, char *mime);
void handle_signal(int signal);

void get_time_string(char *buffer);

int server_socket;
int client_socket;

char *request;

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    signal(SIGINT, handle_signal);

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

    char host_buffer[NI_MAXHOST];
    char service_buffer[NI_MAXSERV];
    int error = getnameinfo((struct sockaddr *)&server_address,
                            sizeof(server_address),
                            host_buffer, sizeof(host_buffer),
                            service_buffer, sizeof(service_buffer), 0);

    if (error) {
        fprintf(stderr, "Error getting name info from %d: %s\n",
                        server_address.sin_addr.s_addr, gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    printf("\nServer is listening on http://%s:%s/\n\n",
           host_buffer, service_buffer);

    while (true) {
        char request[SIZE];
        char method[10], route[100];

        client_socket = accept(server_socket, NULL, NULL);
        read(client_socket, request, SIZE);

        sscanf(request, "%s %s", method, route);
        printf("%s %s", method, route);

        if (strcmp(method, "GET") != 0) {
            const char response[] = "HTTP/1.1 400 Bad Request\r\n\n";
            send(client_socket, response, sizeof(response), 0);
        } else {
            char file_url[100];

            get_file_url(route, file_url);

            FILE *file = fopen(file_url, "r");
            if (!file) {
                const char response[] = "HTTP/1.1 404 Not Found\r\n\n";
                send(client_socket, response, sizeof(response), 0);
            } else {
                char response_header[SIZE];
                char time_buffer[100];
                get_time_string(time_buffer);

                char mime_type[32];
                get_mime_type(file_url, mime_type);

                sprintf(response_header,
                        "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\n\n",
                        time_buffer, mime_type);
                int header_size = strlen(response_header);

                printf(" %s", mime_type);

                fseek(file, 0, SEEK_END);
                long fsize = ftell(file);
                fseek(file, 0, SEEK_SET);

                char *response_buffer = (char *)malloc(fsize + header_size);
                strcpy(response_buffer, response_header);

                char *file_buffer = response_buffer + header_size;
                fread(file_buffer, fsize, 1, file);

                send(client_socket, response_buffer, fsize + header_size, 0);
                free(response_buffer);
                fclose(file);
            }
        }
        close(client_socket);
        printf("\n");
    }
    exit(EXIT_SUCCESS);
}

void get_file_url(char *route, char *file_url) {
    char *question = strrchr(route, '?');
    if (question)
        *question = '\0';

    if (route[strlen(route) - 1] == '/') {
        strcat(route, "index.html");
    }

    strcpy(file_url, "htdocs");
    strcat(file_url, route);

    const char *dot = strrchr(file_url, '.');
    if (!dot || dot == file_url) {
        strcat(file_url, ".html");
    }

    return;
}

void get_mime_type(char *file, char *mime) {
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

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nShutting down server...\n");

        close(client_socket);
        close(server_socket);

        if (request != NULL)
            free(request);

        exit(0);
    }
    return;
}

void get_time_string(char *buffer) {
    time_t now = time(NULL);
    struct tm tm = *gmtime(&now);
    strftime(buffer, sizeof (buffer), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return;
}
