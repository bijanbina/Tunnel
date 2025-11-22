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
#define MAX_REMOTE_NUM      10

ScLocal::ScLocal(QObject *parent):
    QObject(parent)
{
    if( ScSetting::state==SC_STATE_SERVER )
    {
        qDebug() << "Starting Server Mode";
        ScApacheSe *server = new ScApacheSe;
        server->connectApp();
    }
    else if( ScSetting::state==SC_STATE_SE_TEST )
    {
        ScApacheTe *te = new ScApacheTe;
        te->connectApp();
    }
    else if( ScSetting::state==SC_STATE_PC_TEST )
    {
        ScApachePcTE *pcte = new ScApachePcTE;
        pcte->init();
    }
    else if( ScSetting::state==SC_STATE_TEST )
    {
        ScTestPc *test_pc = new ScTestPc;
        test_pc->init();
    }
    else if( ScSetting::state==SC_STATE_LIMIT )
    {
        ScTestSe *test_se = new ScTestSe;
        test_se->init();
    }
    else
    {
        ScApachePC *pc = new ScApachePC;
        pc->init();
    }
}
