/* This file is part of dumb_server.
 * Copyright (C) 2024 Lucas Mior

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

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

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

static void handle_signal(int signal);

static int server_socket;
static int client_socket;

int main(void) {
    struct sockaddr_in server_address;
    int binded;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    signal(SIGINT, handle_signal);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
               &(int){1}, sizeof (int));

    binded = bind(server_socket,
                  (struct sockaddr *)&server_address,
                  sizeof (server_address));
    if (binded < 0) {
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
                            server_address.sin_addr.s_addr,
                            gai_strerror(error));
            exit(EXIT_FAILURE);
        }

        printf("\nServer is listening on http://%s:%s ...\n",
               host_name, service_name);
    }

    while (true) {
        char request[SIZE];
        char method[10], route[100];
        int file;
        char file_url[PATH_MAX];
        char response_header[SIZE];
        char *response_buffer = NULL;
        char *pointer = NULL;
        char time_buffer[128];
        char *mime;
        int header_size;
        uint fsize;

        {
            ssize_t r;
            client_socket = accept(server_socket, NULL, NULL);
            if ((r = read(client_socket, request, SIZE)) < 0) {
                fprintf(stderr, "Error reading from client: %s\n",
                                strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (r == 0) {
                fprintf(stderr, "Read nothing from client. Exiting...\n");
                exit(EXIT_FAILURE);
            }
        }

        sscanf(request, "%s %s", method, route);
        printf("%s %s", method, route);

        if (strcmp(method, "GET") != 0) {
            const char response[] = "HTTP/1.1 400 Bad Request\r\n\n";
            send(client_socket, response, sizeof (response), 0);
            goto close_socket;
        }

        {
            char *dot;
            char *question;

            question = strrchr(route, '?');
            if (question)
                *question = '\0';

            if (route[strlen(route) - 1] == '/') {
                strcat(route, "index.html");
            }

            strcpy(file_url, "content");
            strcat(file_url, route);

            dot = strrchr(file_url, '.');
            if (!dot || dot == file_url) {
                strcat(file_url, ".html");
            }
        }

        if ((file = open(file_url, O_RDWR)) < 0) {
            const char response[] = "HTTP/1.1 404 Not Found\r\n\n";
            send(client_socket, response, sizeof (response), 0);
            goto close_socket;
        }

        {
            time_t now = time(NULL);
            struct tm tm = *gmtime(&now);
            strftime(time_buffer, sizeof (time_buffer),
                     "%a, %d %b %Y %H:%M:%S %Z", &tm);
        }

        {
            const char *dot = strrchr(file_url, '.');

            if (dot == NULL)
                mime = "text/html";
            else if (strcmp(dot, ".html") == 0)
                mime = "text/html";
            else if (strcmp(dot, ".css") == 0)
                mime = "text/css";
            else if (strcmp(dot, ".js") == 0)
                mime = "application/js";
            else if (strcmp(dot, ".jpg") == 0)
                mime = "image/jpeg";
            else if (strcmp(dot, ".png") == 0)
                mime = "image/png";
            else if (strcmp(dot, ".gif") == 0)
                mime = "image/gif";
            else
                mime = "text/html";
        }

        header_size = snprintf(response_header, sizeof (response_header),
                               "HTTP/1.1 200 OK\r\n"
                               "Date: %s\r\nContent-Type: %s\r\n\n",
                               time_buffer, mime);
        if (header_size < 0) {
            fprintf(stderr, "Error in snprintf.\n");
            exit(EXIT_FAILURE);
        }

        printf(" %s", mime);

        {
            struct stat filestat;
            if (fstat(file, &filestat) < 0) {
                fprintf(stderr, "Error in fstat(): %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (filestat.st_size > UINT_MAX) {
                fprintf(stderr, "File size is too large:\n");
                fprintf(stderr, "%s: %zu", file_url, filestat.st_size);
                exit(EXIT_FAILURE);
            }
            if (filestat.st_size <= 0) {
                fprintf(stderr, "File size is zero:\n");
                fprintf(stderr, "%s: %zu", file_url, filestat.st_size);
                exit(EXIT_FAILURE);
            }
            fsize = (uint) filestat.st_size;
        }

        response_buffer = malloc(fsize + (uint) header_size);
        strcpy(response_buffer, response_header);

        pointer = response_buffer + header_size;

        {
            ssize_t r = read(file, pointer, fsize);
            if (r < 0) {
                fprintf(stderr, "Error reading %s: %s\n",
                                file_url, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        if (send(client_socket, response_buffer,
                 fsize + (uint) header_size, 0) < 0) {
            fprintf(stderr, "Error sending response: %s\n",
                            strerror(errno));
            exit(EXIT_FAILURE);
        }
        free(response_buffer);
        close(file);
close_socket:
        close(client_socket);
        printf("\n");
    }
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
