#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>

#include "local.h"

#define MAX_CONNECT_TIMEOUT 10
#define MAX_REMOTE_NUM 10

ScLocal::ScLocal(QObject *parent):
    QObject(parent)
{
    if( ScSetting::state==SC_STATE_SERVER )
    {
        ScApacheSe *server = new ScApacheSe;
        server->connectApp();
    }
    else if( ScSetting::state==SC_STATE_TEST )
    {
        ScApacheTE *te = new ScApacheTE;
        te->init();
    }
    else
    {
        ScApachePC *pc = new ScApachePC;
        pc->init();
    }
}
