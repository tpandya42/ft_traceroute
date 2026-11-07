#ifndef FT_TRACEROUTE_H
#define FT_TRACEROUTE_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdint.h>
#include <signal.h>
#include <stdbool.h>

typedef struct s_config {
	char 	ip_str[INET_ADDRSTRLEN];
	char 	*hostname;
	struct 	sockaddr_in target_addr;
	int count;
	uint16_t  pid;
} t_config;

typedef struct s_logs {
	int	sent;
	int	rec;
	struct timespec start_time;
	struct timespec end_time;
	double rtt_max;
	double rtt_min;
	double rtt_sum;
	double rtt_mdev; 
} t_logs;

extern volatile sig_atomic_t g_sig;

t_config	parseInput(char **argv);
int resolveDNS(t_config *conf);
#endif
