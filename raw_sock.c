#include "raw_sock.h"

static uint16_t checksum(uint16_t *buf, int size)
{
    uint32_t cksum = 0;
    while(size > 1)
    {
        cksum += *buf++;
        size -= 2;
    }
    if(size)
    {
        cksum += *(uint8_t *)buf;
        printf("%2x ", *buf);
    }
    while(cksum >> 16)
        cksum=(cksum >> 16)+(cksum & 0xffff);
    return (uint16_t)(~cksum);
}

int ether_frame_send(char *if_name, uint8_t *payload, int pay_len)
{
    uint8_t src_mac[6];
    uint8_t dst_mac[6];
    struct sockaddr_ll device;
    struct ifreq ifr;

    memset(&device, 0, sizeof(device));
    // Get the MAC address
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", if_name);
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(ioctl(fd, SIOCGIFHWADDR, &ifr) < 0){
        perror("ioctl() failed to get the MAC address");
        return -1;
    }
    close(fd);
    memcpy(&src_mac, ifr.ifr_hwaddr.sa_data, 6);
    // Set destination MAC to 0
    memset(dst_mac, 0, sizeof(dst_mac));

	int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IPV6));
	if(sockfd < 0)
	{
		perror("socket create");
		return -1;
	}
    
    if((device.sll_ifindex = if_nametoindex(if_name)) == 0){
        perror("device");
        exit(EXIT_FAILURE);
    }

    device.sll_family = AF_PACKET;
    device.sll_halen = 6;
    memcpy(device.sll_addr, src_mac, 6);
    // Create and fill the ethernet frame
    int frame_len = pay_len + ETHER_HDR_LEN; 
	uint8_t *ether_frame = (uint8_t *)malloc(frame_len);
    memcpy(ether_frame, dst_mac, 6);
    memcpy(ether_frame + 6, src_mac, 6);
    ether_frame[12] = ETH_P_IPV6 / 256;
    ether_frame[13] = ETH_P_IPV6 % 256;
    memcpy(ether_frame + ETHER_HDR_LEN, payload, pay_len);
	
	int bytes = sendto(sockfd, ether_frame, frame_len, 0, (struct sockaddr *)&device, sizeof(device));
	if(bytes < 0)
	{
		perror("Send failed");
		return -1;
	}
    close(sockfd);
	return 1;
}

int icmp_send(char *ifname, struct ip6_hdr *ip6hdr, struct icmp6_hdr *icmp6hdr, uint8_t *icmp_pay, int pay_len)
{
    uint8_t chksum_buf[ICMP_MAXLEN];
    uint8_t ip_pkt[ICMP_MAXLEN];
    uint8_t *ptr = chksum_buf;
    int chksumlen = 0;
    // Pseudo header
    memcpy(ptr, &ip6hdr->ip6_src.s6_addr, IP6_ADDR_LEN);
    ptr += IP6_ADDR_LEN;
    chksumlen += IP6_ADDR_LEN; 
    memcpy(ptr, &ip6hdr->ip6_dst.s6_addr, IP6_ADDR_LEN);
    ptr += IP6_ADDR_LEN;
    chksumlen += IP6_ADDR_LEN; 
	// Copy Upper Layer Packet length into buf (32 bits).
    // Should not be greater than 65535 (i.e., 2 bytes).
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = (ICMP_HDR_LEN + pay_len) / 256;
    ptr++;
    *ptr = (ICMP_HDR_LEN + pay_len) % 256;
    ptr++;
    chksumlen += 4;
    // Copy zero field to buf (24 bits)
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 3;
    // Copy next header field to buf (8 bits)
    memcpy (ptr, &ip6hdr->ip6_nxt, sizeof (ip6hdr->ip6_nxt));
    ptr += sizeof (ip6hdr->ip6_nxt);
    chksumlen += sizeof (ip6hdr->ip6_nxt);
    // Copy ICMP header
    memcpy(ptr, icmp6hdr, ICMP_HDR_LEN);
    chksumlen += ICMP_HDR_LEN;
    ptr += ICMP_HDR_LEN;
    memcpy(ptr, icmp_pay, pay_len);
    chksumlen += pay_len;
    icmp6hdr->icmp6_cksum = checksum((uint16_t *)chksum_buf, chksumlen);
    memcpy(ip_pkt, ip6hdr, IP6_HDR_LEN);
    memcpy(ip_pkt + IP6_HDR_LEN, icmp6hdr, ICMP_HDR_LEN);
    memcpy(ip_pkt + IP6_HDR_LEN + ICMP_HDR_LEN, icmp_pay, pay_len);
    return ether_frame_send(ifname, ip_pkt, IP6_HDR_LEN + ICMP_HDR_LEN + pay_len);
}

