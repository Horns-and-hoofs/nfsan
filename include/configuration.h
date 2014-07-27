#ifndef CONFIGURATION_INCLUDED
#define CONFIGURATION_INCLUDED

#include <string>
#include <list>

class NFSAnConf
{
public:
  NFSAnConf();
  bool IsOk() const { return ok; }
  const char* GetPcapDev() const { return pcapdev.c_str(); }
  const char* GetRepDir() const { return repdir.c_str(); }
  int GetGenRepTime() const { return genreptime; }
  const std::list<std::string> GetNFSServersList() const { return nfsServersList; }

private:
  bool ok;
  std::string exedir;
  std::string pcapdev;
  std::string repdir;
  int genreptime;
  std::list<std::string> nfsServersList;
  
};

#endif//CONFIGURATION_INCLUDED
