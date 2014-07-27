#include "net_parser.h"
#include <arpa/inet.h>


static int get_auth (u_int32_t *ui, int32_t *euid, int32_t *egid)
{
        unsigned int *pi;
        unsigned int len;

        /*
         * If the auth type isn't UNIX, then just give up.
         *
         * &&& Could also check to make sure that the length fields
         * are something sensible, as an extra sanity check.
         */

        if (ntohl (ui [0]) != AUTH_UNIX) {
                printf ("XX Not Auth_Unix (%ld)??\n", ntohl (ui [0]));
                return (-1);
        }

        /*
         * For UNIX, ui[1] is the total length of the auth and ui[2]
         * is some kind of cookie.  ui[3] is the length of the next
         * field, which doesn't seem to be well documented anywhere. 
         * Set pi to point past this opaque field.  Then pi[0] is the
         * euid, pi[1] is the egid, and pi[2] is the total number of
         * groups.  At some point we might be interested in the total
         * number of groups, and what they all are, but not today.
         */

        len = ntohl (ui [3]);
        if (len % 4) {
                len /= 4;
                len++;
        }
        else {
                len /= 4;
        }

        pi = &ui [4 + len];
        if (euid != NULL)
                *euid = ntohl (pi [0]);

        if (egid != NULL)
                *egid = ntohl (pi [1]);

        return (0);
}

int getData (u_char *bp, u_char *buf, unsigned int maxlen)
{
        return (0);
}

/*
 * Returns a pointer to the hash_t structure for the given xid (or
 * NULL if any) and removes it from the hash table.  Doesn't free the
 * pointer-- it is the responsibility of the caller to free it after
 * use.
 */


hash_t *hashLookupXid (u_int32_t now,
                u_int32_t xid, u_int32_t host, u_int32_t port)
{
        u_int32_t hashval;
        hash_t *curr, **prev_p;

        /*
         * Heuristic-- as long as the current time is less
         * than the GRACE_PERIOD (typically a few moments after
         * the start of tracing, but it could also be set to
         * the end of time...), accept anything without even looking.
         *
         * The reason is that XIDs pairs can easily get split over
         * trace boundaries and we don't want to lose any replies just
         * because we might have put the request into another file.
         *
         * NOT IMPLEMENTED YET!
         */


        /*
         * Otherwise, search the table.  If the XID is found, remove
         * it; there should only be one entry corresponding to each
         * XID so we won't be seeing it again.
         */
        hashval = HASH (xid, host, port);
        prev_p = &xidHashTable [hashval];
        for (curr = xidHashTable [hashval]; curr != NULL; curr = curr->next) {
                if ((curr->rpcXID == xid) && (curr->srcHost == host) &&
                                (curr->srcPort == port)) {
                        *prev_p = curr->next;
                        xidHashElemCnt--;
                        return (curr);
                }
                prev_p = &curr->next;
        }


        return (NULL);
}
/*
 * There are a lot more fields, and they're important, but for now I'm
 * ignoring them.
 */

int hashInsertXid (u_int32_t now, u_int32_t xid,
                u_int32_t host, u_int32_t port,
                u_int32_t version, u_int32_t proc)
{
        static int CullIndex = 0;
        u_int32_t then;
        u_int32_t hashval = HASH (xid, host, port);
        hash_t *newEl = (hash_t *) malloc (sizeof (hash_t));
        hash_t *curr, *next, *old;

        newEl->rpcXID = xid;
        newEl->srcHost = host;
        newEl->srcPort = port;
        newEl->nfsVersion = version;
        newEl->nfsProc = proc;
        newEl->call_time = now;

        newEl->next = xidHashTable [hashval];
        xidHashTable [hashval] = newEl;

        xidHashElemCnt++;

        /*
         * HACK ALERT!
         *
         * Due to dropped packets, some requests never get responses. 
         * This causes the table to leak.  To make sure that
         * eventually all the garbage has a chance to be collected,
         * every time we do a lookup, we pick a hash bucket to cull. 
         * In order to prevent garbage from hiding, we cull a
         * different bucket each time, working our way around the
         * whole table.
         */
       then = now - MAX_AGE;
        for (curr = xidHashTable [CullIndex]; curr != NULL; curr = next) {
                next = curr->next;

                if (curr->call_time < then) {
                        old = hashLookupXid (now, curr->rpcXID, curr->srcHost,
                                        curr->srcPort);
                        assert (old != NULL);
                        free (old);
                }
        }
        CullIndex = (CullIndex + 1) % HASHSIZE;

        return (0);
}
int getEtherHeader (u_int32_t packet_len,
                const u_char *bp, unsigned int *proto, unsigned int *len)
{
        unsigned int length = (bp [12] << 8) | bp [13];

        /*
         * Look for either 802.3 or RFC 894:
         *
         * if "length" <= OLD_ETHERMTU).  Note that we let the pcap
         * library make the decision for jumbo frames-- pcap has
         * already calculated the apparent packet length, and if it
         * agrees with the 802.3 length field, and the next few fields
         * make sense according to the 802.3 spec, then we assume that
         * this is an 802.3 packet.  This is not riskless, but I have
         * no better way to tell the difference.
         */

        if ((length <= ETHERMTU) ||
                        (length == (packet_len - 22) && /* jumbo? */
                        (bp [14] == 0xAA) &&            /* DSAP */
                        (bp [15] == 0xAA) &&            /* SSAP */
                        (bp [16] == 0x03))) {           /* CNTL */
                *proto = (bp [20] << 8) | bp [21];
                *len = length;
                return (22);
        }
        else {
                *len = packet_len - 14;
                *proto = length;
                return (14);
        }
}
/*
 * ip_b is assumed to point to the start of an IP header.  If sanity
 * checks pass, returns the number of bytes consumed by the header
 * (including options).
 */

