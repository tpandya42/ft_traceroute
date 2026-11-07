#include "ft_traceroute.h"
#include <time.h>

static void handle_sigint(int sig)
{
    (void)sig;
    g_sig = 1;
}

static int  udp_socket(t_config *conf, int ttl)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("ft_traceroute: socket");
        return -1;
    }

    // set ttl
    if (setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("ft_traceroute: setsockopt(IP_TTL)");
        close(fd);
        return (-1);
    }

    struct timeval tv;
    tv.tv_sec = conf->timeout_ms / 1000;
    tv.tv_usec = (conf->timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        perror("ft_traceroute: setsockopt(SO_RCVTIMEO)");
    return fd; 
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
        close(fd);
        return (-1);
    }

    struct timeval tv;
    tv.tv_sec = conf->timeout_ms / 1000;
    tv.tv_usec = (conf->timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        perror("ft_traceroute: setsockopt(SO_RCVTIMEO)");
    return fd; 
}


int     ft_traceroute(t_config *conf, struct sockaddr_in *target_addr, char *ip_str)
{
    signal(SIGINT, handle_sigint);
    int ttl = 1;
    int seq = 0;
    int start_port = 33434; // traceroute uses same port by default
    (void)ip_str;

    printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", conf->hostname, conf->ip_str);

    // STEP 1: create raw ICMP socket for receiving (reused for all TTLs)
    int icmp_fd = icmp_socket(conf, 1);
    if (icmp_fd < 0) return -1;

    while (!g_sig && ttl <= 30)
    {
        int reached = 0;
        int got_reply = 0;
        char hop_ip[INET_ADDRSTRLEN];
        memset(hop_ip, 0, sizeof(hop_ip));

        printf("%2d  ", ttl);

        // 3 probes per TTL 
        for (int i = 0; i < 3; i++)
        {
            int port = start_port + seq;
            uint8_t packet[56];
            struct timespec send_ts;
            clock_gettime(CLOCK_MONOTONIC, &send_ts);
            memcpy(packet, &send_ts, sizeof(send_ts));
            memset(packet + sizeof(send_ts), 0x42, 56 - sizeof(send_ts));

            // STEP 2: Send UDP
            int udp_fd = udp_socket(conf, ttl);
            if (udp_fd < 0) { close(icmp_fd); return -1; }

            struct sockaddr_in dest = *target_addr;
            dest.sin_port = htons(port);
            ssize_t sent_bytes = sendto(udp_fd, packet, 56, 0, (struct sockaddr *)&dest, sizeof(dest));
            close(udp_fd);

            if (sent_bytes < 0) { printf(" *"); seq++; continue; }

            // STEP 3: receive ICMP error on raw ICMP socket
            char buf[2048];
            struct sockaddr_in res_addr;
            socklen_t addr_len = sizeof(struct sockaddr_in);
            ssize_t n = recvfrom(icmp_fd, buf, sizeof(buf), 0, (struct sockaddr *)&res_addr, &addr_len);
            if (n < 0) { printf(" *"); seq++; continue; }

            // STEP 4: validate received packet size
            if (n < (ssize_t)sizeof(struct iphdr)) { printf(" *"); seq++; continue; }
            struct iphdr *iph = (struct iphdr *)buf;
            if (iph->protocol != IPPROTO_ICMP) { printf(" *"); seq++; continue; }
            int iphdr_len = iph->ihl * 4;
            if (n < (ssize_t)(iphdr_len + sizeof(struct icmphdr))) { printf(" *"); seq++; continue; }
            struct icmphdr *icmph = (struct icmphdr *)(buf + iphdr_len);

            // STEP 5: check if embedded original UDP port matches our sent udp request.
            uint8_t *orig_payload = (uint8_t *)icmph + sizeof(struct icmphdr);
            if (n < (ssize_t)(iphdr_len + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8)) { printf(" *"); seq++; continue; }
            struct iphdr *orig_iph = (struct iphdr *)orig_payload;
            int orig_iphdr_len = orig_iph->ihl * 4;
            struct udphdr *orig_udp = (struct udphdr *)(orig_payload + orig_iphdr_len);
            if (ntohs(orig_udp->dest) != port) { printf(" *"); seq++; continue; }

            struct timespec end_ts;
            clock_gettime(CLOCK_MONOTONIC, &end_ts);
            double rtt = (end_ts.tv_sec - send_ts.tv_sec) * 1000.0
                       + (end_ts.tv_nsec - send_ts.tv_nsec) / 1000000.0;

            if (icmph->type == ICMP_TIME_EXCEEDED) {
                if (!got_reply) {
                    strncpy(hop_ip, inet_ntoa(res_addr.sin_addr), sizeof(hop_ip) - 1);
                    got_reply = 1;
                    printf(" %s (%s)", hop_ip, hop_ip);
                }
                printf("  %.3f ms", rtt);
            }
            else if (icmph->type == ICMP_DEST_UNREACH && icmph->code == ICMP_PORT_UNREACH) {
                if (!got_reply) {
                    strncpy(hop_ip, inet_ntoa(res_addr.sin_addr), sizeof(hop_ip) - 1);
                    got_reply = 1;
                    printf(" %s (%s)", hop_ip, hop_ip);
                }
                printf("  %.3f ms", rtt);
                reached = 1;
            }
            else {
                printf(" *");
            }
            seq++;
        }

        printf("\n");
        if (reached) break;
        ttl++;
    }

    close(icmp_fd);
    return (0);
}
