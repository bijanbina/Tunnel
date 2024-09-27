#ifndef SC_BACKEND_H
#define SC_BACKEND_H

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#include <QString>

#define SC_STATE_CLIENT   0
#define SC_STATE_SERVER   1
#define SC_STATE_SE_TEST  2
#define SC_STATE_PC_TEST  2

#define SC_PC_CONLEN    20
#define SC_MAX_PACKID   999
#define SC_LEN_PACKID   3
#define SC_MISS_WINDOW  20 // num of packet missed

#define SC_MIN_PACKLEN  2000
#define SC_MXX_PACKLEN  6990

#define SC_TXSERVER_TIMEOUT  100   //ms
#define SC_TXCLIENT_TIMEOUT  SC_TXSERVER_TIMEOUT
#define SC_PCSIDE_TIMEOUT    100   //ms
#define SC_TEST_TIMEOUT      15000 // how often send test packet
#define SC_TXWRITE_TIMEOUT   100   // how often make sure to send
                                   // all remaining data
#define SC_CMD_RESEND        "resend"

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
