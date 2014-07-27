#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "configuration.h"

using namespace std;

static bool isvar(const char* varname, const char* line, char* val)
{
  int len = strlen(varname);

  if (strlen(line) < len + 1)
    return false;

  bool result = 0 == strncmp(varname, line, len) && '=' == line[len];

  const char* p = line + len + 1;

  while ('\0' != *p && '\n' != *p)
    *val++ = *p++;

  *val = '\0';
  return result;
}

NFSAnConf::NFSAnConf() : ok(false), genreptime(10000)
{
  char path[PATH_MAX];
  char dest[PATH_MAX];
  struct stat info;
  pid_t pid = getpid();
  sprintf(path, "/proc/%d/exe", pid);

  if (readlink(path, dest, PATH_MAX) == -1)
    cout << "Failed to determine the executable directory" << endl;
  else
  {
    char* p = strrchr(dest, '/');

    if (p)
      p[0] = '\0';

    exedir = dest;
    repdir = dest;
    pcapdev = "eth0";
    strcat(dest, "/nfsan.conf");
    cout << "Reading configuraton from: " << dest << endl;
    char buf[4096];
    char val[4096];
    ifstream ifs(dest);

    while (ifs.good())
    {
      ifs.getline(buf, 4096);

      if ('#' == buf[0])
        continue;
      else if (isvar("pcapdev", buf, val))
        pcapdev = val;
      else if (isvar("repdir", buf, val))
        repdir = val;
      else if (isvar("genreptime", buf, val))
        sscanf(val, "%d", &genreptime);
      else if (isvar("nfsserver", buf, val))
        nfsServersList.push_back(val);
    }

    cout << "pcapdev=" << pcapdev << endl;
    cout << "repdir=" << repdir << endl;
    cout << "genreptime=" << genreptime << endl;
    ok = true;
  }
}
