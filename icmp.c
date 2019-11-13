#include "icmp.h"

double get_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

uint16_t calculate_checksum(unsigned char* buffer, int bytes)
{
    uint32_t    checksum = 0;
    unsigned char *end = buffer + bytes;

    if(bytes % 2 ==1)
    {
        end = buffer + bytes -1;
        checksum += (*end) << 8;
    }

    while(buffer < end)
    {
        checksum += buffer[0] << 8;
        checksum += buffer[1];
        buffer +=2;
    }

    uint32_t carray = checksum >> 16;
    while(carray)
    {
        checksum = (checksum & 0xffff) + carray;
        carray = checksum >> 16;
    }

    checksum = ~checksum;
    return checksum & 0xffff;
}

int send_echo_request(int sock, struct sockaddr_in *addr, int ident, int seq )
{
    struct icmp_echo icmp;
    bzero(&icmp, sizeof(icmp));
    icmp.type = 8;
    icmp.code = 0;
    icmp.ident = htons(ident);
    icmp.seq = htons(seq);
    strncpy(icmp.magic, MAGIC, MAGIC_LEN);
    icmp.sending_ts = get_timestamp();
    icmp.checksum = htons(calculate_checksum((unsigned char *)&icmp, sizeof(icmp)));

    int bytes = sendto(sock, &icmp, sizeof(icmp), 0, (struct sockaddr *)addr, sizeof(*addr));
    if (bytes == -1)
        return -1;

    return 0;
}

int recv_echo_reply(int sock, int ident)
{
    char buffer[MTU];
    struct sockaddr_in peer_addr;

    int addr_len = sizeof(peer_addr);
    int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&peer_addr, &addr_len);
    if(bytes == -1)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }

    struct icmp_echo *icmp = (struct icmp_echo *)(buffer + 20);
    if(icmp->type != 0 || icmp->code != 0)
        return 0;

    if(ntohs(icmp->ident) != ident)
        return 0;

    printf("%s seq=%d %5.2fms\n", inet_ntoa(peer_addr.sin_addr), ntohs(icmp->seq), (get_timestamp() - icmp->sending_ts)*1000);
    return 0;
}

int ping(const char *ip)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    if(inet_aton(ip, (struct sockaddr *)&addr.sin_addr.s_addr) == 0)
        return -1;
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sock == -1)
    {
        perror("socket create");
        return -1;
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RECV_TIMEOUT_USEC;
    int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if (ret == -1)
        return -1;
    
    double next_ts = get_timestamp();
    int ident = getpid();
    int seq = 1;

    for(;;)
    {
        if(get_timestamp() >= next_ts)
        {
            ret = send_echo_request(sock, &addr, ident, seq);
            if(ret == -1)
            perror("Send failed");
             next_ts += 1;
            seq += 1;
        }
        ret = recv_echo_reply(sock, ident);
        if(ret == -1)
            perror("Receive failed");
    }
    return 0;
}

int main(int argc, const char *argv[])
{
    return ping(argv[1]);
}