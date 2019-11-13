#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main(int argc, char **argv)
{
	int cfd;
	int recbyte;
	int sin_size;
	char buffer[1024] = {0};

	struct sockaddr_in s_add, c_add;
	unsigned short portnum = 0x8888;

	if(argc != 2)
	{
		printf("usage: echo ip\n");
		return -1;
	}

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(cfd == -1)
	{
		printf("Socket failed!\r\n");
		return -1;
	}
	else
		printf("Socket OK!\r\n");

	bzero(&s_add, sizeof(struct sockaddr_in));
	s_add.sin_family = AF_INET;
	s_add.sin_addr.s_addr = inet_addr(argv[1]);
	s_add.sin_port = htons(portnum);
	printf("s_addr = %#x, port: %#x\r\n", s_add.sin_addr.s_addr, s_add.sin_port);

	if(connect(cfd, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)) == -1)
	{
		printf("Connection failed!\r\n");
		return -1;
	}
	else 
		printf("Connection success!\r\n");

	while(1)
	{
		if((recbyte = read(cfd, buffer, 1024)) == -1)
		{
			printf("read failed!\r\n");
			return -1;
		}

		printf("read ok\r\nREC:\r\n");
		buffer[recbyte] = '\0';
		printf("%s\r\n", buffer);
	}

	close(cfd);
	return 0;
}

