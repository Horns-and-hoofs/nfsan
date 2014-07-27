#ifndef NFS_SHARES_INCLUDED
#define NFS_SHARES_INCLUDED

#include <string>
#include <map>
#include <list>
#include <pthread.h>
#include <rpc/rpc.h>
#include "mount.h"
//#include "nfs23.h"
#include "nfs_prot.h"

// Parameters are: handle string, resulting path string.
// Returs true on success and false if handle is not resolved.
typedef void (*result_callback_type) (const char*, const char*);

void* NFSServerCallbackThread(void*);
class NFSServer;
void NFSServerHandleDir(NFSServer*, READDIRPLUS3args*/*, int&*/, std::string);

class NFSServer
{
public:
    explicit NFSServer(const char* url, result_callback_type rcb);
    ~NFSServer();
    //void GetHandlePath(const char* handle);

private:
    std::string url;
    //std::map<std::string, std::string> handleMap;
    //pthread_mutex_t serverMutex;
    //std::list<std::string> requestQueue;
    result_callback_type resultCallback;
    CLIENT* mntClient;
    CLIENT* nfsClient;

    friend void* NFSServerCallbackThread(void*);
    friend void NFSServerHandleDir(NFSServer*, READDIRPLUS3args*/*, int&*/, std::string);
};

class NFSHandleMapper
{
public:
    explicit NFSHandleMapper(result_callback_type rcb);
    ~NFSHandleMapper();
    //void GetHandlePath(const char* url, const char* handle);
    void SearchForHandles(const char* url);
    void SetVerbosity(int newValue);

private:
    std::map<std::string, NFSServer*> serverMap;
    result_callback_type resultCallback;
    pthread_mutex_t mapperMutex;
};

#endif//NFS_SHARES_INCLUDED
