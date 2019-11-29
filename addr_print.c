#include <stdio.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdarg.h>

#define PRINT(...) printf(__VA_ARGS__)
#define PRINT_ADDR(addr) do{    \
                            char a[128]; \
                            inet_ntop(AF_INET6, addr, a, 128); \
                            printf("%s\n", a); \
                            }while(0) 

char a[128];
struct in6_addr addr;

int addr_print(char *buf, struct in6_addr *addr)
{
    int ret = inet_ntop(AF_INET6, addr, buf, sizeof(a));
    if(ret == 0)
        perror("ntop");
    printf("%s\n", buf);
}
int main()
{
    addr.__in6_u.__u6_addr16[0] = 0x80fe; 
    for(int i = 1; i < 8; i++)
    {
        addr.__in6_u.__u6_addr16[i] = 0x3412;
    }
    //addr_print(a, &addr);
    PRINT_ADDR(&addr);
    PRINT("Hello %s\n", "yifan");
    return 1;
}