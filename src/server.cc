#include "server.h"
#include "nfs.h"
#include <ctime>
#include "nfs_prim.h"
//#include "nfsshares.h"

std::map<string,string> HandlerStorage::id2Path;
std::map<string,string> HandlerStorage::path2Id;
std::map<std::string,std::string>::iterator HandlerStorage::iterGetPath;
std::map<std::string,std::string>::iterator HandlerStorage::iterSetPath;
boost::mutex HandlerStorage::m_mutex;

void putHandle(const char* handle, const char* path){
	HandlerStorage::setPath(handle,path);
}


FileObject::FileObject(const char* id){
	fileId = std::string(id);
}

FileObject::FileObject(const char* id, const char*_path){
        fileId = std::string(id);
	path = std::string(_path);
}

bool FileObject::AddObject(u_int32_t id, BaseObject* obj){
	std::map<u_int32_t,deque<BaseObject*>* >::iterator it;
	it = m_nfs_objects.find(id);
	deque<BaseObject*> * list_obj= NULL;
        if(it==m_nfs_objects.end()){
        	list_obj=new deque<BaseObject*>();
                std::pair<u_int32_t,deque<BaseObject*>* > myData(id,list_obj);
                m_nfs_objects.insert(myData);		
	}
	else{
		list_obj=it->second;
	}
	list_obj->push_back(obj);
	return true;
	/*
	if(id==NFSPROC3_LOOKUP){
		LookUpObject* ob = static_cast<LookUpObject*> (obj);
		m_look_up_list.push_back(ob);
		return true;
	}
	else if(id==NFSPROC3_CREATE){
                CreateObject* ob = static_cast<CreateObject*> (obj);
                m_create_list.push_back(ob);
                return true;
	}
        else if(id==NFSPROC3_MKDIR){
                MkDirObject* ob = static_cast<MkDirObject*> (obj);
                m_mkdir_list.push_back(ob);
                return true;
        }*/
	//return false;
}

void FileObject::generateReport(ofstream& myfile){
	if(path.size()==0){
		path = HandlerStorage::getPath(fileId);
	}
		
     	myfile<<"FILE path:"<<path<<" id:"<<fileId<<std::endl;
	//std::map<u_int32_t,deque<BaseObject*>* >::iterator it;
	for (std::map<u_int32_t,deque<BaseObject*>* >::iterator it=m_nfs_objects.begin(); it!=m_nfs_objects.end(); ++it){
		u_int32_t id = it->first;
		deque<BaseObject*>* objects = it->second;
		u_int32_t count=0;
		u_int64_t countBytes=0;
		double minV=-1,maxV=0,sumV=0,averV=0;
		std::string name;
		while (!objects->empty()){
                	BaseObject* ob = objects->front();
			name = ob->getNameObject();
                	double wTime = ob->getWorkTime();
                	sumV+=wTime;
                	if(wTime>0 && (minV==-1 || wTime<minV)){
                        	minV = wTime;
                	}
                	if(wTime>maxV)
                        	maxV=wTime;
			countBytes+=ob->getCount();
                	objects->pop_front();
                	count++;
        	}
        	if(count>0){
                	averV = sumV/count;
                	myfile<<name<<": request number:"<<count<< ", minTime:"<<minV<<", maxTime:"<<maxV<<", averageTime:"<<averV;
			if(name.compare("Write")==0 || name.compare("Read")==0)
				myfile<<" bytes:"<<countBytes; 
			myfile<<std::endl;
        	}
	}
/*
	u_int32_t count=0;
	double minV=-1,maxV=0,sumV=0,averV=0;

	
	while (!m_look_up_list.empty())
  	{
		LookUpObject* ob = m_look_up_list.front(); 
		double wTime = ob->getWorkTime();
		sumV+=wTime;
		if(wTime>0 && (minV==-1 || wTime<minV)){
			minV = wTime;
		}
		if(wTime>maxV)
			maxV=wTime;
		m_look_up_list.pop_front();
		count++;
	}		
	if(count>0){
		averV = sumV/count;
		myfile<<"LookUp request number:"<<count<< " minTime:"<<minV<<" maxTime:"<<maxV<<" averageTime:"<<averV<<std::endl;
	}
	// Create request
        count=0;
        minV=-1,maxV=0,sumV=0,averV=0;

        while (!m_create_list.empty())
        {
                CreateObject* ob = m_create_list.front();
                double wTime = ob->getWorkTime();
                sumV+=wTime;
                if(wTime>0 && (minV==-1 || wTime<minV)){
                        minV = wTime;
                }
                if(wTime>maxV)
                        maxV=wTime;
                m_create_list.pop_front();
                count++;
        } 
        if(count>0){
                averV = sumV/count;
                myfile<<"Create request number:"<<count<< " minTime:"<<minV<<" maxTime:"<<maxV<<" averageTime:"<<averV<<std::endl;
        }
	// MKDIR request
        count=0;
        minV=-1,maxV=0,sumV=0,averV=0;

        while (!m_mkdir_list.empty())
        {
                MkDirObject* ob = m_mkdir_list.front();
                double wTime = ob->getWorkTime();
                sumV+=wTime;
                if(wTime>0 && (minV==-1 || wTime<minV)){
                        minV = wTime;
                }
                if(wTime>maxV)
                        maxV=wTime;
                m_mkdir_list.pop_front();
                count++;
        }
        if(count>0){
                averV = sumV/count;
                myfile<<"MkDir request number:"<<count<< " minTime:"<<minV<<" maxTime:"<<maxV<<" averageTime:"<<averV<<std::endl;
        }*/
}


ClientObject::ClientObject(std::string& _srcAddr,std::string& _dstAddr){
	srcAddr=_srcAddr;
	dstAddr=_dstAddr;
}

BaseObject* ClientObject::getActiveObject(u_int32_t id){
	std::map<u_int32_t,BaseObject*>::iterator it;
	it = active_object.find(id);
	//std::cerr<<"getActiveObject key:" <<id<<" count:"<< active_object.size()<<std::endl;
	if(it==active_object.end())
		return NULL;
	BaseObject* obj = it->second;
	//std::cerr<<"getActiveObject1 obj:" <<obj<<std::endl;
	active_object.erase(it);
	return obj;
}

