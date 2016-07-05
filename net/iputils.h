#ifndef _IPUTILS_H_
#define _IPUTILS_H_

#include <sys/types.h>	// in_addr

void ip2string(char *dest, uint32_t ipaddr);

// is an internal address (192.168.0.0/16, 10.0.0.0/8, 172.16.0.0/12)
// also returns true if broadcast (255.255.255.0)
int ipaddr_is_lan(uint32_t ipaddr);

int ipaddr_is_bcast(uint32_t ipaddr);

#endif // _IPUTILS_H_
