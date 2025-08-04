#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

int getSubnetMask(char *outBuf, size_t bufSize) {
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	const char *addr;

	getifaddrs (&ifap);

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family==AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_netmask;

			if (strcmp(ifa->ifa_name, "eth0") == 0) {
				addr = inet_ntoa(sa->sin_addr);
				strncpy(outBuf, addr, bufSize - 1);
				outBuf[bufSize - 1] = '\0';
				freeifaddrs(ifap);
				return 0;
			}
		}
           
    }

	freeifaddrs(ifap);
	return -1;
}