void ClientObject::setActiveObject(u_int32_t id,BaseObject* obj){
	std::map<u_int32_t,BaseObject*>::iterator it;
	it = active_object.find(id);
	if(it!=active_object.end())
		active_object.erase(it);
	std::pair<u_int32_t,BaseObject*> myData(id,obj);
	active_object.insert(myData);
	//std::cerr<<"put active object key:"<<id<<" obj:"<<obj<<" count:"<<active_object.size()<<std::endl;
	//active_object.insert(myData);
}


FileObject* ClientObject::getFileObject(std::string& id){
	std::map<std::string,FileObject*>::iterator it;
	it = files.find(id);
	if(it==files.end()){
		std::string path = HandlerStorage::getPath(id);
		FileObject* obj = new FileObject(id.c_str(),path.c_str());
		std::pair<std::string,FileObject*> myData(id,obj);
		files.insert(myData);
		return obj;
	}
	else
		return it->second;
}

void ClientObject::generateReport(ofstream& myfile){
	myfile<<"CLIENT ipSrc:"<<srcAddr<<" ipDst"<<dstAddr<<std::endl;
               for (std::map<string,FileObject*>::iterator it=files.begin(); it!=files.end(); ++it)
                {
                        it->second->generateReport(myfile);
			myfile<<std::endl;
                        //delete it->second;

                }
                files.clear();
}

ServerThread::ServerThread(NFSAnConf& conf) : callBack(putHandle), config(conf) {
	isRunning = false;
	outFile = fopen ("log.txt" , "w");
	reportTime = 20;
	number_out = 0;
	workDir = "";
	callBack.SetVerbosity(1);
        const list<string> nfsServersList = config.GetNFSServersList();

        for (list<string>::const_iterator i = nfsServersList.begin(); i != nfsServersList.end(); ++i)
	  callBack.SearchForHandles(i->c_str());
	//callBack(putHandle);
}

ClientObject* ServerThread::getClient(std::string& srcAddr,std::string& dstAddr){
	std::string key = srcAddr+"/"+dstAddr;
	std::map<std::string,ClientObject*>::iterator it;
	it = clients.find(key);
	if(it==clients.end()){
		key = dstAddr+"/"+srcAddr;
		it = clients.find(key);
	}
	
	if(it==clients.end()){
		ClientObject* cObj = new ClientObject(srcAddr,dstAddr);
		std::pair<std::string,ClientObject*> myData(key,cObj);
		clients.insert(myData);
		return cObj;
	}
	else
		return it->second;
}

void ServerThread::setReportTime(int rTime){
	reportTime = rTime;
}

void ServerThread::generateReport(){
	std::cerr<<"start generate report"<<std::endl;
	char buffer [100];
	int n=sprintf (buffer, "report%d.txt", number_out++);
	std::string fName(buffer,n);
	ofstream myfile (fName.c_str());
	if (myfile.is_open())
  	{
		for (std::map<string,ClientObject*>::iterator it=clients.begin(); it!=clients.end(); ++it)
		{
			it->second->generateReport(myfile);
			//delete it->second;
			
		}
		clients.clear();
		myfile.close();
	}
	else
		std::cerr<<"can not create file for report with name:"<<fName<<std::endl;
}

std::string HandlerStorage::getPath(std::string& _id){
	boost::lock_guard<boost::mutex> lock(m_mutex);
	//static std::map<std::string,std::string>::iterator itGetPath;
	iterGetPath = id2Path.find(_id);
	if(iterGetPath==id2Path.end()){
		std::string empty;
		return empty;
	}
	else
		return iterGetPath->second;	
}

std::string HandlerStorage::getId(std::string& _path)
{
        boost::lock_guard<boost::mutex> lock(m_mutex);

        map<string, string>::iterator i = path2Id.find(_path);

        if(i == path2Id.end())
        {
          std::string empty;
          return empty;
        }
        else
          return i->second;
}

void HandlerStorage::deletePath(std::string& _id)
{
  boost::lock_guard<boost::mutex> lock(m_mutex);
  std::map<std::string,std::string>::iterator iterDelPath;
  iterDelPath = id2Path.find(_id);

  if(iterDelPath != id2Path.end())
  {
    std::map<std::string,std::string>::iterator i = path2Id.find(iterDelPath->second);

    if (i != path2Id.end())
      path2Id.erase(i);

    id2Path.erase(iterDelPath);
  }
}

void HandlerStorage::setPath(std::string _id,std::string _path)
{
  boost::lock_guard<boost::mutex> lock(m_mutex);
  iterSetPath = id2Path.find(_id);

  if(iterSetPath == id2Path.end())
  {
    std::pair<std::string,std::string> myData(_id,_path);
    id2Path.insert(myData);
    path2Id[_path] = _id;
    //std::cerr<<"store to id2path key:"<<_id<<" value:"<<_path<<std::endl;
  }
}

std::pair<u_int32_t *,std::string> ServerThread::readDir(u_int32_t *p, u_int32_t *e){
	u_int32_t i;
	u_int32_t fh_len;
	u_int32_t n_words;
	if (p == NULL) {
		std::pair<u_int32_t *,std::string> res(NULL,std::string());
		return res;
	}
	/*
	 * Gotta be at least two words-- one for the length, and one
	 * for some actual data.  (usually it's quite a bit longer,
	 * however...).
	 */

	if (e < (p + 2)) {
		std::pair<u_int32_t *,std::string> res(NULL,std::string());
		return res;
	}

	fh_len = (u_int32_t) ntohl (*p++);

	n_words = fh_len / 4;

	if (fh_len & 3) {
		n_words++;
	}
	if (e < (p + n_words)) {
		std::pair<u_int32_t *,std::string> res(NULL,std::string());
		return res;
	}
        char buffer[200];
        int len=0;
	for (i = 0; i < n_words; i++) {
		len += sprintf (buffer+len, "%.8x", ntohl (p [i]));
	}
	u_int32_t * ret1 = (p + n_words);
	std::pair<u_int32_t *,std::string> retVal(ret1,std::string(buffer,len));

	return retVal;	
}

