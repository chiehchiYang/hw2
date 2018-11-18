#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{

	//socket的建立
	int sockfd = 0;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1)
	{
		printf("Fail to create a socket.");
	}

	//socket的連線

	struct sockaddr_in info;
	//bzero(&info, sizeof(info));
	memset(&info, 0, sizeof(info));
	info.sin_family = PF_INET;

	//localhost test
	info.sin_addr.s_addr = inet_addr("127.0.0.1");
	info.sin_port = htons(1234);

	int err = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
	if (err == -1)
	{
		printf("Connection error");
	}

	char *QUERY_FILE_ORDIR;
	QUERY_FILE_ORDIR = argv[2]; //argv[2];
	char *LOCALHOST;
	LOCALHOST = argv[4];
	char *PORT = argv[6];
	char *Space = " ";
	char *HTTP = "HTTP/1.x\r\nHOST: ";
	char *se = ":";
	char *End = "\r\n\r\n";
	//memset(QUERY_FILE_ORDIR, 0, sizeof(QUERY_FILE_ORDIR));
	//QUERY_FILE_ORDIR = argv[2];

	char message[1000]; //{"Get QUERY_FILE_ORDIR HTTP/1.x\r\nHOST:"};
	memset(message, 0, sizeof(message));
	strcpy(message, "GET ");
	//printf("%s", message);
	strcat(message, QUERY_FILE_ORDIR);
	strcat(message, Space);
	strcat(message, HTTP);
	strcat(message, LOCALHOST);
	strcat(message, se);
	strcat(message, PORT);
	strcat(message, End);
	//printf("%s", message);

	char receiveMessage[1000] = {};
	//printf("%s\n", message);
	send(sockfd, message, sizeof(message), 0);

	printf("sucess\n");
	recv(sockfd, receiveMessage, 1000, 0);
	printf("\n");
	printf("%s\n", receiveMessage);
	printf("close Socket\n");
	close(sockfd);
	return 0;
}