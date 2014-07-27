#ifndef _NET_PARSER_INCLUDED
#define _NET_PARSER_INCLUDED

#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <netinet/in.h>
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include "ether.h"
//#include <netinet/in.h>
#include <rpc/rpc.h>
#include "nfs.h"
#include "nfs_prot.h"


#define MIN_ETHER_HDR_LEN       14              /* RFC 894 */
#define MIN_IP_HDR_LEN          20              /* w/o options */
#define MIN_TCP_HDR_LEN         20              /* w/o options */
#define MIN_UDP_HDR_LEN         8

#define MAX_AGE         30
#define OLD_ETHERMTU    1500    /* somewhat archaic, because jumbo frames */

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN  6

#define SIZE_ETHERNET 14

#define NFS_PROGRAM     100003
#define RPC_VERSION     2

	
static int get_auth (u_int32_t *ui, int32_t *euid, int32_t *egid);
int getData (u_char *bp, u_char *buf, unsigned int maxlen);
hash_t *hashLookupXid (u_int32_t now,
                u_int32_t xid, u_int32_t host, u_int32_t port);

int hashInsertXid (u_int32_t now, u_int32_t xid,
                u_int32_t host, u_int32_t port,
                u_int32_t version, u_int32_t proc);

int getEtherHeader (u_int32_t packet_len,
                const u_char *bp, unsigned int *proto, unsigned int *len);

int getIpHeader (struct ip *ip_b, unsigned int *proto, unsigned int *len,
                u_int32_t *src, u_int32_t *dst);

int getTcpHeader (struct tcphdr *tcp_b);
int getUdpHeader (struct udphdr *udp_b);

int getRpcHeader (struct rpc_msg *bp, u_int32_t *dir_p, u_int32_t maxlen,
                int32_t *euid, int32_t *egid);

	



#endif
