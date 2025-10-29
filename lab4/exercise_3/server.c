#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

sem_t limit_sem; // semaphore to limit max threads

void error(char *m)
{
    perror(m);
}

void *handle_socket(void *newsockfd)
{
    int sock = *(int *)newsockfd;
    free(newsockfd);

    char buffer[256];
    int n;
    memset(buffer, 0, 256);
    n = read(sock, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");

    printf("Message received: %s\n", buffer);

    int num = 5 * atoi(buffer);
    sprintf(buffer, "%d", num);

    n = write(sock, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing back to socket");

    close(sock);

    // release semaphore slot after client finishes
    sem_post(&limit_sem);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
        error("ERROR, no port provided\n");

    port = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    // Initialize semaphore to allow max 3 clients
    sem_init(&limit_sem, 0, 3);

    socklen_t clilen = sizeof(cli_addr);

    printf("Server started. Waiting for clients...\n");

    while (1)
    {
        sem_wait(&limit_sem); // wait if already serving 3 clients
        int *newsock = malloc(sizeof(int));
        *newsock = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (*newsock < 0)
            error("ERROR on accept");

        pthread_t t;
        pthread_create(&t, NULL, handle_socket, newsock);
        pthread_detach(t);
    }

    close(sockfd);
    sem_destroy(&limit_sem);
    return 0;
}
