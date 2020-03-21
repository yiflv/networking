#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <net/if.h>

#define ICMP_HDRLEN 8
#define IPV6_HDRLEN 40
#define PAYLOAD_LEN 7
#define IPV6_ADDR_LEN 16

uint16_t checksum(uint16_t *, int);
uint16_t icmpv6_checksum(struct ip6_hdr iphdr, struct icmp6_hdr icmphdr, uint8_t *payload, int payloadlen);
int main(int argc, const char *argv[])
{
    int sockfd;
    struct ip6_hdr pse_hdr;
    struct sockaddr_in6 dst;
    struct icmp6_hdr icmphdr;
    char payload[PAYLOAD_LEN];
    uint8_t *ip_pkt;
    int send_bytes, recv_bytes;

    sockfd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if(sockfd < 0)
        perror("socket create");
    memset(&dst, 0, sizeof(dst));
    dst.sin6_family = AF_INET6;
    // argv[1]  destination address
    inet_pton(AF_INET6, argv[1], &dst.sin6_addr);
    // icmpv6 header
    icmphdr.icmp6_type = ICMP6_ECHO_REQUEST;
    icmphdr.icmp6_code = 0;
    icmphdr.icmp6_id = htons(getpid());
    icmphdr.icmp6_seq = htons(0x1);
    icmphdr.icmp6_cksum = 0;
    strcpy(payload, "123456");
    // pseudo header
    pse_hdr.ip6_nxt = IPPROTO_ICMPV6;
    inet_pton(AF_INET6, argv[1], &pse_hdr.ip6_dst);
    inet_pton(AF_INET6, "::1", &pse_hdr.ip6_src);
    icmphdr.icmp6_cksum = icmpv6_checksum(pse_hdr, icmphdr, &payload, PAYLOAD_LEN); // Acutally the kernel will calculate this automatically
    ip_pkt = (uint8_t *)malloc(ICMP_HDRLEN + PAYLOAD_LEN);
    memcpy(ip_pkt, &icmphdr, ICMP_HDRLEN);
    memcpy(ip_pkt + ICMP_HDRLEN, &payload, PAYLOAD_LEN);
    // peer address
    struct sockaddr_in6 peer_addr;
    int addrlen = sizeof(peer_addr);
    uint8_t buf[100];
    while(1){
        send_bytes = sendto(sockfd, ip_pkt, ICMP_HDRLEN + PAYLOAD_LEN, 0, (struct sockaddr *)&dst, sizeof(dst));
        if(send_bytes < 0)
            perror("send failed");
        recv_bytes = recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr, &addrlen);
        if(recv_bytes < 0)
            perror("receive failed");
        printf("Receive %d bytes\n", recv_bytes);
        sleep(2);
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