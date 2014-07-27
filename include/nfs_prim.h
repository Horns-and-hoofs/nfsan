#ifndef _NFS_PRIM_INCLUDED
#define _NFS_PRIM_INCLUDED

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include "nfs_prot.h"

typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
//typedef uint32_t  count3;

class BaseObject{
private:
	u_int32_t   secs;
	u_int32_t   usecs;
	//u_int32_t   workTime;
	std::string parentId;
	std::string m_name;
public:
	bool isExUnswer;
public:
	BaseObject(const char* name,const char* pId, u_int32_t  _secs, u_int32_t  _usecs);
	const char* getParentId() {return parentId.c_str();};
	void setInitTime(u_int32_t  _secs,u_int32_t _usecs){secs=_secs;usecs=_usecs;};
	void setWorkTime(u_int32_t  _secs,u_int32_t _usecs);
	double getWorkTime();
	const char* getNameObject(){return m_name.c_str();}; 
	virtual u_int64_t getCount(){return 0;};
};

class LookUpObject : public BaseObject{
private:
	std::string name;
	std::string id;
public:
	LookUpObject(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
	void setId(const char* id);
	const char* getId();
	const char* getName() {return name.c_str();};
};

class CreateObject : public BaseObject{
private:
        std::string name;
        std::string id;
public:
        CreateObject(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* id);
        const char* getId();
        const char* getName() {return name.c_str();};
};

class MkDirObject : public BaseObject{
private:
        std::string name;
        std::string id;
public:
        MkDirObject(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* id);
        const char* getId();
        const char* getName() {return name.c_str();};
};

class GetAttrObj: public BaseObject{
public:
	GetAttrObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class SetAttrObj: public BaseObject{
public:
        SetAttrObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class AccessObj: public BaseObject{
public:
        AccessObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class ReadDirObj: public BaseObject{
public:
        ReadDirObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class ReadDirPlusObj: public BaseObject{
public:
        ReadDirPlusObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class FsStatObj: public BaseObject{
public:
        FsStatObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class FsInfoObj: public BaseObject{
public:
        FsInfoObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class PathConfObj: public BaseObject{
public:
        PathConfObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class CommitObj: public BaseObject{
public:
        CommitObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class RemoveObj: public BaseObject{
private:
        std::string name;
        std::string id;
public:
        RemoveObj(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* _id){id = std::string(_id);};
        const char* getId() {return id.c_str();};
        const char* getName() {return name.c_str();};

};

class RmDirObj: public BaseObject{
private:
        std::string name;
        std::string id;
public:
        RmDirObj(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* _id){id = std::string(_id);};
        const char* getId() {return id.c_str();};
        const char* getName() {return name.c_str();};
};

class RenameObj: public BaseObject{
public:
        RenameObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
};

class SymLinkObj: public BaseObject{
private:
        std::string name;
        std::string id;
public:
        SymLinkObj(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* _id){id = std::string(_id);};
        const char* getId() {return id.c_str();};
        const char* getName() {return name.c_str();};
};

class MkNodObj: public BaseObject{
private:
        std::string name;
        std::string id;
public:
        MkNodObj(const char* _name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setId(const char* _id){id = std::string(_id);};
        const char* getId() {return id.c_str();};
        const char* getName() {return name.c_str();};
};


class WriteObj: public BaseObject{
private:
	u_int64_t countBytes;
public:
        WriteObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
	void addCountBytes(count3 count){countBytes += count;std::cerr<<"add bytes:"<<count<<std::endl;
	std::cerr<<"all bytes:"<<countBytes<<std::endl;};
	u_int64_t getCount() {return countBytes;};
};

class ReadObj: public BaseObject{
private:
        u_int64_t countBytes;
public:
        ReadObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs);
        void setCountBytes(u_int32_t count){countBytes = count;};
        u_int64_t getCount() {return countBytes;};
};


#endif

