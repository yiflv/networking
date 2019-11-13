#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#define IPV6_HDRLEN 40
#define RT_HDRLEN   24
#define ICMP_HDRLEN 8
#define PAYLOAD_LEN 10
#define TOTAL_LEN   IPV6_HDRLEN + RT_HDRLEN + ICMP_HDRLEN + PAYLOAD_LEN

struct ip6_rthdr_0
  {
    uint8_t  ip6r0_nxt;		/* next header */
    uint8_t  ip6r0_len;		/* length in units of 8 octets */
    uint8_t  ip6r0_type;	/* always zero */
    uint8_t  ip6r0_segleft;	/* segments left */
    uint8_t  ip6r0_reserved;	/* reserved field */
    uint8_t  ip6r0_slmap[3];	/* strict/loose bit map */
    /* followed by up to 127 struct in6_addr */
    struct in6_addr ip6r0_addr;
  };

// argv[1] src addr, argv[2] dst addr
int main(int argc, const char *argv[])
{
    int sockfd;
    struct sockaddr_in6 dst;
    struct ip6_hdr iphdr;
    struct ip6_rthdr0 rthdr;
    struct icmp6_hdr icmphdr;
    char   payload[10];
    const int on = 1;

    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if(sockfd < 0){
        perror("socket create");
        return -1;
    }
    
    memset(&dst, 0, sizeof(dst));
    //dst.sin6_flowinfo = htonl(0);
    dst.sin6_family = AF_INET6;
    //dst.sin6_port = 0;
    inet_pton(AF_INET6, argv[2], &dst.sin6_addr);

    // ipv6 header
    iphdr.ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
    iphdr.ip6_plen = htons(0);
    iphdr.ip6_nxt = IPPROTO_ROUTING;
    iphdr.ip6_hops = 64;
    iphdr.ip6_plen = htons(TOTAL_LEN - IPV6_HDRLEN);
    inet_pton(AF_INET6, argv[2], &iphdr.ip6_dst);
    inet_pton(AF_INET6, argv[1], &iphdr.ip6_src);
    // routing header
    rthdr.ip6r0_nxt = IPPROTO_ICMPV6;	
    rthdr.ip6r0_len = 2;	
    rthdr.ip6r0_type = 0;
    rthdr.ip6r0_segleft = 1;
    inet_pton(AF_INET6, "::2", &rthdr.ip6r0_addr);
    // icmpv6 header
    icmphdr.icmp6_type = ICMP6_ECHO_REQUEST;
    icmphdr.icmp6_code = 0;
    icmphdr.icmp6_id = 0x2345;
    icmphdr.icmp6_seq = htons(0x0001);
    icmphdr.icmp6_cksum = 0x1234;
    strcpy(payload, "hello1234");
    // tell kernel not to include ip header
    if (setsockopt (sockfd, IPPROTO_IPV6, IP_HDRINCL, &on, sizeof (on)) < 0) {
        perror ("setsockopt() failed to set IP_HDRINCL ");
        exit (EXIT_FAILURE);
    }
    // bind to interface lo
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", "lo");

    // Bind socket to interface index.
    if (setsockopt (sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr)) < 0) {
        perror ("setsockopt() failed to bind to interface ");
        exit (EXIT_FAILURE);
    }
    // IPv6 packet ptr
    uint8_t *ip_pkt = (uint8_t *)malloc(sizeof(uint8_t)*TOTAL_LEN);
    if(ip_pkt < 0)
        perror("ipv6 packet create");
    // Construct IPv6 datagram
    memcpy(ip_pkt, &iphdr, sizeof(iphdr));
    memcpy(ip_pkt + IPV6_HDRLEN, &rthdr, sizeof(rthdr));
    memcpy(ip_pkt + IPV6_HDRLEN + RT_HDRLEN, &icmphdr, sizeof(icmphdr));
    memcpy(ip_pkt + IPV6_HDRLEN + RT_HDRLEN + ICMP_HDRLEN, &payload, sizeof(payload));
    // Send the datagram
    int bytes = sendto(sockfd, ip_pkt, TOTAL_LEN, 0, (struct sockaddr *)&dst, sizeof(dst));
    if(bytes < 0){
        perror("send failed");
        return -1;
    }
    return 1;
}