void ServerThread::run(void* lpParam)
{
 std::cerr<<"ServerThread start"<<std::endl;
 //int count=0;
 pQ = (ISynchronizedQueue*)lpParam;
 isRunning=true;
 TPacketNfs* pkt;
 time_t initTime = time(0);
 while(isRunning){
	if((pkt=pQ->get())!= NULL){
		//std::cerr<<"get pkt start"<<std::endl;
		nfs_pkt_t* record = pkt->getStruct();
		if (record->rpcCall == CALL) {
			if (record->nfsVersion == 3) {
				storeNfs3Message(record,pkt->getData());
				//nfs_v3_print_call (record->nfsProc, record->rpcXID,
				//	dp, payload_len, actual_len,
				//	&v3statsBlock);
			}
			else if (record->nfsVersion == 2) {
				storeNfs2Message(record,pkt->getData());
				//nfs_v2_print_call (record->nfsProc, record->rpcXID,
				//	dp, payload_len, actual_len,
				//	&v2statsBlock);
			}
			else {
				fprintf (outFile, "CU%d\n", record->nfsVersion);
			}
		}
		else {
			if (record->nfsVersion == 3) {
				storeNfs3MessageResponse(record,pkt->getData());
				//nfs_v3_print_resp (record->nfsProc, record->rpcXID,
				//	dp, payload_len, actual_len,
				//	record->rpcStatus,
				//	&v3statsBlock);
			}
			else if (record->nfsVersion == 2) {
				storeNfs2MessageResponse(record,pkt->getData());
				//nfs_v2_print_resp (record->nfsProc, record->rpcXID,
				//	dp, payload_len, actual_len,
				//	record->rpcStatus,
				//	&v2statsBlock);
			}
			else {
				fprintf (outFile, "RU%d\n", record->nfsVersion);
			}
			
			fprintf (outFile, "status=%x ", record->rpcStatus);

			fprintf (outFile, "pl = %d ", record->payload_len);
		}
		delete pkt;
	}
	else
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	time_t curTime = time(0);
	if((curTime-initTime)>reportTime){
		generateReport();
		initTime = time(0);
	}
 }

}

std::string ServerThread::getDir(nfs_fh3 *f){
	char buffer[200];
	int len=0;
	for (int i = 0; i < f->data.data_len; i++) {
		len += sprintf (buffer+len, "%.2x", 0xff & f->data.data_val [i]);
	}
	return std::string(buffer,len);
}

