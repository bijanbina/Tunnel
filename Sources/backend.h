#ifndef SC_BACKEND_H
#define SC_BACKEND_H

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#include <QString>

#define SC_STATE_CLIENT  0
#define SC_STATE_SERVER  1
#define SC_STATE_TEST    2

#define SC_PC_CONLEN    10
#define SC_MAX_PACKID   999
#define SC_LEN_PACKID   3

class ScSetting
{
public:
    static int     state;
    static int     local_port;
    static int     tx_port;
    static int     rx_port;
    static int     dbg_port;
    static QString password;
    static QString remote_host;
};

#endif // SC_BACKEND_H
