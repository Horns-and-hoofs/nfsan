//#include <pcap.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <netinet/in.h>
//#include <iostream>
//#include "ip.h"
//#include "tcp.h"
//#include "udp.h"
//#include "ether.h"
//#include <rpc/rpc.h>
#include <signal.h>
#include "net_parser.h"
#include "nfs.h"
#include "nfs_prot.h"
#include "server.h"
#include "configuration.h"

int number = 0;

SynchronizedDequeue m_q;

void got_packet(u_char* args, const pcap_pkthdr* header, const u_char* pp)
{

	nfs_pkt_t record2;
	nfs_pkt_t* record = &record2;
	struct	ip	*ip_b		= NULL;
	struct	tcphdr	*tcp_b		= NULL;
	struct	udphdr	*udp_b		= NULL;
	struct  rpc_msg	*rpc_b		= NULL;
	u_int32_t	tot_len		= header->caplen;
	u_int32_t	consumed	= 0;
	u_int32_t	src_port	= 0;
	u_int32_t	dst_port	= 0;
	u_int32_t	payload_len	= 0;
	int	e_len, i_len, h_len, rh_len;
	u_int32_t	rpc_len;
	unsigned int	length;
	u_int32_t srcHost, dstHost;
	unsigned int proto;
	u_int32_t dir;
	unsigned int cnt;
	std::cerr<<"step 1"<<std::endl;
	if (tot_len <= (MIN_ETHER_HDR_LEN + MIN_IP_HDR_LEN + MIN_UDP_HDR_LEN)) {
		return;
	}

	e_len = getEtherHeader (tot_len, pp, &proto, &length);
	if (e_len <= 0) {
		return;
	}
	consumed += e_len;
	/*
	 * If the type of the packet isn't IP, then we're not
	 * interested in it-- chuck it now.
	 *
	 * Note-- ordinarily by the time we get here, we've already
	 * filtered out the packets using a pattern in the pcap
	 * library, so this shouldn't happen (unless we are running
	 * off a capture of the entire traffic on the wire).
	 */

	if (proto != 0x0800) {
		return;
	}

	ip_b = (struct ip *) (pp + e_len);

	i_len = getIpHeader (ip_b, &proto, &length, &srcHost, &dstHost);
	if (i_len <= 0) {
		return;
	}

	consumed += i_len;
	/*
	 * Truncated packet-- what's up with that?  At this point,
	 * this can only happen if the packet is very short and the IP
	 * options are very long.  Still, must be cautious...
	 */

	if (consumed >= tot_len) {
		return;
	}

	if (proto == IPPROTO_TCP) {
		if (consumed + MIN_TCP_HDR_LEN >= tot_len) {
/* 			fprintf (OutFile, "XX 1: TCP pkt too short.\n"); */
			return;
		}

		tcp_b = (struct tcphdr *) (pp + consumed);
		h_len = getTcpHeader (tcp_b);
		if (h_len <= 0) {
/* 			fprintf (OutFile, "XX 2: TCP header error\n"); */
			return;
		}

		consumed += h_len;
		if (consumed >= tot_len) {
/* 			fprintf (OutFile, */
/* 			"XX 3: Dropped (consumed = %d, tot_len = %d)\n", */
/* 					consumed, tot_len); */
			return;
		}

		h_len += sizeof (u_int32_t);

		src_port = ntohs (tcp_b->th_sport);
		dst_port = ntohs (tcp_b->th_dport);

	}
	else if (proto == IPPROTO_UDP) {
		if (consumed + MIN_UDP_HDR_LEN >= tot_len) {
			return;
		}

		udp_b = (struct udphdr *) (pp + consumed);
		h_len = getUdpHeader (udp_b);
		if (h_len <= 0) {
			return;
		}

		consumed += h_len;
		if (consumed >= tot_len) {
			return;
		}

		src_port = ntohs (udp_b->uh_sport);
		dst_port = ntohs (udp_b->uh_dport);

		rpc_len = tot_len - consumed;

	}
	else {

		/* 
		 * If it's not TCP or UPD, then no matter what it is,
		 * we don't care about it, so just ignore it.
		 */

/* 		fprintf (OutFile, "XX 5: Not TCP or UDP.\n"); */
		return;
	}
	/*
	 * If we get to this point, there's a good chance this packet
	 * contains something interesting, so we start filling in the
	 * fields of the record immediately.
	 */

	record->secs	= header->ts.tv_sec;
	record->usecs	= header->ts.tv_usec;
	record->srcHost	= srcHost;
	record->dstHost	= dstHost;
	record->srcPort	= src_port;
	record->dstPort	= dst_port;
	cnt = 0;
	while (consumed < tot_len) {
		std::cerr<<"step 5.1 consumed="<<consumed<<" tot_len="<<tot_len<<std::endl;
		int32_t euid, egid;
		int print_it;

		cnt++;
		euid = egid = -1;

		/*
		 * If it's RPC over TCP, then the length of the rest
		 * of the RPC is given next.
		 *
		 * skip over the RPC length header, but save a copy
		 * for later use.
		 *
		 * &&& Note that the high-order bit is always set in
		 * the RPC length field.  This gives us a tiny extra
		 * sanity check, if we ever want it.
		 */

		if (proto == IPPROTO_TCP) {
			rpc_len = ntohl (*(u_int32_t *)(pp + consumed));
			rpc_len &= 0x7fffffff;

			consumed += sizeof (u_int32_t);
		}

		rpc_b = (struct rpc_msg *) (pp + consumed);
		rh_len = getRpcHeader (rpc_b, &dir, tot_len - consumed,
				&euid, &egid);
		if (rh_len == 0) {
			if (cnt > 1) {
			//	fprintf (OutFile,
			//		"XX rh_len == 0, cnt = %d\n", cnt);
			}
			return;
		}
		else if (rh_len < 0) {
			std::cerr<<"XX rh_len="<<rh_len<<std::endl;
/* 			fprintf (OutFile, "XX cnt = %d\n", cnt); */
			return;
		}
		consumed += rh_len;

		if (consumed == tot_len && dir == CALL && 
				ntohl (rpc_b->ru.RM_cmb.cb_proc) == NFSPROC_NULL) {

			/* It's a NULL RPC; do nothing */
			continue;
		}

		if (consumed >= tot_len) {
			//std::cerr<<"step 5.3 consumed="<<consumed<<" tot_len="<<tot_len<<std::endl;
/* 			fprintf (OutFile, "XX truncated payload %d >= tot_len %d\n", */
/* 					consumed, tot_len); */
			//goto end;
			return;
		}
		std::cerr<<"step 5.4 consumed="<<consumed<<" tot_len="<<tot_len<<" dir:"<<dir<<std::endl;
/* 		fprintf (OutFile, "XX good (cnt = %d)\n", cnt); */
		print_it = 1;

		if (dir == CALL) {
			record->rpcCall		= CALL;
			record->rpcXID		= ntohl (rpc_b->rm_xid);
			record->nfsVersion	= ntohl (rpc_b->ru.RM_cmb.cb_vers);
			record->nfsProc		= ntohl (rpc_b->ru.RM_cmb.cb_proc);
			hashInsertXid (record->secs, record->rpcXID,
					record->srcHost, record->srcPort,
					record->nfsVersion, record->nfsProc);
			std::cerr<<"nfsProc:"<<record->nfsProc<<std::endl;
		}
		else {
			hash_t *p;
			u_int32_t accepted;
			u_int32_t acceptStatus;

			record->rpcCall		= REPLY;
			record->rpcXID		= ntohl (rpc_b->rm_xid);

			/*
			 * If the RPC was not accepted, dump it.
			 */

			accepted = ntohl (rpc_b->rm_reply.rp_stat);
			acceptStatus = ntohl (*(u_int32_t *) (pp + consumed));
			if (accepted != 0 || acceptStatus != 0) {
/* 				fprintf (OutFile, */
/* 						"XX not accepted (%x, %x)\n", */
/* 						accepted, acceptStatus); */
				print_it = 0;
			}

			/*
			 * If we see an XID we've never seen before,
			 * print an "unknown" record.  Note that we
			 * don't know the actual NFS version -- we
			 * just assume that it is v3.  Since we can't
			 * decode the payload anyway, it doesn't
			 * matter very much what version of the
			 * protocol it was.
			 *
			 * You need to take all of the "unknown"
			 * responses with a grain of salt, because
			 * they might be complete garbage.
			 *
			 * NOTE:  this is a new feature!  Before
			 * 3/23/03, unmatched responses would simply
			 * be dumped.
			 */
			p = hashLookupXid (record->secs, record->rpcXID,
					record->dstHost, record->dstPort);
			if (p == NULL) {
/* 				fprintf (OutFile, "XX response without call\n"); */
				std::cerr<<"XX response without call"<<std::endl;
				print_it = 1;
				record->nfsVersion	= 3;
				record->nfsProc		= -1;
				record->rpcStatus	= ntohl (*(u_int32_t *)
					(pp + e_len + i_len + h_len + rh_len +
						sizeof (u_int32_t)));
			}
			else {
				record->nfsVersion	= p->nfsVersion;
				record->nfsProc		= p->nfsProc;
				record->rpcStatus	= ntohl (*(u_int32_t *)
					(pp + e_len + i_len + h_len + rh_len +
						sizeof (u_int32_t)));
				free (p);
			}
			/*
			 * Treat the accept_status and call_status
			 * as part of the RPC header, for replies.
			 */

			rh_len += 2 * sizeof (u_int32_t);
			consumed += 2 * sizeof (u_int32_t);
		}

		/*
		 * The payload is everything left in the packet except
		 * the rpc header.
		 */
		payload_len = rpc_len - rh_len;
		record->payload_len = payload_len;
		record->proto = proto;
		record->actual_len = tot_len - consumed;
		if(tot_len>consumed && record->nfsProc!=-1 ){
			TPacketNfs* rec = new TPacketNfs(record2,(void*)(pp+consumed),record2.actual_len);
			m_q.add(rec);
		}
		//else
		//	std::cerr<<"main step 9"<<std::endl;

		/*
		if (payload_len + consumed > tot_len) {
			fprintf (OutFile, "XX too long, must ignore! %d > %d\n",
					payload_len + consumed, tot_len);
		}
		*/
		
/*		if (print_it) {
			printRecord (record, (void *) (pp + consumed),
					payload_len, proto,
					tot_len - consumed);

			if (euid != -1 && egid != -1) {
				fprintf (OutFile, "euid %x egid %x ",
						euid, egid);
			}

			fprintf (OutFile, "con = %d len = %d",
					consumed,
					payload_len + consumed > tot_len ?
						tot_len : payload_len + consumed);

			// For debugging purposes: 
			if (tot_len > payload_len + consumed) {
				fprintf (OutFile, " LONGPKT-%d ",
						tot_len - (payload_len + consumed));
			}

			//
			//  Used to check here to see if the
			//  payload_len + consumed was greater than
			//  tot_len, but this is just wrong for TCP,
			//  where the payload might be spread out over
			//  several IP datagrams.
			/ /

#ifdef	COMMENT
			if (payload_len + consumed > tot_len) {
				fprintf (OutFile, " +++");
			}
#endif	

			fprintf (OutFile, "\n");
		}*/

		consumed += payload_len;

/* 		fprintf (OutFile, "XX consumed = %d, tot_len = %d\n", consumed, tot_len); */

	}
}