void ServerThread::storeNfs3Message(nfs_pkt_t *record, u_int32_t* p)
{
	//std::cerr<<std::hex<<"srcUint: "<<record->srcHost<<" dstUint:"<<record->dstHost<<std::endl;
	std::string srcIp(getStringIp(record->srcHost));
	//std::string dstIp;
	std::string dstIp(getStringIp(record->dstHost));
		
	ClientObject* obj = getClient(srcIp,dstIp);
	//std::cerr<<"srcIp:"<<srcIp<<" dstIp:"<<dstIp<<" ClientObject:"<<obj<<std::endl;
	//std::cerr<<"sec:"<<record->secs<<" usecs:"<<record->usecs<<std::endl;
	//printf ("srcIP:%u.%u.%u.%u",0xff & (srcIp >> 24), 0xff & (srcIp >> 16),0xff & (srcIp >> 8), 0xff & (srcIp >> 0));
        //printf ("dstIP:%u.%u.%u.%u",0xff & (dstIp >> 24), 0xff & (dstIp >> 16),0xff & (dstIp >> 8), 0xff & (dstIp >> 0));

	int rc = 0;
	u_int32_t *e;
	XDR xdr;
	int got_all = 1;


	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	xdrmem_create (&xdr, (char *) p, record->actual_len, XDR_DECODE);

	e = p + (record->actual_len / 4);

	obj->statv3.c3_total++;
	std::cerr<<"command:"<<record->nfsProc<<std::endl;
	//fprintf (outFile, "C3 %-8x %2x ", xid, op);

	switch (record->nfsProc) {
	case NFSPROC3_NULL :	/* OK */
		obj->statv3.c3_null++;
		fprintf (outFile, "%-8s ", "null");
		break;

	case NFSPROC3_GETATTR : { /* OK */
		GETATTR3args args;

		obj->statv3.c3_getattr++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		//fprintf (outFile, "%-8s ", "getattr");

		if ((got_all = xdr_GETATTR3args (&xdr, &args)) != 0) {
			std::string dirName = getDir(&args.object);
			FileObject* fObj = obj->getFileObject(dirName);
			GetAttrObj* nfsobj = new GetAttrObj(dirName.c_str(), record->secs,record->usecs);
			bool res = fObj->AddObject(NFSPROC3_GETATTR,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_GETATTR,nfsobj);
                        } 
			//print_fh3_x (&args.object, "fh");
		}
		break;
	}

	case NFSPROC3_SETATTR : {	/* OK */
		SETATTR3args args;

		obj->statv3.c3_setattr++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		//fprintf (outFile, "%-8s ", "setattr");

		if ((got_all = xdr_SETATTR3args (&xdr, &args)) != 0) {
			std::string dirName = getDir(&args.object);
                        FileObject* fObj = obj->getFileObject(dirName);
                        SetAttrObj* nfsobj = new SetAttrObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_SETATTR,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_SETATTR,nfsobj);
                        }
			//print_fh3_x (&args.object, "fh");
			//print_sattr3_x (&args.new_attributes);
			//print_sattrguard3_x (&args.guard);
		}

		break;
	}

	case NFSPROC3_LOOKUP : {	/* OK */
		LOOKUP3args args;

		obj->statv3.c3_lookup++;
		bzero ((void *) &args, sizeof (args));
		args.what.dir.data.data_val = BigBuf0;
		args.what.name = BigBuf1;

		//fprintf (outFile, "%-8s ", "lookup");
		//std::cerr<<"before lookup"<<std::endl;
		if ((got_all = xdr_LOOKUP3args (&xdr, &args)) != 0) {
			//std::cerr<<"lookup done"<<std::endl;
			//std::string dirName(args.what.dir.data.data_val,args.what.dir.data.data_len);
			std::string dirName = getDir(&args.what.dir);
			std::string nameF(args.what.name,strlen (args.what.name));
//			std::cerr<<"lookup done. Handle:"<<dirName<<std::endl;
			FileObject* fObj = obj->getFileObject(dirName);	
			LookUpObject* nfsobj = new LookUpObject(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
			bool res = fObj->AddObject(NFSPROC3_LOOKUP,nfsobj);
			if(res){
				obj->setActiveObject(NFSPROC3_LOOKUP,nfsobj);
			}
//			std::cerr<<"res="<<res<<std::endl;
			//setPath(dirName,nameF);
			//print_diropargs3_x (&(args.what), "fh", "name");
		}

		break;
	}

	case NFSPROC3_ACCESS : {	/* Probably OK */
		ACCESS3args args;

		obj->statv3.c3_access++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		//fprintf (outFile, "%-8s ", "access");

		if ((got_all = xdr_ACCESS3args (&xdr, &args)) != 0) {
                        std::string dirName = getDir(&args.object);
                        FileObject* fObj = obj->getFileObject(dirName);
                        AccessObj* nfsobj = new AccessObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_ACCESS,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_ACCESS,nfsobj);
                        }
			//print_fh3_x (&(args.object), "fh");
			//print_access3_x (args.access);
		}

		break;
	}

	case NFSPROC3_READLINK : {
		READLINK3args args;

		obj->statv3.c3_readlink++;
		bzero ((void *) &args, sizeof (args));
		args.symlink.data.data_val = BigBuf0;

		fprintf (outFile, "%-8s ", "readlink");

		if ((got_all = xdr_READLINK3args (&xdr, &args)) != 0) {
			//print_fh3_x (&(args.symlink), "fh");
		}

		break;
	}

	case NFSPROC3_READ : {		/* OK */
		READ3args args;

		obj->statv3.c3_read++;
		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;

		//fprintf (outFile, "%-8s ", "read");

		if ((got_all = xdr_READ3args (&xdr, &args)) != 0) {
                        std::string dirName = getDir(&(args.file));
                        FileObject* fObj = obj->getFileObject(dirName);
                        ReadObj* nfsobj = new ReadObj(dirName.c_str(), record->secs,record->usecs);
			nfsobj->setCountBytes(args.count);
                        bool res = fObj->AddObject(NFSPROC3_READ,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_READ,nfsobj);
                        }
			//print_fh3_x (&(args.file), "fh");

			//print_uint64_x ((u_int32_t *) &args.offset, "off");
			//fprintf (outFile, "count %lx ", args.count);
		}

		break;
	}

	case NFSPROC3_WRITE : {		/* OK */
		nfs_fh3	file;
		offset3 offset;
		count3 count;
		stable_how how;

		//obj->statv3.c3_write++;
		std::cerr<<"write recieved"<<std::endl;
//		fprintf (outFile, "%-8s ", "write");

		/*
		 * We can't just gulp down the entire args to write,
		 * because they might have an arbitrary amount of data
		 * following.  Therefore, we get the args one field at
		 * a time.
		 */

		bzero ((void *) &file, sizeof (file));
		file.data.data_val = BigBuf0;
		WriteObj* nfsobj = NULL;
		if ((got_all = xdr_nfs_fh3 (&xdr, &file)) != 0) {
			std::cerr<<"write recieved1"<<std::endl;
                       std::string dirName = getDir(&file);
                        FileObject* fObj = obj->getFileObject(dirName);
                        nfsobj = new WriteObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_WRITE,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_WRITE,nfsobj);
                        }

			//print_fh3_x (&file, "fh");
		}
		std::cerr<<"write recieved2"<<std::endl;
		if ((got_all = xdr_offset3 (&xdr, &offset)) != 0) {
			//print_uint64_x ((u_int32_t *) &offset, "off");
		}
		if ((got_all = xdr_count3 (&xdr, &count)) != 0) {
			std::cerr<<"write recieved3"<<std::endl;
			if(nfsobj==NULL)
				nfsobj = static_cast<WriteObj*> (obj->getActiveObject(NFSPROC3_WRITE));

			if(nfsobj!=NULL){
				std::cerr<<"write bytes:"<<count<<std::endl;
				nfsobj->addCountBytes(count);
			}
//			fprintf (outFile, "count %lx ", count);

//			obj->statv3.c3_write_b += count;
//			if (obj->statv3.c3_write_b >= (1024 * 1024)) {
//				obj->statv3.c3_write_m += obj->statv3.c3_write_b / (1024 * 1024);
//				obj->statv3.c3_write_b %= 1024 * 1024;
//			}
		}
		if ((got_all = xdr_stable_how (&xdr, &how)) != 0) {
			//print_stable_how_x (how);
		}

		break;
	}

	case NFSPROC3_CREATE : {	/* OK */
		CREATE3args args;

		obj->statv3.c3_create++;
		//fprintf (outFile, "%-8s ", "create");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;

		if ((got_all = xdr_CREATE3args (&xdr, &args)) != 0) {
                        std::string dirName = getDir(&args.where.dir);
                        std::string nameF(args.where.name,strlen (args.where.name));
                        std::cerr<<"Create done. Handle:"<<dirName<<std::endl;
                        FileObject* fObj = obj->getFileObject(dirName);
                        CreateObject* nfsobj = new CreateObject(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_CREATE,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_CREATE,nfsobj);
                        }
                        //std::cerr<<"res="<<res<<std::endl;

			//print_diropargs3_x (&args.where, "fh", "name");
			//print_createhow3_x (&args.how);
		}

		break;
	}

	case NFSPROC3_MKDIR : {
		MKDIR3args args;

		obj->statv3.c3_mkdir++;
		//fprintf (outFile, "%-8s ", "mkdir");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;
		
		if ((got_all = xdr_MKDIR3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.where.dir);
                        std::string nameF(args.where.name,strlen (args.where.name));
                        FileObject* fObj = obj->getFileObject(dirName);
                        MkDirObject* nfsobj = new MkDirObject(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_MKDIR,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_MKDIR,nfsobj);
                        }
			//print_diropargs3_x (&args.where, "fh", "name");
			//print_sattr3_x (&args.attributes);
		}

		break;
	}


	case NFSPROC3_SYMLINK : {
		SYMLINK3args args;

		obj->statv3.c3_symlink++;
		fprintf (outFile, "%-8s ", "symlink");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;
		args.symlink.symlink_data = BigBuf2;

		if ((got_all = xdr_SYMLINK3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.where.dir);
                        std::string nameF(args.where.name,strlen (args.where.name));
                        FileObject* fObj = obj->getFileObject(dirName);
                        SymLinkObj* nfsobj = new SymLinkObj(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_SYMLINK,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_SYMLINK,nfsobj);
                        }

			//print_diropargs3_x (&args.where, "fh", "name");
			//print_sattr3_x (&args.symlink.symlink_attributes);
			//fprintf (outFile, "%s ", "sdata");
			//print_string (args.symlink.symlink_data,
			//		strlen (args.symlink.symlink_data));
		}

		break;
	}

	case NFSPROC3_MKNOD : {
		MKNOD3args args;

		obj->statv3.c3_mknod++;
		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;

		//fprintf (outFile, "%-8s ", "mknod");

		if ((got_all = xdr_MKNOD3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.where.dir);
                        std::string nameF(args.where.name,strlen (args.where.name));
                        FileObject* fObj = obj->getFileObject(dirName);
                        MkNodObj* nfsobj = new MkNodObj(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_MKNOD,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_MKNOD,nfsobj);
                        }

			//print_diropargs3_x (&(args.where), "fh", "name");

			/*
			 * There's more, but I'm not going to worry
			 * about it for now.
			 */

		}

		break;
	}

	case NFSPROC3_REMOVE : {	/* OK */
		REMOVE3args args;

		obj->statv3.c3_remove++;
		fprintf (outFile, "%-8s ", "remove");

		bzero ((void *) &args, sizeof (args));
		args.object.dir.data.data_val = BigBuf0;
		args.object.name = BigBuf1;

		if ((got_all = xdr_REMOVE3args (&xdr, &args)) != 0) {
                        std::string dirName = getDir(&args.object.dir);
                        std::string nameF(args.object.name,strlen (args.object.name));
                        FileObject* fObj = obj->getFileObject(dirName);
                        RemoveObj* nfsobj = new RemoveObj(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_REMOVE,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_REMOVE,nfsobj);
                        }

			//print_diropargs3_x (&(args.object), "fh", "name");
		}

		break;
	}

	case NFSPROC3_RMDIR : {
		RMDIR3args args;

		obj->statv3.c3_rmdir++;
		fprintf (outFile, "%-8s ", "rmdir");

		bzero ((void *) &args, sizeof (args));
		args.object.dir.data.data_val = BigBuf0;
		args.object.name = BigBuf1;

		if ((got_all = xdr_RMDIR3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.object.dir);
                        std::string nameF(args.object.name,strlen (args.object.name));
                        FileObject* fObj = obj->getFileObject(dirName);
                        RmDirObj* nfsobj = new RmDirObj(nameF.c_str(), dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_RMDIR,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_RMDIR,nfsobj);
                        }
			//print_diropargs3_x (&(args.object), "fh", "name");
		}

		break;
	}

	case NFSPROC3_RENAME : {
		RENAME3args args;

		obj->statv3.c3_rename++;
		fprintf (outFile, "%-8s ", "rename");

		bzero ((void *) &args, sizeof (args));
		args.from.dir.data.data_val = BigBuf0;
		args.from.name = BigBuf1;
		args.to.dir.data.data_val = BigBuf2;
		args.from.name = BigBuf3;

		if ((got_all = xdr_RENAME3args (&xdr, &args)) != 0) {
                  string fromDirFHStr = getDir(&args.from.dir);
                  string fromDirPath = HandlerStorage::getPath(fromDirFHStr);
                  string toDirFHStr = getDir(&args.to.dir);
                  string toDirPath = HandlerStorage::getPath(toDirFHStr);

                  FileObject* fObj = obj->getFileObject(fromDirFHStr);
                  RenameObj* nfsobj = new RenameObj(fromDirFHStr.c_str(), record->secs, record->usecs);
                  bool res = fObj->AddObject(NFSPROC3_RENAME, nfsobj);

                  if(res)
                    obj->setActiveObject(NFSPROC3_RENAME, nfsobj);

                  if (!fromDirPath.empty() && !toDirPath.empty())
                  {
                    string oldFileName(fromDirPath);
                    oldFileName += '/';
                    oldFileName += args.from.name;
                    string fh = HandlerStorage::getId(oldFileName);

                    if (fh.empty())
                    {
                        oldFileName = fromDirFHStr;
                        oldFileName += '/';
                        oldFileName += args.from.name;
                        fh = HandlerStorage::getId(oldFileName);
                    }

                    if (!fh.empty())
                    {
                      HandlerStorage::deletePath(fh);
                      string newFileName = toDirPath;
                      newFileName += '/';
                      newFileName += args.to.name;
                      HandlerStorage::setPath(fh, newFileName);
                    }
                  }
			//print_diropargs3_x (&args.from, "fh", "name");
			//print_diropargs3_x (&args.to, "fh2", "name2");
		}

		break;
	}

	case NFSPROC3_LINK : {
		LINK3args args;

		obj->statv3.c3_link++;
		fprintf (outFile, "%-8s ", "link");

		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;
		args.link.dir.data.data_val = BigBuf1;
		args.link.name = BigBuf2;

		if ((got_all = xdr_LINK3args (&xdr, &args)) != 0) {
			//print_fh3_x (&args.file, "fh");
			//print_diropargs3_x (&args.link, "fh2", "name");
		}

		break;
	}

	case NFSPROC3_READDIR : {		/* OK */
		READDIR3args args;

		obj->statv3.c3_readdir++;
		fprintf (outFile, "%-8s ", "readdir");

		bzero ((void *) &args, sizeof (args));
		args.dir.data.data_val = BigBuf0;

		if ((got_all = xdr_READDIR3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.dir);
                        FileObject* fObj = obj->getFileObject(dirName);
                        ReadDirObj* nfsobj = new ReadDirObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_READDIR,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_READDIR,nfsobj);
                        }

			//print_fh3_x (&args.dir, "fh");
			//print_uint64_x ((u_int32_t *) &args.cookie, "cookie");
