#include "nfs_prim.h"

BaseObject::BaseObject(const char* name, const char* pId, u_int32_t  _secs,u_int32_t  _usecs){
	secs  = _secs;
	usecs = _usecs;
	parentId = std::string(pId);
	m_name = std::string(name);
//	workTime = 0;
	isExUnswer = false;
}

void BaseObject::setWorkTime(u_int32_t  _secs,u_int32_t _usecs){
	if(secs>_secs){
	        std::cerr<<"Error 1 of time work"<<std::endl;
                secs = 0;
                usecs = 0;
                return;
	}
	if(usecs>_usecs){
		if(secs>=_secs){
			std::cerr<<"Error 2 of time work"<<std::endl;
			secs = 0;
			usecs = 0;
			return;
		}
	}
	secs = _secs - secs;
	usecs = _usecs - usecs;
	//std::cerr<<"sec:"<<secs<<" usecs:"<<usecs<<std::endl;
}

double BaseObject::getWorkTime(){
	if(!isExUnswer)
		return 0;
	double res = secs + usecs/1000000.;
	return res;
}

LookUpObject::LookUpObject(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
	: BaseObject("LookUp",pId, _secs, _usecs){
	name = std::string(_name);
	id = "";
}

void LookUpObject::setId(const char* _id){
	id = std::string(_id);
}

const char* LookUpObject::getId(){
	return id.c_str();
}


CreateObject::CreateObject(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Create",pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}

void CreateObject::setId(const char* _id){
        id = std::string(_id);
}

const char* CreateObject::getId(){
        return id.c_str();
}

MkDirObject::MkDirObject(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("MkDir", pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}

void MkDirObject::setId(const char* _id){
        id = std::string(_id);
}

const char* MkDirObject::getId(){
        return id.c_str();
}

GetAttrObj::GetAttrObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("GetAttr",pId, _secs, _usecs){
}

SetAttrObj::SetAttrObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("SetAttr",pId, _secs, _usecs){
}

AccessObj::AccessObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Access",pId, _secs, _usecs){
}

ReadDirObj::ReadDirObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("ReadDir",pId, _secs, _usecs){
}

ReadDirPlusObj::ReadDirPlusObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("ReadDirPlus",pId, _secs, _usecs){
}

FsStatObj::FsStatObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("FsStat",pId, _secs, _usecs){
}

FsInfoObj::FsInfoObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("FsInfo",pId, _secs, _usecs){
}

PathConfObj::PathConfObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("PathConf",pId, _secs, _usecs){
}

CommitObj::CommitObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Commit",pId, _secs, _usecs){
}

RemoveObj::RemoveObj(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Remove",pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}

RmDirObj::RmDirObj(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("RmDir",pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}

RenameObj::RenameObj(const char* pId, u_int32_t  _secs,u_int32_t  _usecs) : BaseObject("rename", pId, _secs, _usecs)
{
}

SymLinkObj::SymLinkObj(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("SymLink",pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}

MkNodObj::MkNodObj(const char* _name, const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("MkNod",pId, _secs, _usecs){
        name = std::string(_name);
        id = "";
}


WriteObj::WriteObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Write",pId, _secs, _usecs){
	countBytes = 0;
}

ReadObj::ReadObj(const char* pId, u_int32_t  _secs, u_int32_t  _usecs)
        : BaseObject("Read",pId, _secs, _usecs){
        countBytes = 0;
}



