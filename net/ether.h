#ifndef _ETHER_H_
#define _ETHER_H_

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_ether.h	8.3 (Berkeley) 5/2/95
 */

#include <stdint.h>

#define	ETHERMTU	1500

/*
 * The number of bytes in an ethernet (MAC) address.
 */
#define	ETHER_ADDR_LEN		6

/*
 * Structure of a DEC/Intel/Xerox or 802.3 Ethernet header.
 */
typedef struct	ether_header {
	uint8_t	ether_dhost[ETHER_ADDR_LEN];
	uint8_t	ether_shost[ETHER_ADDR_LEN];
	uint16_t	ether_type;
}
ether_hdr_t;

/*
 * Length of a DEC/Intel/Xerox or 802.3 Ethernet header; note that some
 * compilers may pad "struct ether_header" to a multiple of 4 bytes,
 * for example, so "sizeof (struct ether_header)" may not give the right
 * answer.
 */
#define ETHER_HDRLEN		14

#define     ETHERTYPE_IP            0x0800  /* IP protocol */
#define     ETHERTYPE_VLAN          0x8100  /* 802.1Q VLAN */
#define     ETHERTYPE_VLAN2         0x88a8  /* 802.1Q VLAN 2 */

#endif // _ETHER_H_