int getIpHeader (struct ip *ip_b, unsigned int *proto, unsigned int *len,
                u_int32_t *src, u_int32_t *dst)
{
        u_int32_t *ip = (u_int32_t *) ip_b;
        u_int32_t off;

        /*
         * If it's not IPv4, then we're completely lost.
         */

        if (IP_V (ip_b) != 4) {
                return (-1);
        }

        /*
         * If this isn't fragment zero of an higher-level "packet",
         * then it's not something that we're interested in, so dump
         * it.
         */

        off = ntohs(ip_b->ip_off);
        if ((off & 0x1fff) != 0) {
                return (0);
        }

        *proto = ip_b->ip_p;
        *len = ntohs (ip_b->ip_len);
        *src = ntohl (ip [3]);
        *dst = ntohl (ip [4]);

        return (IP_HL (ip_b) * 4);
}
/*
 * Right now, all we want out of the tcp header is the length,
 * so we can skip over it.  If all goes well, we pluck out the
 * ports elsewhere.
 */

int getTcpHeader (struct tcphdr *tcp_b)
{

        return (4 * TH_OFF (tcp_b));
}

int getUdpHeader (struct udphdr *udp_b)
{

        return (8);
}

int getRpcHeader (struct rpc_msg *bp, u_int32_t *dir_p, u_int32_t maxlen,
                int32_t *euid, int32_t *egid)
{
        int cred_len, verf_len;
        u_int32_t *ui, *auth_base;
        u_int32_t rh_len;
        u_int32_t dir;
        u_int32_t *max_ptr;
        int rc;

        max_ptr = (u_int32_t *) bp;
        max_ptr += (maxlen / sizeof (u_int32_t));

        *dir_p = dir = ntohl (bp->rm_direction);

        if (dir == CALL) {
                if ((ntohl (bp->ru.RM_cmb.cb_rpcvers) != RPC_VERSION) ||
                        (ntohl (bp->ru.RM_cmb.cb_prog) != NFS_PROGRAM)) {
			std::cerr<<"RPC_VERSION="<<ntohl (bp->ru.RM_cmb.cb_rpcvers)<<" NFS_PROGRAM:"<<ntohl (bp->ru.RM_cmb.cb_prog)<<std::endl;
                        return (-1);
                }
                /*
                 * Need to skip over all the credentials and
                 * verification fields.  The nuisance is figuring out
                 * how long these fields are, sincentohl they do not have a
                 * fixed size.
                 *
                 * Note that we must depart from using the rpc_msg
                 * structure here, because it doesn't know how big
                 * these structs are either.  It just uses an opaque
                 * reference to get at them, after they've been
                 * parsed.  Since I see things in an unparsed state, I
                 * need to actually do the calculation.
                 *
                 * Note that the credentials and verifier are
                 * word-aligned, so we can't just add up cred_len and
                 * verf_len and always get the right answer.  Instead,
                 * we need to round them both up.  It's easier to just
                 * do some pointer arithmetic at the end.
                 */

                ui = (u_int32_t *) &bp->rm_call.cb_cred;
                auth_base = ui;

                /*
                 * &&& Can also add sanity checks; there are limits on
                 * the possible lengths for the cred and the verifier!
                 */

                if ((ui + 1) >= max_ptr) {
/*                      fprintf (OutFile, "XX cut-off RPC header (cred)\n"); */
                        return (-2);
                }

                cred_len = ntohl (ui [1]);

                auth_base = ui;
               ui += (cred_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

                if ((ui + 1) >= max_ptr) {
/*                      fprintf (OutFile, "XX cut-off RPC header (verif)\n"); */
                        return (-3);
                }

                rc = get_auth (auth_base, euid, egid);
                if (rc != 0) {
                        return (-4);
                }

                verf_len = ntohl (ui [1]);

                ui += (verf_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

                rh_len = (4 * (ui - (u_int32_t *) bp));

        }
        else if (dir == REPLY) {

                /*
                 * Like the CALL case, but uglier because of potential
                 * alignment problems; can't use very much of the
                 * reply struct as-is.
                 */

                ui = ((u_int32_t *) &bp->rm_reply) + 1;

                if ((ui + 1) >= max_ptr) {
/*                      fprintf (OutFile, */
/*                              "XX cut-off RPC reply header (verif)\n"); */
                        return (-5);
                }
                verf_len = ntohl (ui [1]);

                ui += (verf_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

                rh_len = 4 * (ui - (u_int32_t *) bp);

        }
        else {
		std::cerr<<"dir="<<dir<<std::endl;
                /* Doesn't look like an RPC header: punt. */
                return (-6);
        }

        return (rh_len);
}





