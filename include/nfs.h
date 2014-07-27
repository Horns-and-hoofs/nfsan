#ifndef _NFS_INCLUDED
#define _NFS_INCLUDED


#include <iostream>
#include <deque>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <rpc/rpc.h>


using namespace std;


typedef	struct	_hash_t	{
	u_int32_t	rpcXID;
	u_int32_t	srcHost;		/* invoking host */
	u_int32_t	srcPort;		/* port on invoking host */
	u_int32_t	nfsVersion;
	u_int32_t	nfsProc;
	u_int32_t	call_time;	/* For aging. */
	struct _hash_t	*next;
} hash_t;

typedef	struct	{

	u_int32_t	secs, usecs;		/* timestamp */
	u_int32_t	srcHost, dstHost;	/* IP of src and dst hosts */
	u_int32_t	srcPort, dstPort;	/* ports... */
	u_int32_t	rpcXID;

						/* these can probably be squished a bunch. */
	u_int32_t	rpcCall, nfsVersion, nfsProc;
	u_int32_t	rpcStatus;
	//AE 07.04.2014
	u_int32_t       payload_len;
	u_int32_t       actual_len;
	u_int32_t       proto;

	/*
	 * So far-- 11 uint32's replace 14 uint32's + verif + time
	 */

	/* some buffer for the payload. */

}nfs_pkt_t;

u_int32_t *skip_fattr3 (u_int32_t *p, u_int32_t *e);
u_int32_t *read_uint64 (u_int32_t *p, u_int32_t *e, u_int64_t *val);
u_int32_t *read_uint32 (u_int32_t *p, u_int32_t *e, u_int32_t *val);
u_int32_t *skip_pre_op_attr3 (u_int32_t *p, u_int32_t *e);
u_int32_t *skip_wcc_data3 (u_int32_t *p, u_int32_t *e);

std::string getStringIp(u_int32_t host);

hash_t *hashLookupXid (u_int32_t now,
		u_int32_t xid, u_int32_t host, u_int32_t port);

static int get_auth (u_int32_t *ui, int32_t *euid, int32_t *egid);

#define	HASHSIZE	3301
#define	HASH(x,h,p)	(((x) % HASHSIZE + (h) % HASHSIZE + (p) % HASHSIZE) \
				% HASHSIZE)

static	hash_t	*xidHashTable [HASHSIZE];
static  int	xidHashElemCnt	= 0;



typedef	struct	nfs_v3_stat_t {
	unsigned long	c3_total, r3_total;
	unsigned long 	c3_null, r3_null;
	unsigned long 	c3_getattr, r3_getattr;
	unsigned long 	c3_setattr, r3_setattr;
	unsigned long 	c3_lookup, r3_lookup;
	unsigned long 	c3_access, r3_access;
	unsigned long 	c3_readlink, r3_readlink;
	unsigned long 	c3_read, r3_read, r3_read_b, r3_read_m;
	unsigned long 	c3_write, r3_write, c3_write_b, c3_write_m;
	unsigned long 	c3_create, r3_create;
	unsigned long 	c3_mkdir, r3_mkdir;
	unsigned long 	c3_symlink, r3_symlink;
	unsigned long 	c3_mknod, r3_mknod;
	unsigned long 	c3_remove, r3_remove;
	unsigned long 	c3_rmdir, r3_rmdir;
	unsigned long 	c3_rename, r3_rename;
	unsigned long 	c3_link, r3_link;
	unsigned long 	c3_readdir, r3_readdir;
	unsigned long 	c3_readdirp, r3_readdirp;
	unsigned long 	c3_fsstat, r3_fsstat;
	unsigned long 	c3_fsinfo, r3_fsinfo;
	unsigned long 	c3_pathconf, r3_pathconf;
	unsigned long 	c3_commit, r3_commit;
	unsigned long 	c3_unknown, r3_unknown;
} nfs_v3_stat_t;

typedef	struct	nfs_v2_stat_t {
	unsigned long c2_total, r2_total;
	unsigned long c2_null, r2_null;
	unsigned long c2_getattr, r2_getattr;
	unsigned long c2_setattr, r2_setattr;
	unsigned long c2_root, r2_root;
	unsigned long c2_lookup, r2_lookup;
	unsigned long c2_readlink, r2_readlink;
	unsigned long c2_read, r2_read, r2_read_b, r2_read_m;
	unsigned long c2_write, r2_write, c2_write_b, c2_write_m;
	unsigned long c2_writecache, r2_writecache;
	unsigned long c2_create, r2_create;
	unsigned long c2_remove, r2_remove;
	unsigned long c2_rename, r2_rename;
	unsigned long c2_symlink, r2_symlink;
	unsigned long c2_link, r2_link;
	unsigned long c2_mkdir, r2_mkdir;
	unsigned long c2_rmdir, r2_rmdir;
	unsigned long c2_readdir, r2_readdir;
	unsigned long c2_statfs, r2_statfs;
	unsigned long c2_unknown, r2_unknown;
} nfs_v2_stat_t;

 
#define default_packesize 1280 
 
class TPacketNfs
{ 
 int my_size;
 unsigned int ID;
 nfs_pkt_t pkt;
 bool isInit;
 //u_char* my_databuf;
 u_int32_t* my_databuf; 
public:
 TPacketNfs();
 TPacketNfs(nfs_pkt_t _pkt,void* buf, u_int32_t size);
 ~TPacketNfs(); 
 int GetSize() {return my_size;}
 nfs_pkt_t* getStruct(){return &pkt;}
// void SetSize(int size) {my_size = size;}
 unsigned int getID() {return ID;}
 void setID(int id) {ID = id;}
 //void getData(u_int32_t* pbuf) {memcpy(pbuf,my_databuf,my_size);}
 u_int32_t* getData(){return my_databuf;}
 void setData(u_int32_t* pbuf,int size) {my_size=size; memcpy(my_databuf,pbuf,size);}
 void setStruct(nfs_pkt_t _pkt){memcpy(&pkt,&_pkt,sizeof(nfs_pkt_t));}
public: 
// virtual bool IsValid() {return false;}
// virtual bool Encode() {return false;}
// virtual bool Decode() {return false;}
};
 

//queue interface
class ISynchronizedQueue
{
public:
// virtual bool add(TPacketNfs pkt) = 0;
// virtual bool get(TPacketNfs& pkt) = 0;
   virtual bool add(TPacketNfs* pkt) = 0;
   virtual TPacketNfs* get() = 0;
};

class SynchronizedDequeue: public ISynchronizedQueue
{
 boost::mutex m_mutex;
 deque<TPacketNfs*> m_queue;
 boost::condition_variable m_cond;

public: 
bool add(TPacketNfs* pkt);
TPacketNfs* get();
};


#endif
