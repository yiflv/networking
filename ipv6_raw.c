#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#define ICMP_HDRLEN 8
#define IPV6_HDRLEN 40
#define PAYLOAD_LEN 7
#define IPV6_ADDR_LEN 16
struct icmp
{
    struct icmp6_hdr icmphdr;
    //char   payload[10];
    char payload[PAYLOAD_LEN];
};

uint16_t checksum(uint16_t *, int);
uint16_t icmpv6_checksum(struct ip6_hdr iphdr, struct icmp6_hdr icmphdr, uint8_t *payload, int payloadlen);
int main(int argc, const char *argv[])
{
    int sockfd;
    struct sockaddr_in6 dst;
    struct icmp icmppkt;
    struct ip6_hdr iphdr;
    uint8_t *ip_pkt;
    const int on = 1;

    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if(sockfd < 0)
        perror("socket create");

    // argv[1] src addr; argv[2] dst addr
    memset(&dst, 0, sizeof(dst));
    dst.sin6_family = AF_INET6;
    inet_pton(AF_INET6, argv[1], &dst.sin6_addr);


    // ipv6 header
    iphdr.ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
    iphdr.ip6_plen = htons(ICMP_HDRLEN + PAYLOAD_LEN);
    iphdr.ip6_nxt = IPPROTO_ICMPV6;
    iphdr.ip6_hops = 64;
    inet_pton(AF_INET6, argv[1], &iphdr.ip6_dst);
    inet_pton(AF_INET6, "::1", &iphdr.ip6_src);
    // icmpv6 header
    icmppkt.icmphdr.icmp6_type = ICMP6_ECHO_REQUEST;
    icmppkt.icmphdr.icmp6_code = 0;
    icmppkt.icmphdr.icmp6_id = htons(0x1234);
    icmppkt.icmphdr.icmp6_seq = htons(0x0);
    icmppkt.icmphdr.icmp6_cksum = 0;
    strcpy(icmppkt.payload, "123456");
    //icmppkt.payload = '123456';
    // checksum
    /*uint8_t *cksum;
    cksum = (uint8_t *)malloc(18);
    memcpy(cksum, &icmppkt.icmphdr, 8);
    memcpy(cksum + 8, &icmppkt.payload, 1);
    for(int i = 0; i< 9; i++)
    {
        printf("%02x ", *(cksum+i));
    }
    printf("\n");
    //icmppkt.icmphdr.icmp6_cksum = htons(checksum((uint16_t *)cksum, 9)); */
    icmppkt.icmphdr.icmp6_cksum = icmpv6_checksum(iphdr, icmppkt.icmphdr, &icmppkt.payload, PAYLOAD_LEN);
    
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
    ip_pkt = (uint8_t *)malloc(IPV6_HDRLEN + ICMP_HDRLEN + PAYLOAD_LEN);
    if(ip_pkt < 0)
        perror("ipv6 packet create");
    memcpy(ip_pkt, &iphdr, sizeof(iphdr));
    memcpy(ip_pkt + IPV6_HDRLEN, &icmppkt, ICMP_HDRLEN + PAYLOAD_LEN);
    int bytes = sendto(sockfd, ip_pkt, IPV6_HDRLEN + ICMP_HDRLEN + PAYLOAD_LEN, 0, (struct sockaddr *)&dst, sizeof(dst));
    if(bytes < 0)
        perror("send failed");
    return 1;
}

uint16_t checksum(uint16_t *addr, int size)
{
    uint32_t cksum = 0;
    while(size > 1)
    {
        printf("%4.4x ", *addr);
        cksum += *addr++;
        size -= 2;
    }
    if(size)
    {
        cksum += *(uint8_t *)addr;
        printf("%2x ", *addr);
    }
    while(cksum >> 16)
        cksum=(cksum >> 16)+(cksum & 0xffff);
    return (uint16_t)(~cksum);
}

uint16_t icmpv6_checksum(struct ip6_hdr iphdr, struct icmp6_hdr icmphdr, uint8_t *payload, int payloadlen)
{
    char buf[100];
    char *ptr;
    int chksumlen = 0;
    int i;

    ptr = &buf[0];
    // pseudo header
    memcpy(ptr, &iphdr.ip6_src.s6_addr, IPV6_ADDR_LEN);
    ptr += IPV6_ADDR_LEN;
    chksumlen += IPV6_ADDR_LEN; 
    memcpy(ptr, &iphdr.ip6_dst.s6_addr, IPV6_ADDR_LEN);
    ptr += IPV6_ADDR_LEN;
    chksumlen += IPV6_ADDR_LEN; 
    // Copy Upper Layer Packet length into buf (32 bits).
    // Should not be greater than 65535 (i.e., 2 bytes).
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = (ICMP_HDRLEN + payloadlen) / 256;
    ptr++;
    *ptr = (ICMP_HDRLEN + payloadlen) % 256;
    ptr++;
    chksumlen += 4;

    // Copy zero field to buf (24 bits)
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 3;

    // Copy next header field to buf (8 bits)
    memcpy (ptr, &iphdr.ip6_nxt, sizeof (iphdr.ip6_nxt));
    ptr += sizeof (iphdr.ip6_nxt);
    chksumlen += sizeof (iphdr.ip6_nxt);

    // Copy ICMPv6 
    memcpy(ptr, &icmphdr, ICMP_HDRLEN);
    chksumlen += ICMP_HDRLEN;
    ptr += ICMP_HDRLEN;
    memcpy(ptr, payload, payloadlen);
    chksumlen += PAYLOAD_LEN;
    checksum((uint16_t *)buf, chksumlen);
}