static pcap_t* handle = NULL;
static ServerThread* pServerThreadInstance = NULL;

static void sigint_handler(int s)
{
  printf("Gracefully stopping the nfsan\n");

  if (NULL != pServerThreadInstance)
  {
    pServerThreadInstance->generateReport();
    pServerThreadInstance->stop();
  }

  if (NULL != handle)
    pcap_breakloop(handle);
}

int main()
{
  NFSAnConf config;
  int errStage = 0;
  int intStatus;
  char dev_name[128];
  strcpy(dev_name, config.GetPcapDev());
  char errbuf[PCAP_ERRBUF_SIZE];
  bpf_program fExp;
  char fExpString[] = "src or dst port 2049";//"port 2049";// NFS v.3 port number. Both for Tcp and Udp.
  bpf_u_int32 mask;// The netmask of our sniffing device.
  bpf_u_int32 net;// The IP of our sniffing device.

  intStatus = pcap_lookupnet(dev_name, &net, &mask, errbuf);
  ++errStage;

  if (-1 == intStatus)
  {
    printf("Failed to look up netmask for device");
    return errStage;
  }

  handle = pcap_open_live(dev_name, BUFSIZ, 1/* int promisc */, 1000, errbuf);
  ++errStage;

  if (NULL == handle)
  {
    printf("Failed to open: %s\n", dev_name);
    return errStage;
  }

  printf("Network interface is opened!\n");

  intStatus = pcap_compile(handle, &fExp, fExpString, 0/* int optimize */, net);
  ++errStage;

  if (-1 == intStatus)
  {
    printf("Failed to compile filter expression\n");
    pcap_close(handle);
    return errStage;
  }

  printf("Filter expression is compiled!\n");

  intStatus = pcap_setfilter(handle, &fExp);
  ++errStage;

  if (-1 == intStatus)
  {
    printf("Failed to install filter expression\n");
    pcap_close(handle);
    return errStage;
  }

  printf("Filter expression is installed\n");

  ServerThread thread(config);
  pServerThreadInstance = &thread;
  boost::thread server_thread(boost::bind(&ServerThread::run,&thread,(void*)&m_q));

  // Set a handler for SIGINT
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = sigint_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  intStatus = pcap_loop(handle, -1/* int cnt. Negative cnt means work infinetely */, got_packet, 0/* u_char* user */);
  ++errStage;

  if (-1 == intStatus)
  {
    printf("Failed to start pcap loop\n");
    pcap_close(handle);
    return errStage;
  }

//  printf("Started pcap loop!\n");

  errStage = 0;// Normal exit.

//  ServerThread thread;
//  boost::thread server_thread(boost::bind(&ServerThread::run,&thread,(void*)&m_q));
//  server_thread.join();
  return errStage;
}
