#include "raw_sock.h"
#define ICMP6_RPL                   155
#define RPL_DIS                     0x00
#define RPL_DIO                     0x01
#define RPL_DAO                     0x02
#define RPL_DAO_ACK                 0x03

int echo_req_send(char *src, char *dst)
{
    struct ip6_hdr ip6hdr;
    struct icmp6_hdr icmp6hdr; 
    // IP header
    ip6hdr.ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
    ip6hdr.ip6_plen = htons(ICMP_HDR_LEN + 4);
    ip6hdr.ip6_nxt = IPPROTO_ICMPV6;
    ip6hdr.ip6_hops = 255;
    inet_pton(AF_INET6, src, &ip6hdr.ip6_src);
    inet_pton(AF_INET6, dst, &ip6hdr.ip6_dst);
	// ICMP header
	icmp6hdr.icmp6_type = ICMP6_ECHO_REQUEST; 
	icmp6hdr.icmp6_code = 0;
	icmp6hdr.icmp6_cksum = 0;
    uint16_t pay[2] = {0, 0};
    return icmp_send("lo", &ip6hdr, &icmp6hdr, (uint8_t *)pay, 4);
}

int icmp_rpl_send(char *src_addr, char *dst_addr,int code, uint8_t *opt, int len)
{
    struct ip6_hdr ip6hdr;
    struct icmp6_hdr icmp6hdr; 
    // IP header
    ip6hdr.ip6_flow = htonl((6 << 28) | (0 << 20) | 0);
    ip6hdr.ip6_plen = htons(ICMP_HDR_LEN + len);
    ip6hdr.ip6_nxt = IPPROTO_ICMPV6;
    ip6hdr.ip6_hops = 255;
    inet_pton(AF_INET6, src_addr, &ip6hdr.ip6_src);
    inet_pton(AF_INET6, dst_addr, &ip6hdr.ip6_dst);
	// ICMP header
	icmp6hdr.icmp6_type = ICMP6_RPL; 
	icmp6hdr.icmp6_code = code;
	icmp6hdr.icmp6_cksum = 0;
    return icmp_send("lo", &ip6hdr, &icmp6hdr, opt, len);
}

int main()
{
    char *src_addr = "2001:beef::1";
    char *dst_addr = "2001:beef::2";
    echo_req_send(src_addr, dst_addr);
    icmp_rpl_send(src_addr, dst_addr, RPL_DIO, NULL, 0);
}