//			fprintf (outFile, "count %lx ", args.count);
		}

		break;
	}

	case NFSPROC3_READDIRPLUS : {
		READDIRPLUS3args args;

		obj->statv3.c3_readdirp++;
		fprintf (outFile, "%-8s ", "readdirp");

		bzero ((void *) &args, sizeof (args));
		args.dir.data.data_val = BigBuf0;

		if ((got_all = xdr_READDIRPLUS3args (&xdr, &args)) != 0) {
			//print_fh3_x (&args.dir, "fh");
                        std::string dirName = getDir(&args.dir);
                        FileObject* fObj = obj->getFileObject(dirName);
                        ReadDirPlusObj* nfsobj = new ReadDirPlusObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_READDIRPLUS,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_READDIRPLUS,nfsobj);
                        }
			//print_uint64_x ((u_int32_t *) &args.cookie, "cookie");
			//fprintf (outFile, "count %lx ", args.dircount);
			//fprintf (outFile, "maxcnt %lx ", args.maxcount);
		}

		break;
	}

	case NFSPROC3_FSSTAT : {
		FSSTAT3args args;

		obj->statv3.c3_fsstat++;
//		fprintf (outFile, "%-8s ", "fsstat");

		bzero ((void *) &args, sizeof (args));
		args.fsroot.data.data_val = BigBuf0;

		if ((got_all = xdr_FSSTAT3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.fsroot);
                        FileObject* fObj = obj->getFileObject(dirName);
                        FsStatObj* nfsobj = new FsStatObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_FSSTAT,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_FSSTAT,nfsobj);
                        }
			//print_fh3_x (&args.fsroot, "fh");
		}

		break;
	}

	case NFSPROC3_FSINFO : {
		FSINFO3args args;

		obj->statv3.c3_fsinfo++;
		//fprintf (outFile, "%-8s ", "fsinfo");

		bzero ((void *) &args, sizeof (args));
		args.fsroot.data.data_val = BigBuf0;

		if ((got_all = xdr_FSINFO3args (&xdr, &args)) != 0) {
                       std::string dirName = getDir(&args.fsroot);
                        FileObject* fObj = obj->getFileObject(dirName);
                        FsInfoObj* nfsobj = new FsInfoObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_FSINFO,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_FSINFO,nfsobj);
                        }

			//print_fh3_x (&args.fsroot, "fh");
		}


		break;
	}

	case NFSPROC3_PATHCONF : {
		PATHCONF3args args;

		obj->statv3.c3_pathconf++;
//		fprintf (outFile, "%-8s ", "pathconf");

		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		if ((got_all = xdr_PATHCONF3args (&xdr, &args)) != 0) {
                      std::string dirName = getDir(&args.object);
                        FileObject* fObj = obj->getFileObject(dirName);
                        PathConfObj* nfsobj = new PathConfObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_PATHCONF,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_PATHCONF,nfsobj);
                        }
			//print_fh3_x (&args.object, "fh");
		}

		break;
	}

	case NFSPROC3_COMMIT : {
		COMMIT3args args;

		obj->statv3.c3_commit++;
//		fprintf (outFile, "%-8s ", "commit");

		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;

		if ((got_all = xdr_COMMIT3args (&xdr, &args)) != 0) {
                      std::string dirName = getDir(&args.file);
                        FileObject* fObj = obj->getFileObject(dirName);
                        CommitObj* nfsobj = new CommitObj(dirName.c_str(), record->secs,record->usecs);
                        bool res = fObj->AddObject(NFSPROC3_COMMIT,nfsobj);
                        if(res){
                                obj->setActiveObject(NFSPROC3_COMMIT,nfsobj);
                        }
			//print_fh3_x (&args.file, "file");
			//print_uint64_x ((u_int32_t *) &args.offset, "off");
//			fprintf (outFile, "count %lx ", args.count);
		}

		break;
	}

	default :
		fprintf (outFile, "unknown ");
		rc = -1;

	}

	xdr_destroy (&xdr);

	if (! got_all) {
		fprintf (outFile, "SHORT PACKET");
	}

}


