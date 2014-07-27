#ifndef _SERVER_INCLUDED
#define _SERVER_INCLUDED

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <deque>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/unordered_map.hpp>
#include "nfs.h"
#include "nfs_prot.h"

#include "nfs_prim.h"
#include "nfsshares.h"
#include "configuration.h"

//std::map<string,string> id2Path;
//extern string getPath(std::string& _id);
//extern void setPath(std::string _id,std::string _path);

class HandlerStorage{
private:
	static boost::mutex m_mutex;
        static std::map<string,string> id2Path;
        static std::map<string,string> path2Id;
        static std::map<std::string,std::string>::iterator iterGetPath;
        static std::map<std::string,std::string>::iterator iterSetPath;
public:
	static std::string getPath(std::string& _id);
        static std::string getId(std::string& _path);
	static void setPath(std::string _id,std::string _path);
	static void deletePath(std::string& _id);
};

class FileObject{
private:
	std::string path;
	std::string fileId;
        nfs_v2_stat_t statv2;
        nfs_v3_stat_t statv3;
	
	deque<LookUpObject*> m_look_up_list;
	deque<CreateObject*> m_create_list;
	deque<MkDirObject*> m_mkdir_list;
	std::map<u_int32_t,deque<BaseObject*>* > m_nfs_objects;
public:
	FileObject(const char* id);
	FileObject(const char* id, const char*_path);
	bool AddObject(u_int32_t id,BaseObject* obj);
	void generateReport(ofstream& myfile);
};

class ClientObject{
private:
	nfs_v2_stat_t statv2;
	//nfs_v3_stat_t statv3;
	std::string srcAddr;
	std::string dstAddr;
	std::map<string,FileObject*> files;
	std::map<u_int32_t,BaseObject*> active_object;
public:
	nfs_v3_stat_t statv3;
	ClientObject(std::string& _srcAddr,std::string& _dstAddr);
	FileObject* getFileObject(std::string& id);
	BaseObject* getActiveObject(u_int32_t id);
	void setActiveObject(u_int32_t id,BaseObject* obj);
	void generateReport(ofstream& myfile);
};

class ServerThread{
private:
	ISynchronizedQueue* pQ;
	u_int32_t reportTime;
	nfs_v2_stat_t statv2;
	bool isRunning;
	FILE *outFile;
	u_int32_t number_out;
	std::string workDir;
//	boost::unordered_map<std::string, std::string> id2Path;
//	nfs_v3_stat_t statv3;
	char	BigBuf0 [2 * 1024];//for v3
	char	BigBuf1 [2 * 1024];//for v3
	char	BigBuf2 [2 * 1024];//for v3
        char	BigBuf3 [2 * 1024];//for v3
	std::map<string,ClientObject*> clients;
	NFSHandleMapper callBack;
        NFSAnConf& config;
//public:
	//static std::map<string,string> id2Path;
	//static std::map<std::string,std::string>::iterator iterGetPath;
	//static std::map<std::string,std::string>::iterator iterSetPath;

public:
	
	explicit ServerThread(NFSAnConf& conf);
	void run(void* var1);
	void stop();
	void storeNfs3Message(nfs_pkt_t *record, u_int32_t* xdr);
	void storeNfs2Message(nfs_pkt_t *record, u_int32_t* xdr);
        void storeNfs3MessageResponse(nfs_pkt_t *record, u_int32_t* xdr);
        void storeNfs2MessageResponse(nfs_pkt_t *record, u_int32_t* xdr);
	ClientObject* getClient(std::string& srcAddr,std::string& dstAddr);
	void setReportTime(int rTime);
	void generateReport();
	//static string getPath(std::string& _id);
	//static void setPath(std::string _id,std::string _path);
	std::string getDir(nfs_fh3 *f);
	std::pair<u_int32_t *,std::string> readDir(u_int32_t *p, u_int32_t *e);
};
//void startServerThread(void* var1);

#endif
