#include "nfs.h"
#include <arpa/inet.h>

std::string getStringIp(u_int32_t host){
        unsigned int octet[4]  = {0,0,0,0};
        for (int i=0; i<4; i++)
        {
                octet[i] = ( host >> (i*8) ) & 0xFF;
        }
        std::stringstream st;
        std::string str = "";
        st<<octet[3]<<"."<<octet[2]<<"."<<octet[1]<<"."<<octet[0];
        return st.str();
}

u_int32_t *read_uint32 (u_int32_t *p, u_int32_t *e, u_int32_t *val) {
       if ((p == NULL) || (e < p)) {
                return (NULL);
        }

        if (val != NULL) {
        *val = ntohl (*p);
        }

        return (p + 1);
}
u_int32_t *read_uint64 (u_int32_t *p, u_int32_t *e, u_int64_t *val)
{
	u_int32_t hi, lo;

	if ((p == NULL) || (e < (p + 1))) {
		return (NULL);
	}

	if (val) {
		hi = ntohl (p [0]);
		lo = ntohl (p [1]);
		*val=lo;//need correct
	}

	return (p + 2);
}
u_int32_t *skip_pre_op_attr3 (u_int32_t *p, u_int32_t *e)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	new_p = read_uint64 (new_p, e, NULL);
        new_p = read_uint64 (new_p, e, NULL);//???
        new_p = read_uint64 (new_p, e, NULL);//???
	return (new_p);
}  

u_int32_t *skip_wcc_data3 (u_int32_t *p, u_int32_t *e)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}
	if (e < p) {
		return (NULL);
	}
	if (*new_p++) { 
		new_p = skip_pre_op_attr3 (new_p, e);
	}
	if (*new_p++) { 
		new_p = skip_fattr3 (new_p, e);
	}

	return (new_p);
}
 
u_int32_t *skip_fattr3 (u_int32_t *p, u_int32_t *e){
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}
	if(NULL==NULL){
		new_p = read_uint32 (new_p, e, NULL);	/* ftype */
	        new_p = read_uint32 (new_p, e, NULL);   /* mode */
	        new_p = read_uint32 (new_p, e, NULL);   /* nlink */
	        new_p = read_uint32 (new_p, e, NULL);   /* uid */
	        new_p = read_uint32 (new_p, e, NULL);   /* gid */
	        new_p = read_uint64 (new_p, e, NULL); /* size */
	        new_p = read_uint64 (new_p, e, NULL); /* used */

	        new_p = read_uint32 (new_p, e, NULL);   /* specdata 1 */
        	new_p = read_uint32 (new_p, e, NULL);   /* specdata 2 */

	        new_p = read_uint64 (new_p, e, NULL); /* fsid */
        	new_p = read_uint64 (new_p, e, NULL); /* fileid */
	}
	else {
		//not implemented
                new_p = read_uint32 (new_p, e, NULL);   /* ftype */
                new_p = read_uint32 (new_p, e, NULL);   /* mode */
                new_p = read_uint32 (new_p, e, NULL);   /* nlink */
                new_p = read_uint32 (new_p, e, NULL);   /* uid */
                new_p = read_uint32 (new_p, e, NULL);   /* gid */
                new_p = read_uint64 (new_p, e, NULL); /* size */
                new_p = read_uint64 (new_p, e, NULL); /* used */

                new_p = read_uint32 (new_p, e, NULL);   /* specdata 1 */
                new_p = read_uint32 (new_p, e, NULL);   /* specdata 2 */
        
                new_p = read_uint64 (new_p, e, NULL); /* fsid */
                new_p = read_uint64 (new_p, e, NULL); /* fileid */		
	}

	new_p = (new_p+2);
	new_p = (new_p+2);
	new_p = (new_p+2);
	//new_p = print_nfstime3 (new_p, e, print);	/* atime */
	//new_p = print_nfstime3 (new_p, e, print);	/* mtime */
	//new_p = print_nfstime3 (new_p, e, print);	/* ctime */

	return (new_p); 
}

bool SynchronizedDequeue::add(TPacketNfs* pkt)
 {
  boost::lock_guard<boost::mutex> lock(m_mutex);
 
  m_queue.push_back(pkt);
  return true;
 }

TPacketNfs* SynchronizedDequeue::get()
 {
  boost::lock_guard<boost::mutex> lock(m_mutex);
  if (!m_queue.size())
  {
   return NULL;
  }
  TPacketNfs* pkt = m_queue.front();
  m_queue.pop_front();
  return pkt;
 }

TPacketNfs::TPacketNfs(){
	isInit=false;
}

TPacketNfs::TPacketNfs(nfs_pkt_t _pkt,void* buf,u_int32_t size){
	isInit=true;
	my_size = size;
	memcpy(&pkt,&_pkt,sizeof(nfs_pkt_t));
	my_databuf = (u_int32_t*) malloc (size);
	memcpy(my_databuf,buf,size);
}

TPacketNfs::~TPacketNfs(){
	if(isInit){
                delete my_databuf;
        }
}