void ServerThread::storeNfs2Message(nfs_pkt_t *record, u_int32_t* xdr){
}


void ServerThread::storeNfs3MessageResponse(nfs_pkt_t *record, u_int32_t* p)
{
        std::string srcIp(getStringIp(record->srcHost));
	//std::string srcIp;
        std::string dstIp(getStringIp(record->dstHost));
        //std::cerr<<std::hex<<"srcUint: "<<record->srcHost<<" dstUint:"<<record->dstHost<<std::endl;

        ClientObject* obj = getClient(srcIp,dstIp);
        //std::cerr<<"unswer srcIp:"<<srcIp<<" dstIp:"<<dstIp<<" ClientObject:"<<obj<<std::endl;
	std::cerr<<"unswer command:"<<record->nfsProc<<" status:"<<record->rpcStatus<<std::endl;
        //std::cerr<<"sec:"<<record->secs<<" usecs:"<<record->usecs<<std::endl;
        //printf ("srcIP:%u.%u.%u.%u",0xff & (srcIp >> 24), 0xff & (srcIp >> 16),0xff & (srcIp >> 8), 0xff & (srcIp >> 0));
        //printf ("dstIP:%u.%u.%u.%u",0xff & (dstIp >> 24), 0xff & (dstIp >> 16),0xff & (dstIp >> 8), 0xff & (dstIp >> 0));
	u_int32_t *e;
	u_int32_t *new_p = p;
	u_int32_t count;
	int got_all;

	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	e = p + (record->actual_len / 4);

	//fprintf (OutFile, "R3 %8x %2x ", xid, op);

	obj->statv3.r3_total++;
        BaseObject* bObj = obj->getActiveObject(record->nfsProc);
        if(bObj!=NULL){
		bObj->isExUnswer = true;
		bObj->setWorkTime(record->secs,record->usecs);
        }
	else{
		std::cerr<<"Error no found request. Id of request:"<<record->nfsProc<<std::endl;
	}

	switch (record->nfsProc) {
		case NFSPROC3_NULL :
			obj->statv3.r3_null++;
			//fprintf (OutFile, "%-8s ", "null");
			break;

		case NFSPROC3_GETATTR :
			obj->statv3.r3_getattr++;
			//fprintf (OutFile, "%-8s ", "getattr");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK) {
				//new_p = print_fattr3 (p, e, 1);
			}
			break;

		case NFSPROC3_SETATTR :
			obj->statv3.r3_setattr++;
			//fprintf (OutFile, "%-8s ", "setattr");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK) {
				//new_p = print_wcc_data3 (p, e, 1);
			}
			break;

		case NFSPROC3_LOOKUP :
			obj->statv3.r3_lookup++;
			//fprintf (OutFile, "%-8s ", "lookup");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK && bObj!=NULL) {
				LookUpObject* lObj = static_cast<LookUpObject*> (bObj);
				std::pair<u_int32_t *,std::string> answ = readDir(p,e);
				if(answ.first != NULL){
					std::cerr<<"LookUp answer recieved4"<<std::endl;
					lObj->setId(answ.second.c_str());
					std::string newId(answ.second);
					std::cerr<<"New id is:"<<newId<<std::endl;
					std::string pId(lObj->getParentId());
					std::string str = HandlerStorage::getPath(pId);
					if(str.size()>0){
						std::string path = str+"/"+std::string(lObj->getName());
						HandlerStorage::setPath(newId,path);
					}
					else{
						std::string path = std::string(lObj->getParentId())+"/"+std::string(lObj->getName());
						HandlerStorage::setPath(newId,path);
					}
						
				}else{
					std::cerr<<"Error in readDir function."<<std::endl;
					
				}
				//fprintf (OutFile, "fh ");
				//new_p = print_fh3 (p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 0);
			}
			break;

		case NFSPROC3_ACCESS :
			obj->statv3.r3_access++;
			//fprintf (OutFile, "%-8s ", "access");
			//PRINT_STATUS (status, 1);
			//new_p = print_post_op_attr3 (p, e, 0);
			if (record->rpcStatus == NFS3_OK) {
				//fprintf (OutFile, "acc ");
				//new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC3_READLINK :
			obj->statv3.r3_readlink++;
			//fprintf (OutFile, "%-8s ", "readlink");
			//PRINT_STATUS (status, 1);
			//new_p = print_post_op_attr3 (p, e, 0);
			if (record->rpcStatus == NFS3_OK) {
				//fputs ("name ", OutFile);
				//new_p = print_fn3 (new_p, e, 1);
			}
			break;

		case NFSPROC3_READ :
			obj->statv3.r3_read++;
			//fprintf (OutFile, "%-8s ", "read");
			//PRINT_STATUS (status, 1);
			//new_p = print_post_op_attr3 (p, e, 1);
			if (record->rpcStatus == NFS3_OK) {
				//fprintf (OutFile, "count ");
				//new_p = print_count3 (new_p, e, 1, &count);

				obj->statv3.r3_read_b += count;
				if (obj->statv3.r3_read_b >= (1024 * 1024)) {
					obj->statv3.r3_read_m += obj->statv3.r3_read_b / (1024 * 1024);
					obj->statv3.r3_read_b %= 1024 * 1024;
				}

				//fprintf (OutFile, "eof ");
				//new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC3_WRITE :
			//obj->statv3.r3_write++;
			//fprintf (OutFile, "%-8s ", "write");
			//PRINT_STATUS (status, 1);
			new_p = skip_wcc_data3 (p, e);
			if (record->rpcStatus == NFS3_OK) {
				//std::cerr<<"recieved data:"<<std::endl;
				u_int32_t count;
				read_uint32 (new_p, e, &count);
				std::cerr<<"recieved data:"<<count<<std::endl;	
				//fprintf (OutFile, "count ");
				//new_p = print_uint32 (new_p, e, 1, NULL);
				//fprintf (OutFile, "stable ");
				//new_p = print_stable3 (new_p, e, 1);
				/* there's more, but we'll skip it */
			}
			break;

		case NFSPROC3_CREATE :
			obj->statv3.r3_create++;
			//fprintf (OutFile, "%-8s ", "create");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK && bObj!=NULL) {
                                std::cerr<<"Create answer recieved"<<std::endl;
                                CreateObject* lObj = static_cast<CreateObject*> (bObj);
				if (*p) { 
                                        std::pair<u_int32_t *,std::string> answ = readDir(p+1,e);
                                        if(answ.first != NULL ){
                                                //std::cerr<<"LookUp answer recieved4"<<std::endl;
                                                lObj->setId(answ.second.c_str());
                                                std::string newId(answ.second);
                                                std::cerr<<"New id is:"<<newId<<std::endl;
						std::string pId(lObj->getParentId());
                                                std::string str = HandlerStorage::getPath(pId);
                                                if(str.size()>0){
                                                        std::string path = str+"/"+std::string(lObj->getName());
                                                        HandlerStorage::setPath(newId,path);
                                                }
                                                else{
                                                        std::string path = std::string(lObj->getParentId())+"/"+std::string(lObj->getName());
                                                        HandlerStorage::setPath(newId,path);
                                                }
                                        } else{
                                                std::cerr<<"Error in readDir function."<<std::endl;
                                        
					}
                                }
				//fputs ("fh ", OutFile);
				//new_p = print_post_op_fh3 (p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_MKDIR :
			obj->statv3.r3_mkdir++;
			//fprintf (OutFile, "%-8s ", "mkdir");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK) {
                                std::cerr<<"MkDir answer recieved"<<std::endl;
                                if (*p) {
					MkDirObject* lObj = static_cast<MkDirObject*> (bObj);
                                	std::pair<u_int32_t *,std::string> answ = readDir(p+1,e);
                                        if(answ.first != NULL ){
                                                lObj->setId(answ.second.c_str());
                                                std::string newId(answ.second);
                                                std::cerr<<"New id is:"<<newId<<std::endl;
                                                std::string pId(lObj->getParentId());
                                                std::string str = HandlerStorage::getPath(pId);
                                                if(str.size()>0){
                                                        std::string path = str+"/"+std::string(lObj->getName());
                                                        HandlerStorage::setPath(newId,path);
                                                }
                                                else{
                                                        std::string path = std::string(lObj->getParentId())+"/"+std::string(lObj->getName());
                                                        HandlerStorage::setPath(newId,path);
                                                }
                                        }
                                        else{
                                                std::cerr<<"Error in readDir function."<<std::endl;
                                        }
                                }
				//fputs ("fh ", OutFile);
				//new_p = print_post_op_fh3 (p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_SYMLINK :
			obj->statv3.r3_symlink++;
			//fprintf (OutFile, "%-8s ", "symlink");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK) {
				//fputs ("fh ", OutFile);
				//new_p = print_post_op_fh3 (p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_MKNOD :
			obj->statv3.r3_mknod++;
			//fprintf (OutFile, "%-8s ", "mknod");
			//PRINT_STATUS (status, 1);
			if (record->rpcStatus == NFS3_OK) {
				//fputs ("fh ", OutFile);
				//new_p = print_post_op_fh3 (p, e, 1);
				//new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_REMOVE :
			obj->statv3.r3_remove++;
			//fprintf (OutFile, "%-8s ", "remove");
			//PRINT_STATUS (status, 1);
			//new_p = print_wcc_data3 (p, e, 1);
			break;

		case NFSPROC3_RMDIR :
			obj->statv3.r3_rmdir++;
			//fprintf (OutFile, "%-8s ", "rmdir");
			//PRINT_STATUS (status, 1);
			//new_p = print_wcc_data3 (p, e, 1);
			break;

		case NFSPROC3_RENAME :
			obj->statv3.r3_rename++;
			//fprintf (OutFile, "%-8s ", "rename");
			//PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_LINK :
			obj->statv3.r3_link++;
			//fprintf (OutFile, "%-8s ", "link");
			//PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_READDIR : {

			obj->statv3.r3_readdir++;
			//fprintf (OutFile, "%-8s ", "readdir");
			//PRINT_STATUS (status, 1);

			if (record->rpcStatus == NFS3_OK) {
				XDR xdr;
				READDIR3resok res;
				unsigned int name_cnt = 0;

				xdrmem_create (&xdr, (char *) p, (e - p) * sizeof (u_int32_t),
						XDR_DECODE);
				//xdrmem_create (&xdr, (char *) p, record->actual_len, XDR_DECODE);
				//new_p = print_post_op_attr3 (p, e, 1);

				bzero ((void *) &res, sizeof (res));
				if ((got_all = xdr_READDIR3resok (&xdr, &res)) != 0) {
					entry3 *entry = res.reply.entries;

					//fprintf (OutFile, "eof %d ", res.reply.eof);
					while (entry != NULL) {
						//fprintf (OutFile, "fileid-%u ", name_cnt);
						//print_uint64_x ((u_int32_t *) &entry->fileid, NULL);

						//fprintf (OutFile, "name-%u ", name_cnt);
						//print_string (entry->name, strlen (entry->name));

						//fprintf (OutFile, "cookie-%u ", name_cnt);
						//print_uint64_x ((u_int32_t *) &entry->cookie, NULL);

						/* &&& there's more that could be printed */

						entry = entry->nextentry;
						name_cnt++;
					}
				}

				xdr_destroy (&xdr);
			}
			else {
				//new_p = print_post_op_attr3 (p, e, 1);
			}
			break;
		}

		case NFSPROC3_READDIRPLUS : {
			obj->statv3.r3_readdirp++;
			//fprintf (OutFile, "%-8s ", "readdirp");
			//PRINT_STATUS (status, 1);

			if (record->rpcStatus == NFS3_OK) {
				XDR xdr;
				READDIRPLUS3resok res;
				unsigned int name_cnt = 0;

				xdrmem_create (&xdr, (char *) p, (e - p) * sizeof (u_int32_t),
						XDR_DECODE);
				//new_p = print_post_op_attr3 (p, e, 1);

				bzero ((void *) &res, sizeof (res));
				if ((got_all = xdr_READDIRPLUS3resok (&xdr, &res)) != 0) {
					entryplus3 *entry = res.reply.entries;

					//fprintf (OutFile, "eof %d ", res.reply.eof);
                                        string thisDirId(bObj->getParentId());
                                        string parentDirName = HandlerStorage::getPath(thisDirId);

					while (entry != NULL) {
						//fprintf (OutFile, "fileid-%u ", name_cnt);
						//print_uint64_x ((u_int32_t *) &entry->fileid, NULL);

						//fprintf (OutFile, "name-%u ", name_cnt);
						//print_string (entry->name, strlen (entry->name));

						//fprintf (OutFile, "cookie-%u ", name_cnt);
						//print_uint64_x ((u_int32_t *) &entry->cookie, NULL);

						//print_post_op_attr3_p (p, e, 1, name_cnt);
                                                nfs_fh3 fh = entry->name_handle.post_op_fh3_u.handle;
                                                string fhStr = getDir(&fh);

                                                if (!parentDirName.empty())
                                                {
                                                  string fileName(parentDirName);
                                                  fileName += '/';
                                                  fileName += entry->name;
                                                  HandlerStorage::setPath(fhStr, fileName);
                                                }

						entry = entry->nextentry;
						name_cnt++;
					}
				}

				xdr_destroy (&xdr);
			}
			else {
				//new_p = print_post_op_attr3 (p, e, 1);
			}
			break;
		}

		case NFSPROC3_FSSTAT :
			obj->statv3.r3_fsstat++;
			//fprintf (OutFile, "%-8s ", "fsstat");
			//PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_FSINFO :
			obj->statv3.r3_fsinfo++;
			//fprintf (OutFile, "%-8s ", "fsinfo");
			//PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_PATHCONF :
			obj->statv3.r3_pathconf++;
			//fprintf (OutFile, "%-8s ", "pathconf");
			//PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_COMMIT :
			obj->statv3.r3_commit++;
			//fprintf (OutFile, "%-8s ", "commit");
			//PRINT_STATUS (status, 1);
			//new_p = print_wcc_data3 (p, e, 1);
			/* skip the rest */
			break;

		default :
			//fprintf (OutFile, "%-8s ", "unknown");
			break;

	}

	if (new_p == NULL) {
		//fprintf (OutFile, "SHORT PACKET");
		return;
	}

	return;
}

void ServerThread::storeNfs2MessageResponse(nfs_pkt_t *record, u_int32_t* xdr){
}

void ServerThread::stop(){
	isRunning=false;
}
