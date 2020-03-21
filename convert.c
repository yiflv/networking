#include <stdio.h>
#include <arpa/inet.h>

int
main(int argc, const char *argv[])
{
    char buf[33];
    int res = inet_pton(AF_INET, argv[1], buf);
    if(!res)
        perror("inet_pton");
    char buf2[20];
    inet_ntop(AF_INET, buf, buf2, sizeof(buf));
    printf("%s\n",buf2);
    return 1;
}