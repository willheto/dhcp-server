#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include "../include/dhcp.h"

enum OpCode {
	REQUEST = 1,
	RESPONSE = 2
};

int getFreeIp(struct in_addr *offered_ip) {
	// Hardcoded IP for testing	
	if(inet_aton("192.168.1.80", offered_ip)) {
		return 1;
	} else {
		return 0;
	}
}

int createOfferPacket(struct dhcp_packet *offer, unsigned char *buffer) {
	memset(offer, 0, sizeof(struct dhcp_packet));

	enum OpCode offerPacketOpCode = RESPONSE;
	offer->op = offerPacketOpCode;
	offer->htype = 1;
	offer->hlen = 6;
	offer->hops = 0;

	struct dhcp_packet *discover = (struct dhcp_packet *)buffer;
	uint32_t xid_from_discover = discover->xid;

	offer->xid = xid_from_discover; 
	offer->secs = 0;
	offer->flags = htons(0x8000);

	struct in_addr offered_ip;
	if(!getFreeIp(&offered_ip)) {
		return 0;
	}

	offer->yiaddr = offered_ip;
	inet_aton("192.168.1.140", &offer->siaddr);
	memcpy(offer->chaddr, discover->chaddr, 6);

	offer->ciaddr.s_addr = 0;

	uint8_t *opt = offer->options;

	opt[0] = 99;
	opt[1] = 130;
	opt[2] = 83;
	opt[3] = 99;

	opt += 4;  // move past magic cookie
	
	// Option 53: DHCP message type = Offer (2)
	*opt++ = 53;  // option code
	*opt++ = 1;   // length
	*opt++ = 2;   // offer
	
	// Option 54: Server Identifier (your server IP)
	*opt++ = 54;
	*opt++ = 4;
	struct in_addr server_ip;
	inet_aton("192.168.1.140", &server_ip);
	memcpy(opt, &server_ip.s_addr, 4);
	opt += 4;
	
	// Option 1: Subnet mask (255.255.255.0)
	*opt++ = 1;

	*opt++ = 4;
	uint32_t subnet_mask = inet_addr("255.255.255.0");
	memcpy(opt, &subnet_mask, 4);
	opt += 4;

	// Option 51: Lease time (seconds) — example 3600 (1 hour)
	*opt++ = 51;
	*opt++ = 4;
	uint32_t lease_time = htonl(3600);
	memcpy(opt, &lease_time, 4);
	opt += 4;

	// Option 255: End option
	*opt++ = 255;	

	return 1;
}

int getRequestedIp(unsigned char *buffer, struct in_addr *requested_ip) {
    struct dhcp_packet *packet = (struct dhcp_packet *)buffer;
    uint8_t *options = packet->options;

    if (!(options[0] == 99 && options[1] == 130 && options[2] == 83 && options[3] == 99)) {
        fprintf(stderr, "Invalid DHCP magic cookie.\n");
        return 0;
    }

    options += 4;

    while (*options != 255) {
        uint8_t option_type = *options++;
        if (option_type == 0) continue;

        uint8_t option_len = *options++;

        if (option_type == 50 && option_len == 4) {
            memcpy(&requested_ip->s_addr, options, 4);
            return 1;
        }

        options += option_len;
    }

    return 0;
}

void sendDhcpPacket(int sock, struct dhcp_packet *packet, int use_broadcast, struct in_addr dest_ip) {
    size_t packet_size = sizeof(struct dhcp_packet);

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(68);

    if (use_broadcast) {
        client_addr.sin_addr.s_addr = INADDR_BROADCAST;

        int enable_broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast));
    } else {
        client_addr.sin_addr = dest_ip;
    }

    ssize_t sent = sendto(sock,
                          packet,
                          packet_size,
                          0,
                          (struct sockaddr *)&client_addr,
                          sizeof(client_addr));

    if (sent < 0) {
        perror("DHCP packet send failed!");
    } else {
        printf("DHCP packet (%zd bytes) sent to %s\n", sent, inet_ntoa(client_addr.sin_addr));
    }
}

