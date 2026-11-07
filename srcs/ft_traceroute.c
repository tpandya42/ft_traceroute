#include "ft_traceroute.h"
#include <time.h>

static void handle_sigint(int sig)
{
    (void)sig;
    g_sig = 1;
}

static int  icmp_socket(t_config *conf, int ttl)
{
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd < 0)
    {
        perror("ft_traceroute: socket");
        return -1;
    }

    // set ttl
    if (setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("ft_traceroute: setsockopt(IP_TTL)");
    }

    struct timeval tv;
    tv.tv_sec = conf->timeout_ms / 1000;
    tv.tv_usec = (conf->timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        perror("ft_traceroute: setsockopt(SO_RCVTIMEO)");
    return fd; 
}

static uint32_t checksumSum(const void *data, size_t len, uint32_t sum)
{
    const uint16_t *words = (const uint16_t *)data;
    while (len > 1)
    {
        sum += *words++;
        len -= 2;
    }
    if (len == 1) {
        uint16_t odd_byte = 0;
        *((uint8_t *)&odd_byte) = *(const uint8_t *)words;
        sum += odd_byte;
    }
    return sum;
}

static uint16_t checksumFold(uint32_t sum)
{
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)(~sum);
}

static uint16_t calculateChecksum(const void *data, size_t l)
{
    return checksumFold(checksumSum(data, l, 0));
}

static size_t   buildEchoPacket(uint8_t *packet, uint16_t seq, uint16_t pid, struct timespec *st_time)
{
    size_t icmp_len = sizeof(struct icmphdr) + 56;
    struct icmphdr *hdr = (struct icmphdr *)packet;
    hdr->type = ICMP_ECHO;
    hdr->code = 0;
    hdr->un.echo.id = htons(pid);
    hdr->un.echo.sequence = htons(seq);
    hdr->checksum = 0;
    uint8_t *payload = packet + sizeof(struct icmphdr);
    memset(payload, 0x42, 56);
    memcpy(payload + 56 - sizeof(struct timespec), st_time, sizeof(struct timespec));
    hdr->checksum = calculateChecksum(packet, icmp_len);
    return icmp_len;
}

int     ft_traceroute(t_config *conf, struct sockaddr_in *target_addr, char *ip_str)
{
    signal(SIGINT, handle_sigint);
    uint8_t packet[sizeof(struct icmphdr) + 1500];
    int ttl = 1;
    int seq = 0;
    (void)ip_str;

    printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", conf->hostname, conf->ip_str);

    while (!g_sig && ttl <= 30)
    {
        int fd = icmp_socket(conf, ttl);
        if (fd < 0) return -1;

        int reached = 0;
        int got_reply = 0;
        char hop_ip[INET_ADDRSTRLEN];
        memset(hop_ip, 0, sizeof(hop_ip));

        printf("%2d  ", ttl);

        // 3 probes per TTL
        for (int i = 0; i < 3; i++)
        {
            struct timespec send_ts;
            clock_gettime(CLOCK_MONOTONIC, &send_ts);
            size_t icmp_len = buildEchoPacket(packet, seq, conf->pid, &send_ts);
            if (icmp_len == 0) { close(fd); return (-1); }

            ssize_t sent_bytes = sendto(fd, packet, icmp_len, 0, (struct sockaddr *)target_addr, sizeof(*target_addr));
            if (sent_bytes < 0) { printf(" *"); seq++; continue; }

            char buf[2048];
            struct sockaddr_in res_addr;
            socklen_t addr_len = sizeof(struct sockaddr_in);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&res_addr, &addr_len);
            if (n < 0) { printf(" *"); seq++; continue; }

            struct iphdr *iph = (struct iphdr *)buf;
            int iphdr_len = iph->ihl * 4;
            struct icmphdr *icmph = (struct icmphdr *)(buf + iphdr_len);

            struct timespec end_ts;
            clock_gettime(CLOCK_MONOTONIC, &end_ts);
            double rtt = (end_ts.tv_sec - send_ts.tv_sec) * 1000.0
                       + (end_ts.tv_nsec - send_ts.tv_nsec) / 1000000.0;

            if (!got_reply) {
                strncpy(hop_ip, inet_ntoa(res_addr.sin_addr), sizeof(hop_ip) - 1);
                got_reply = 1;
                printf(" %s (%s)", hop_ip, hop_ip);
            }
            printf("  %.3f ms", rtt);

            if (icmph->type == ICMP_ECHOREPLY && res_addr.sin_addr.s_addr == target_addr->sin_addr.s_addr)
                reached = 1;
            seq++;
        }

        close(fd);
        printf("\n");

        if (reached)
            break;
        ttl++;
    }

    return (0);
}
