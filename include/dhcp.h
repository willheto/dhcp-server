#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include <netinet/in.h>

#define DHCP_OPTIONS_SIZE 312

struct dhcp_packet {
	uint8_t op;      // Message op code / message type
	uint8_t htype;   // Hardware address type
	uint8_t hlen;    // Hardware address length
	uint8_t hops;
	uint32_t xid;    // Transaction ID
	uint16_t secs;
	uint16_t flags;
	struct in_addr ciaddr; // Client IP address
	struct in_addr yiaddr; // Offer IP address
	struct in_addr siaddr; // Server IP address
	struct in_addr giaddr; // Gateway IP address
	uint8_t chaddr[16];    // Client hardware address
	uint8_t sname[64];     // Server host name
	uint8_t file[128];     // Boot file name
	uint8_t options[DHCP_OPTIONS_SIZE];
};

#endif
