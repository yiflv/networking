#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#define RT_HDRLEN   24
#define ICMP_HDRLEN 8
#define PAYLOAD_LEN 10
#define TOTAL_LEN   RT_HDRLEN + ICMP_HDRLEN + PAYLOAD_LEN
#define IPV6_ADDR_LEN 16

uint16_t icmpv6_checksum(struct ip6_hdr iphdr, struct icmp6_hdr icmphdr, uint8_t *payload, int payloadlen);

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

// argv[1] dst addr
int main(int argc, const char *argv[])
{
    int sockfd;
    struct sockaddr_in6 dst;
    struct ip6_hdr pse_hdr;
    struct ip6_rthdr_0 rthdr;
    struct icmp6_hdr icmphdr;
    char   payload[10];
    const int on = 1;

    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ROUTING);
    if(sockfd < 0){
        perror("socket create");
        return -1;
    }
    
    memset(&dst, 0, sizeof(dst));
    dst.sin6_family = AF_INET6;
    inet_pton(AF_INET6, argv[1], &dst.sin6_addr);
    // pseudo ipv6 header
    inet_pton(AF_INET6, argv[1], &pse_hdr.ip6_dst);
    inet_pton(AF_INET6, "::1", &pse_hdr.ip6_src);
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
    icmphdr.icmp6_cksum = 0;
    strcpy(payload, "hello1234");
    icmphdr.icmp6_cksum = icmpv6_checksum(pse_hdr, icmphdr, &payload, PAYLOAD_LEN);
    // IPv6 packet ptr
    uint8_t *ip_pkt = (uint8_t *)malloc(sizeof(uint8_t)*TOTAL_LEN);
    if(ip_pkt < 0)
        perror("ipv6 packet create");
    // Construct IPv6 datagram
    memcpy(ip_pkt, &rthdr, sizeof(rthdr));
    memcpy(ip_pkt + RT_HDRLEN, &icmphdr, sizeof(icmphdr));
    memcpy(ip_pkt + RT_HDRLEN + ICMP_HDRLEN, payload, sizeof(payload));
    // Send the datagram
    int bytes = sendto(sockfd, ip_pkt, TOTAL_LEN, 0, (struct sockaddr *)&dst, sizeof(dst));
    if(bytes < 0){
        perror("send failed");
        return -1;
    }
    return 1;
}

uint16_t checksum(uint16_t *addr, int size)
{
    uint32_t cksum = 0;
    while(size > 1)
    {
        //printf("%4.4x ", *addr);
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