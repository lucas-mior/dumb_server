#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

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

static int server_socket;
static int client_socket;

int main(void) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    signal(SIGINT, handle_signal);
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

    {
        char host_name[NI_MAXHOST];
        char service_name[NI_MAXSERV];
        int error = getnameinfo((struct sockaddr *)&server_address,
                                sizeof (server_address),
                                host_name, sizeof (host_name),
                                service_name, sizeof (service_name), 0);

        if (error) {
            fprintf(stderr, "Error getting name info from %d: %s\n",
                            server_address.sin_addr.s_addr, gai_strerror(error));
            exit(EXIT_FAILURE);
        }

        printf("\nServer is listening on http://%s:%s ...\n",
               host_name, service_name);
    }

    while (true) {
        char request[SIZE];
        char method[10], route[100];

        client_socket = accept(server_socket, NULL, NULL);
        read(client_socket, request, SIZE);

        sscanf(request, "%s %s", method, route);
        printf("%s %s", method, route);

        if (strcmp(method, "GET") != 0) {
            const char response[] = "HTTP/1.1 400 Bad Request\r\n\n";
            send(client_socket, response, sizeof (response), 0);
        } else {
            int file;
            char file_url[PATH_MAX];

            get_file_url(route, file_url);
            printf("fileurl:%s\n", file_url);

            if ((file = open(file_url, O_RDWR)) < 0) {
                const char response[] = "HTTP/1.1 404 Not Found\r\n\n";
                send(client_socket, response, sizeof (response), 0);
            } else {
                char response_header[SIZE];
                char time_buffer[100];
                char mime_type[32];
                int header_size;
                int fsize;
                struct stat filestat;

                get_time_string(time_buffer);
                get_mime_type(file_url, mime_type);

                snprintf(response_header, sizeof (response_header),
                        "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\n\n",
                        time_buffer, mime_type);
                header_size = (int) strlen(response_header);

                printf(" %s", mime_type);

                if (fstat(file, &filestat) < 0) {
                    fprintf(stderr, "Error in fstat(): %s\n", strerror(errno));
                }
                fsize = filestat.st_size;
                printf("fsize: %ld\n", filestat.st_size);

                char *response_buffer = malloc(fsize + header_size);
                strcpy(response_buffer, response_header);

                char *file_buffer = response_buffer + header_size;
                int w = read(file, file_buffer, fsize);
                if (w < 0) {
                    printf("error reading: %s\n", strerror(errno));
                }

                send(client_socket, response_buffer, fsize + header_size, 0);
                free(response_buffer);
                close(file);
            }
        }
        close(client_socket);
        printf("\n");
    }
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
