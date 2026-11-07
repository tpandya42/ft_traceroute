#include "ft_traceroute.h"

int     resolveDNS(t_config *conf)
{
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_flags = AI_CANONNAME;

    if (getaddrinfo(conf->hostname, NULL, &hints, &result) != 0)
    {
        fprintf(stderr, "ft_traceroute: unknown host %s\n", conf->hostname);
        return (-1);
    }
    struct addrinfo *rp = NULL;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET)
            break;
    }
    if (!rp)
    {
        fprintf(stderr, "ft_traceroute: no IPv4 address for %s\n", conf->hostname);
        freeaddrinfo(result);
        return (-1);
    }
    struct sockaddr_in *addr = (struct sockaddr_in *)rp->ai_addr;
    conf->target_addr = *addr;
    const char *ptr = inet_ntop(AF_INET, &addr->sin_addr, conf->ip_str, sizeof(conf->ip_str));
    if (!ptr)
    {
        strncpy(conf->ip_str, conf->hostname, sizeof(conf->ip_str));
    }
    freeaddrinfo(result);
    return (0);
}
