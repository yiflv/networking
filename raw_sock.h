#ifndef RAW_SOCK_H
#define RAW_SOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <sys/ioctl.h>
#include <bits/ioctls.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <unistd.h>

#define MTU             1500
#define ICMP_MAXLEN     200
#define IP6_HDR_LEN     40
#define ICMP_HDR_LEN    4
#define IP6_ADDR_LEN    16

int icmp_send(char *ifname, struct ip6_hdr *ip6hdr, struct icmp6_hdr *icmp6hdr, uint8_t *icmp_pay, int pay_len);

#endif