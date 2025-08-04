## DHCP-SERVER
Part of a project to build a router from scratch. Listens and identifies dhcp packets and their types. Also offers a hardcoded ip to any clients who send a dhcpdiscover packet. If client responds further, it should also send a dhcpack packet to confirm lease.

Code quality is currently terrible, and the project is work in progress. Formatting may also be a bit off on some editors, since i'm using nvim and some plugins myself.

Doesn't include built project, but there's a makefile for that.