int createAckPacket(struct dhcp_packet *ack, unsigned char *buffer, struct in_addr assigned_ip) {
    memset(ack, 0, sizeof(struct dhcp_packet));

    enum OpCode ackPacketOpCode = RESPONSE;
    ack->op = ackPacketOpCode;
    ack->htype = 1;
    ack->hlen = 6;
    ack->hops = 0;

    struct dhcp_packet *request = (struct dhcp_packet *)buffer;
    ack->xid = request->xid;
    ack->secs = 0;
    ack->flags = request->flags;

    // set the assigned IP address (should match what was offered/requested)
    ack->yiaddr = assigned_ip;

    inet_aton("192.168.1.140", &ack->siaddr);  // Hardcoded for now.
    memcpy(ack->chaddr, request->chaddr, 6);

    ack->ciaddr.s_addr = 0;

    uint8_t *opt = ack->options;

    // Magic cookie again
    opt[0] = 99;
    opt[1] = 130;
    opt[2] = 83;
    opt[3] = 99;
    opt += 4;

    *opt++ = 53;
    *opt++ = 1;
    *opt++ = 5;

    *opt++ = 54;
    *opt++ = 4;
    struct in_addr server_ip;
    inet_aton("192.168.1.140", &server_ip);
    memcpy(opt, &server_ip.s_addr, 4);
    opt += 4;

    *opt++ = 1;
    *opt++ = 4;
    uint32_t subnet_mask = inet_addr("255.255.255.0");
    memcpy(opt, &subnet_mask, 4);
    opt += 4;

    *opt++ = 3;
    *opt++ = 4;
    struct in_addr gateway;
    inet_aton("192.168.1.1", &gateway);
    memcpy(opt, &gateway.s_addr, 4);
    opt += 4;

    *opt++ = 6;
    *opt++ = 4;
    struct in_addr dns;
    inet_aton("8.8.8.8", &dns);  // Google dns for now, doesn't really matter at this point.
    memcpy(opt, &dns.s_addr, 4);
    opt += 4;

    *opt++ = 51;
    *opt++ = 4;
    uint32_t lease_time = htonl(3600); // 1 hour
    memcpy(opt, &lease_time, 4);
    opt += 4;

    *opt++ = 255;

    return 1;
}

void handleDhcpActionBasedOnMessage(int sock, unsigned char *buffer) {
    // DHCP fixed header is 236 bytes + 4 bytes magic cookie
    unsigned char *options = buffer + 236;

    if (options[0] != 0x63 || options[1] != 0x82 || options[2] != 0x53 || options[3] != 0x63) {
        printf("Invalid DHCP magic cookie\n");
        return;
    }

    unsigned char *opt_ptr = options + 4;
    while (*opt_ptr != 255) {
        unsigned char option_type = *opt_ptr++;
        unsigned char option_len = *opt_ptr++;

        if (option_type == 53 && option_len == 1) {
            unsigned char msg_type = *opt_ptr;
            switch (msg_type) {
                case 1: {
					printf("Received DHCPDISCOVER\n");
					struct in_addr requested_ip;
					struct dhcp_packet offer;
					createOfferPacket(&offer, buffer);
					printf("DHCPOFFER packet created\n");
					sendDhcpPacket(sock, &offer, 1, requested_ip);
					printf("Sending DHCPOFFER...\n");
					break;
				}
                case 2: printf("DHCPOFFER\n"); break;
                case 3: {
					printf("Received DHCPREQUEST\n");
					struct in_addr requested_ip;
					struct dhcp_packet ack;
					if (getRequestedIp(buffer, &requested_ip)) {
						printf("Client requested IP: %s\n", inet_ntoa(requested_ip));
						createAckPacket(&ack, buffer, requested_ip);
						printf("DHCPACK packet created\n");
						printf("Sending DHCPACK...\n");
						sendDhcpPacket(sock, &ack, 1, requested_ip);
					} else {
						printf("No requested IP found in DHCPREQUEST.\n");
					}
					break;
				}
                case 4: printf("DHCPDECLINE\n"); break;
                case 5: printf("DHCPACK\n"); break;
                case 6: printf("DHCPNAK\n"); break;
                case 7: printf("DHCPRELEASE\n"); break;
                case 8: printf("DHCPINFORM\n"); break;
                default: printf("Unknown DHCP Message Type: %d\n", msg_type); break;
            }
        }

		if (option_type == 54 && option_len == 4) {
            struct in_addr server_ip;
            memcpy(&server_ip, opt_ptr, 4);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &server_ip, ip_str, sizeof(ip_str));


            printf("The request is linked server IP %s", ip_str);
			return;
        }

	
        opt_ptr += option_len;  // skip to next option
    }

    printf("DHCP Message Type option not found\n");
}
