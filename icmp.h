#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>

#define MAGIC_LEN   11
#define MAGIC       "hello"
#define MTU         1500
#define RECV_TIMEOUT_USEC 100000

struct icmp_echo
{
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    ident;
    uint16_t    seq;
    double      sending_ts;
    char        magic[MAGIC_LEN];
};
