#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

void error(char *m)
{
    perror(m);
    exit(0);
}

void connect_and_send(char *hostname, int port)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL)
        error("ERROR, no such host\n");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    srand(time(NULL) ^ (getpid() << 16));
    int r = rand() % 100;
    sprintf(buffer, "%d", r);
    printf("[PID %d] Sending: %s\n", getpid(), buffer);

    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");

    memset(buffer, 0, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");

    printf("[PID %d] Received: %s\n", getpid(), buffer);
    close(sockfd);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
        error("usage client [hostname] [port]\n");

    char *hostname = argv[1];
    int port = atoi(argv[2]);

    // fork up to 3 child processes
    for (int i = 0; i < 3; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // child process
            connect_and_send(hostname, port);
            exit(0);
        }
        else if (pid < 0)
        {
            error("ERROR on fork");
        }
    }

    // parent waits for all children
    for (int i = 0; i < 3; i++)
        wait(NULL);

    printf("All clients finished.\n");
    return 0;
}
