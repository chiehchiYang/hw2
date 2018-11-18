#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include "status.h"
#include <arpa/inet.h>
#include <dirent.h>

char inputBuffer[1000] = {};
char message[1000] = {};
struct node
{
    int socket_fd;
    struct node *next;
};
struct node *Q_head = NULL;
struct node *Q_tail = NULL;
int Q_num = 0;

void push(int data)
{
    if (Q_head == NULL)
    {
        Q_head = (struct node *)malloc(sizeof(struct node));
        Q_head->socket_fd = data;
        Q_head->next = NULL;
        Q_tail = Q_head;
    }

    {
        struct node *ptr = (struct node *)malloc(sizeof(struct node));
        ptr->socket_fd = data;
        ptr->next = NULL;
        Q_tail->next = ptr;
        Q_tail = ptr;
    }
    Q_num++;
}
int pop()
{
    struct node *ptr = Q_head;
    int res = ptr->socket_fd;
    Q_head = ptr->next;
    free(ptr);
    Q_num--;
    return res;
}
int isEmpty()
{
    if (Q_num == 0)
        return 1;
    else
        return 0;
}

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_lock;

void *threadpool_thread(void *args)
{
    struct stat sb;
    char *pathname = malloc(100);
    char *method = malloc(100);
    char *fullpath = malloc(100);
    char *pathnametype = malloc(50);
    char *tem = malloc(50);
    //int id = *(int*)args;
    int client_socket_fd;
    int count;
    char *delim = " ";

    char *pch1;
    char *delim1 = ".";
    char *delim2 = "0";
    char *delim3 = "GET";
    char *delim4 = "/";

    while (1)
    {

        pthread_mutex_lock(&lock);
        while (isEmpty())
        {
            pthread_cond_wait(&cond_lock, &lock);
        }
        client_socket_fd = pop();
        memset(inputBuffer, 0, sizeof(inputBuffer));

        recv(client_socket_fd, inputBuffer, sizeof(inputBuffer), 0);

        //get the content type
        count = 0;

        for (char *pch = strtok(inputBuffer, delim); pch; pch = strtok(NULL, delim))
        {
            if (count == 0)
                strcpy(method, pch);
            if (count == 1)
                strcpy(pathname, pch);
            count++;
        }
        char *temforcount = malloc(50);
        strcpy(temforcount, pathname);
        printf("%s\n", temforcount);
        int counter; //count how many "/"
        char *filename = malloc(50);

        for (char *pch = strtok(temforcount, delim4); pch; pch = strtok(NULL, delim4))
        {
            counter++;
            strcpy(filename, pch);
        }

        printf("count---->%d\n", counter);

        sprintf(filename, "testdir/%s", filename);

        if ((strcmp(pathname, "/testdir") == 0) || (strcmp(pathname, "/testdir/") == 0))
        {
            strcpy(pathname, "testdir");
        }
        else if (pathname[0] == '/')
        {
            ;
        }
        else
        {
            sprintf(pathname, "testdir%s", pathname);
        }
        printf("%s\n", pathname);
        if (counter > 1)
        {
            if (stat(filename, &sb) == 0 && S_ISREG(sb.st_mode))
                printf("sucess!!!!!!");
        }
        else if (strcmp(method, delim3) != 0)
        {
            sprintf(message, "HTTP/1.x %d METHOD_NOT_ALLOWED\r\nContent-Type:\r\nServer: httpserver/1.x\r\n\r\n", status_code[METHOD_NOT_ALLOWED]);
            send(client_socket_fd, message, sizeof(message), 0);
        }
        else if (stat(pathname, &sb) == 0 && S_ISDIR(sb.st_mode))
        {
            printf("1 \n");
            char *tempath = malloc(50);

            sprintf(tempath, "./%s", pathname);
            DIR *d;
            struct dirent *dir;
            printf("%s\n", tempath);
            d = opendir("./testdir");
            sprintf(message, "HTTP/1.x %d OK\r\nContent-Type: directory\r\nServer: httpserver/1.x\r\n\r\n", status_code[OK]);

            if (d)
            {
                while ((dir = readdir(d)) != NULL)
                {
                    if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, ".."))
                    {

                        strcat(tem, dir->d_name);
                        strcat(tem, delim);
                    }
                }
                closedir(d);
            }
            sprintf(message, "%s%s", message, tem);

            send(client_socket_fd, message, sizeof(message), 0);
            //Directory
        }
        else if (stat(pathname, &sb) == 0 && S_ISREG(sb.st_mode))
        {
            printf("2 \n");
            printf("%s\n", pathname);

            char *tempath = malloc(50);
            strcpy(tempath, pathname);

            pch1 = strtok(pathname, delim1);
            pch1 = strtok(NULL, delim1);
            printf("%s\n", pch1);
            strcpy(pathnametype, "0");
            for (int i = 0; i < 8; i++)
            {
                if (strcmp(pch1, extensions[i].ext) == 0)
                {
                    strcpy(pathnametype, extensions[i].mime_type);
                }
            }
            if (strcmp(pathnametype, delim2) == 0)
            {

                sprintf(message, "HTTP/1.x %d UNSUPPORT_MEDIA_TYPE\r\nContent-Type:\r\nServer: httpserver/1.x\r\n\r\n", status_code[UNSUPPORT_MEDIA_TYPE]);
            }
            else
            {
                memset(message, 0, sizeof(message));
                //readfile
                char buffer[2000];
                char ch;
                int i = 0;
                FILE *file;
                file = fopen(tempath, "r");
                while ((ch = fgetc(file)) != EOF)
                {
                    buffer[i] = ch;
                    i++;
                }
                fclose(file);

                sprintf(message, "HTTP/1.x %d OK\r\nContent-Type: %s\r\nServer: httpserver/1.x\r\n\r\n%s", status_code[OK], pathnametype, buffer);

                //File
            }
            send(client_socket_fd, message, sizeof(message), 0);
        }
        else if (stat(pathname, &sb) != 0)
        {
            char *test = pathname;
            pch1 = strtok(test, delim1);
            pch1 = strtok(NULL, delim1);
            printf("%s\n", pch1);
            strcpy(pathnametype, "0");
            for (int i = 0; i < 8; i++)
            {
                if (strcmp(pch1, extensions[i].ext) == 0)
                {
                    strcpy(pathnametype, extensions[i].mime_type);
                }
            }

            printf("3\n");
            printf("%s", pathname);
            if (pathname[0] != '/')
            {
                sprintf(message, "HTTP/1.x %d BAD_REQUEST\r\nContent-Type:\r\nServer: httpserver/1.x\r\n\r\n", status_code[BAD_REQUEST], pathnametype);
            }
            else if (strcmp(pathnametype, delim2) == 0)
            {

                sprintf(message, "HTTP/1.x %d UNSUPPORT_MEDIA_TYPE\r\nContent-Type:\r\nServer: httpserver/1.x\r\n\r\n", status_code[UNSUPPORT_MEDIA_TYPE]);
            }
            else
            {
                sprintf(message, "HTTP/1.x %d NOT_FOUND\r\nContent-Type:\r\nServer: httpserver/1.x\r\n\r\n", status_code[NOT_FOUND], pathnametype);
            }
            send(client_socket_fd, message, sizeof(message), 0);
            //not found
        }

        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, char *argv[])
{
    int sockfd = 0, forClientSockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        printf("Fail to create a socket.\n");
    }

    //socket的連線
    struct sockaddr_in serverInfo, clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    memset(&serverInfo, 0, sizeof(serverInfo));
    //bzero(&serverInfo, sizeof(serverInfo));

    serverInfo.sin_family = PF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(1234);
    bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo));
    listen(sockfd, 5);
    int thread_count = 10;

    pthread_t *thread_pool = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);

    for (int i = 0; i < thread_count; i++)
    {
        pthread_create(&(thread_pool[i]), NULL, threadpool_thread, NULL);
        usleep(100);
    }

    while (1)
    {
        forClientSockfd = 0;
        forClientSockfd = accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen);
        printf("receive:%d\n", forClientSockfd);
        pthread_mutex_lock(&lock);
        push(forClientSockfd);
        pthread_cond_signal(&cond_lock);
        pthread_mutex_unlock(&lock);
    }
    pthread_join(*thread_pool, NULL);
    return 0;
}