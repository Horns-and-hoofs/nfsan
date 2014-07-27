#include "nfsshares.h"
#include <iostream>

static const int handles_chunk_size = 200;
static const useconds_t sleep_time = 10000;// In microseconds.
static int verbosity = 0;

using namespace std;

static int allThreadsResult  = 0;

static void Handle2Hex(nfs_fh3 handle, char* hex)
{
    char buffer[200];
    int len=0;	
    for (int i = 0; i < handle.data.data_len; i++) {
 	   len += sprintf (buffer+len, "%.2x", 0xff & handle.data.data_val [i]);
    }
    buffer[len]='\0';	
    strcpy(hex,buffer);	
/*    for (int i = 0; i < handle.data.data_len; ++i)
    {
        char val = handle.data.data_val[i] & 0xFF;
        sprintf(buf, "%X", val);
        hex[2 * i] = buf[0];
        val = handle.data.data_val[i] >> 4;
        sprintf(buf, "%X", val);
        hex[2 * i + 1] = buf[0];
    }

    hex[2 * handle.data.data_len]  = '\0';*/
}

void NFSServerHandleDir(NFSServer* nfsServer, READDIRPLUS3args* rdplsargs/*, int& handledEntries*/, string dirPath)
{
    char hex[256];
    entryplus3* e = 0;
    entryplus3* firstE = 0;
    entryplus3* lastE = 0;
    entryplus3* firstLastE = 0;
    READDIRPLUS3res* rdplsres = 0;

    do
    {
        rdplsres = nfsproc3_readdirplus_3(rdplsargs, nfsServer->nfsClient);
        //cout << "rdplsres=" << rdplsres << endl;

        if (NULL == rdplsres)
            return;

        if (NFS3_OK != rdplsres->status)
            return;

        e = rdplsres->READDIRPLUS3res_u.resok.reply.entries;

        if (0 == firstE)
            firstE = e;

        if (0 != lastE)
          lastE->nextentry = e;

        firstLastE = e;

        while (e)
        {
            lastE = e;
            e = e->nextentry;
        }

        e = firstLastE;

        //cout << "rdplsargs=" << rdplsargs << endl;
        rdplsargs->cookie = lastE->cookie;
        //cout << "rdplsargs->cookieverf=" << rdplsargs->cookieverf << endl;
        memcpy(rdplsargs->cookieverf, rdplsres->READDIRPLUS3res_u.resok.cookieverf, 8);
        //cout << "Exiting directory scan" << endl;
    } while (!rdplsres->READDIRPLUS3res_u.resok.reply.eof);

    e = firstE;

    while (e)
    {
        if (0 == strcmp(e->name, ".") || 0 == strcmp(e->name, ".."))
        {
            e = e->nextentry;
            continue;
        }

        nfs_fh3 fh = e->name_handle.post_op_fh3_u.handle;
        Handle2Hex(fh, hex);
        string fPath(dirPath);
        fPath += "/";
        fPath += e->name;
        //nfsServer->cookie = e->cookie;
        //memcpy(nfsServer->cookieverf, rdplsres->READDIRPLUS3res_u.resok.cookieverf, 8);
        //nfsServer->handleMap[hex] = fPath;

        if (verbosity > 0)
            printf("%s <--> %s\n", hex, fPath.c_str());

        //++handledEntries;
        nfsServer->resultCallback(hex, fPath.c_str());

        /*if (handledEntries >= handles_chunk_size)
        {
            pthread_mutex_lock(&nfsServer->serverMutex);

            for (list<string>::iterator i = nfsServer->requestQueue.begin(); i != nfsServer->requestQueue.end();)
            {
                string handle = *i;
                map<string, string>::iterator j = nfsServer->handleMap.find(handle);

                if (j != nfsServer->handleMap.end())
                {
                    nfsServer->resultCallback(handle.c_str(), j->second.c_str());
                    i = nfsServer->requestQueue.erase(i);
                }
                else
                    ++i;
            }

            pthread_mutex_unlock(&nfsServer->serverMutex);
            handledEntries = 0;
        }*/

        if (e->name_attributes.post_op_attr_u.attributes.type == NF3DIR)
        {
            READDIRPLUS3args rdplsargsSub;
            /*nfs_fh3*/rdplsargsSub.dir = fh;
            /*cookie3*/rdplsargsSub.cookie = 0;
            /*cookieverf3*/memset(rdplsargsSub.cookieverf, 0, 8);
            /*count3*/rdplsargsSub.dircount = 1024;
            rdplsargsSub.maxcount = 10240;
            NFSServerHandleDir(nfsServer, &rdplsargsSub/*, handledEntries*/, fPath);
        }

        e = e->nextentry;
    }
}

