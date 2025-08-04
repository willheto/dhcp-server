#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "../include/subnetMaskUtils.h"
#include "../include/dhcpPacketHandler.h"
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 67
#define BUFFER_SIZE 1500

void startServer() {
	int sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);
	unsigned char buffer[BUFFER_SIZE];
  
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
    perror("socket");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sock);
        exit(1);
    }

	char mask[INET_ADDRSTRLEN];
	if (getSubnetMask(mask, sizeof(mask)) == 0) {
		printf("Subnet mask is %s\n", mask);
		if (strcmp(mask, "255.255.255.0") == 0) {
			printf("Offering LAN IPs from /24 pool\n");
		} else {
			printf("No support for subnets outside /24, bye.\n");
		}
	} else {
		printf("Failed to get subnet mask.");
	}

    printf("Listening for DHCP packets on port %d...\n", PORT);
    while (1) {
        int n = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        printf("Received %d bytes from %s:%d\n", n,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
		
		handleDhcpActionBasedOnMessage(sock, buffer);
	}
}
