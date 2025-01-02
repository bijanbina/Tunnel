#ifndef SC_BACKEND_H
#define SC_BACKEND_H

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#include <QString>
#include <QVector>
#include <QByteArray>

#define SC_STATE_CLIENT   0
#define SC_STATE_SERVER   1
#define SC_STATE_SE_TEST  2
#define SC_STATE_PC_TEST  2

#define SC_PC_CONLEN    20
#define SC_MAX_PACKID   999
#define SC_LEN_PACKID   3
#define SC_MAX_PACK     5

#define SC_MIN_PACKLEN  2000
#define SC_MAX_PACKLEN  6990

#define SC_TXSERVER_TIMEOUT  100   //ms
#define SC_TXCLIENT_TIMEOUT  SC_TXSERVER_TIMEOUT
#define SC_PCSIDE_TIMEOUT    100   //ms
#define SC_TEST_TIMEOUT      15000 // how often send test packet
#define SC_TXWRITE_TIMEOUT   100   // how often make sure to send
                                   // all remaining data
#define SC_ACK_TIMEOUT       200   // how often make sure to check
                                   // all packet are here
#define SC_CONN_TIMEOUT      300

#define SC_CMD_ACK           "ack"
#define SC_CMD_INIT          "init"
#define SC_CMD_EOP           "\n"  // End of packet
#define SC_CMD_EOP_CHAR      '\n'  // End of packet
#define SC_DATA_EOP          "<EOP>\n"

class ScSetting
{
public:
    static int     state;
    static int     local_port;
    static int     tx_port;
    static int     rx_port;
    static int     dbg_rx_port;
    static int     dbg_tx_port;
    static QString password;
    static QString remote_host;
};

QByteArray sc_mkPacket(QByteArray *send_buf, int *count);
int        sc_needResend(int ack, int curr_index);
int        sc_hasPacket(QVector<QByteArray> *buf, int id);

#endif // SC_BACKEND_H
