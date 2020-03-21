#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main()
{
	int sfp, nfp, num = 0;
	struct sockaddr_in s_add, c_add;
	int sin_size;
	unsigned short portnum = 0x8888;
	char buffer[100] = {0};

	sfp = socket(AF_INET, SOCK_STREAM, 0);
	if(sfp == -1)
	{
		printf("Socket failed!\r\n");
		return -1;
	}

	printf("Socket OK!\r\n");

	bzero(&s_add, sizeof(struct sockaddr_in));
	s_add.sin_family = AF_INET;
	s_add.sin_addr.s_addr = htonl(INADDR_ANY);
	s_add.sin_port = htons(portnum);

	if(bind(sfp, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)) == -1)
	{
		printf("Bind failed!\r\n");
		return -1;
	}
	else
		printf("Bind OK!\r\n");
	if (listen(sfp, 5) == -1)
	{
		printf("Listen failed!\r\n");
		return -1;
	}
	else
		printf("Listen OK!\r\n");

	sin_size = sizeof(struct sockaddr_in);

	nfp = accept(sfp, (struct sockaddr *)(&c_add), &sin_size);
	if(nfp == -1)
	{
		printf("Accept failed!\r\n");
		return -1;
	}
	else
		printf("Accept OK!\r\nServer gets connection from %#x : %#x\r\n", ntohl(c_add.sin_addr.s_addr), ntohs(c_add.sin_port));

	while(1)
	{
		memset(buffer, 0, 100);
		sprintf(buffer, "Hello, welcome to my server(%d)\r\n", num++);
		send(nfp, buffer, strlen(buffer), 0);
		usleep(5000000);
	}

	close(nfp);
	close(sfp);

	return 0;
}