void* NFSServerCallbackThread(void* userData)
{
    NFSServer* nfsServer = static_cast<NFSServer*> (userData);

    if (verbosity > 0)
        printf("Started NFSServer thread for %s\n", nfsServer->url.c_str());

    char inbuf[256];
    //int handledEntries = 0;
    void* argp = 0;
    exports exps;
    dirpath mountdir = inbuf;
    mountres3* mountres;

    // 1. Collect information from a given NFS server.
    nfsServer->mntClient = clnt_create(nfsServer->url.c_str(), 100005, 3, "tcp");

    if (0 == nfsServer->mntClient)
    {
        if (verbosity > 0)
            printf("Failed to create mount client. \n");

        goto end;
    }

    nfsServer->nfsClient = clnt_create(nfsServer->url.c_str(), 100003, 3, "tcp");

    if (0 == nfsServer->nfsClient)
    {
        if (verbosity > 0)
            printf("Failed to create nfs client. \n");

        goto end;
    }

    exps = *mountproc3_export_3(argp, nfsServer->mntClient);

    while (exps)
    {
        const char* shareName = exps->ex_dir;
        strcpy(mountdir, shareName);

        if (verbosity > 0)
            printf("Found share: %s\n", shareName);

        mountres = mountproc3_mnt_3(&mountdir, nfsServer->mntClient);

        if (NULL == mountres)
        {
            if (verbosity > 0)
                printf("Failed to mount: %s\n", mountdir);

            exps = exps->ex_next;
            continue;
        }

        if (MNT3_OK != mountres->fhs_status)
        {
            if (verbosity > 0)
                printf("Failed to mount 2: %s status:%d\n", mountdir, mountres->fhs_status);

            exps = exps->ex_next;
            continue;
        }

        READDIRPLUS3args rdplsargs;
        /*nfs_fh3*/rdplsargs.dir.data.data_len = mountres->mountres3_u.mountinfo.fhandle.fhandle3_len;
        rdplsargs.dir.data.data_val = mountres->mountres3_u.mountinfo.fhandle.fhandle3_val;
        /*cookie3*/rdplsargs.cookie = 0;
        /*cookieverf3*/memset(rdplsargs.cookieverf, 0, 8);
        //memcpy(rdplsargs.cookieverf, nfsServer->cookieverf, 8);
        /*count3*/rdplsargs.dircount = 1024;
        rdplsargs.maxcount = 10240;

        string dirPath = shareName;
        char hex[256];
        Handle2Hex(rdplsargs.dir, hex);
        //nfsServer->handleMap[hex] = dirPath;
        nfsServer->resultCallback(hex, dirPath.c_str());
        //++handledEntries;

        if (verbosity > 0)
            printf("%s <--> %s\n", hex, shareName);

        NFSServerHandleDir(nfsServer, &rdplsargs/*, handledEntries*/, dirPath);

        exps = exps->ex_next;
    }

end:
/*stage2:
    // 2. Handling requests.

    while (true)
    {
        pthread_mutex_lock(&nfsServer->serverMutex);

        while (!nfsServer->requestQueue.empty())
        {
            string handle = nfsServer->requestQueue.front();
            map<string, string>::iterator i = nfsServer->handleMap.find(handle);

            if (i != nfsServer->handleMap.end())
                nfsServer->resultCallback(handle.c_str(), i->second.c_str());
            else
                nfsServer->resultCallback(handle.c_str(), "");

            nfsServer->requestQueue.pop_front();
        }

        pthread_mutex_unlock(&nfsServer->serverMutex);
        usleep(sleep_time);
    }*/

    return &allThreadsResult;
}

NFSServer::NFSServer(const char* url, result_callback_type rcb) : url(url), resultCallback(rcb), nfsClient(0), mntClient(0)
{
    if (verbosity > 0)
        printf("Creating new NFSServer\n");

    //pthread_mutex_init(&serverMutex, NULL);
    pthread_t thread;
    pthread_create(&thread, NULL, NFSServerCallbackThread, this);
}

NFSServer::~NFSServer()
{
    //pthread_mutex_destroy(&serverMutex);
}

/*void NFSServer::GetHandlePath(const char* handle)
{
    pthread_mutex_lock(&serverMutex);
    requestQueue.push_back(handle);
    pthread_mutex_unlock(&serverMutex);
}*/

NFSHandleMapper::NFSHandleMapper(result_callback_type rcb) : resultCallback(rcb)
{
    pthread_mutex_init(&mapperMutex, NULL);
    pthread_t thread;
}

NFSHandleMapper::~NFSHandleMapper()
{
    pthread_mutex_lock(&mapperMutex);

    for (map<string, NFSServer*>::iterator i = serverMap.begin(); i != serverMap.end(); ++i)
        delete i->second;

    serverMap.clear();
    pthread_mutex_destroy(&mapperMutex);
}

void NFSHandleMapper::SearchForHandles(const char* url)
{
    pthread_mutex_lock(&mapperMutex);
    map<string, NFSServer*>::iterator i = serverMap.find(url);

    if (i == serverMap.end())
    {
        NFSServer* nfsServer = new NFSServer(url, resultCallback);
        serverMap[url] = nfsServer;
        //nfsServer->GetHandlePath(handle);
        pthread_mutex_unlock(&mapperMutex);
    }

    pthread_mutex_unlock(&mapperMutex);
}

void NFSHandleMapper::SetVerbosity(int newValue)
{
    verbosity = newValue;
}
