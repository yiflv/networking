#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#define ICMP_HDRLEN

struct icmp
{
    struct icmp6_hdr icmphdr;
    char   payload[10];
};

int main(int argc, const char *argv[])
{
    int sockfd;
    struct sockaddr_in6 dst;
    struct icmp icmppkt;
    struct ip6_hdr iphdr;
    uint8_t *ip_pkt;
    const int on = 1;

    ip_pkt = (uint8_t *)malloc(sizeof(uint8_t) * 100);
    if(ip_pkt < 0)
        perror("ipv6 packet create");
    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if(sockfd < 0)
        perror("socket create");

    // argv[1] src addr; argv[2] dst addr
    dst.sin6_family = AF_INET6;
    inet_pton(AF_INET6, argv[2], &dst.sin6_addr);
    

    // ipv6 header
    iphdr.ip6_flow = ((6 << 28) | (0 << 20) | 0);
    iphdr.ip6_plen = htons(0);
    iphdr.ip6_nxt = IPPROTO_ICMPV6;
    // icmpv6 header
    icmppkt.icmphdr.icmp6_type = ICMP6_ECHO_REQUEST;
    icmppkt.icmphdr.icmp6_code = 0;
    icmppkt.icmphdr.icmp6_id = 0;
    icmppkt.icmphdr.icmp6_seq = htons(0);
    icmppkt.icmphdr.icmp6_cksum = 0;
    strcpy(icmppkt.payload, "hello");
    if (setsockopt (sockfd, IPPROTO_IPV6, IP_HDRINCL, &on, sizeof (on)) < 0) {
        perror ("setsockopt() failed to set IP_HDRINCL ");
        exit (EXIT_FAILURE);
    }
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", "lo");


    // Bind socket to interface index.
    if (setsockopt (sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr)) < 0) {
        perror ("setsockopt() failed to bind to interface ");
        exit (EXIT_FAILURE);
    }
    int bytes = sendto(sockfd, ip_pkt, sizeof(ip_pkt), 0, (struct sockaddr *)&dst, sizeof(dst));
    if(bytes < 0)
        perror("send failed");
    return 1;
}
