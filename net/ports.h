#ifndef _PORTS_H_
#define _PORTS_H_

#define BF_PORT_21   (1<<0)		// FTP
#define BF_PORT_22   (1<<1)		// SSH
#define BF_PORT_23   (1<<2)		// Telnet
#define BF_PORT_53   (1<<3)		// DNS
#define BF_PORT_80   (1<<4)		// HTTP
#define BF_PORT_123  (1<<5)		// NTPD
#define BF_PORT_135  (1<<6)		// WINS
#define BF_PORT_137  (1<<7)		// Netbios name service

#define BF_PORT_138  (1<<8)		// Netbios datagram
#define BF_PORT_161  (1<<9)		// SNMP
#define BF_PORT_162  (1<<10)		// SNMP TRAP
#define BF_PORT_389  (1<<11)		// LDAP
#define BF_PORT_443  (1<<12)		// HTTPS
#define BF_PORT_445  (1<<13)		// Active Directory shares
#define BF_PORT_37   (1<<14)		// TIME
#define BF_PORT_8080 (1<<15)		// HTTP Dev

#endif // _PORTS_H_
