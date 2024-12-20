#include "sc_rx_client.h"

ScRxClient::ScRxClient(int rx_port, QObject *parent):
    QObject(parent)
{
    port     = rx_port;
    is_debug = 0;
    if( port==ScSetting::dbg_rx_port )
    {
        is_debug = 0;
    }

    curr_id       = 0;

    connect(client, SIGNAL(readyRead()),
            this  , SLOT  (readyRead()));
    connect(client, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT  (error()));

    rx_buf.resize (SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);
}

ScRxClient::~ScRxClient()
{
    ;
}

void ScRxClient::init()
{
    // dbg
    client = new QUdpSocket;
    client->bind(port);
}

void ScRxClient::readyRead()
{
    if( is_debug )
    {
        dataReady(client->readAll());
    }
    else
    {
        rx_buf += client->readAll();
    }
}

void ScRxClient::error()
{
    if( client->error()!=QUdpSocket::RemoteHostClosedError )
    {
        qDebug() << "ScRxClient::error"
                 << client->state()
                 << client->errorString();
    }
}

void ScRxClient::processBuffer()
{
    if( rx_buf.isEmpty() )
    {
        return;
    }

    QString buf_id_s = rx_buf.mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    qDebug() << "ScRxClient::processBuffer buf_id:"
             << buf_id << "data_len:" << rx_buf.length()
             << "start" << curr_id;
    rx_buf.remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf;
    rx_buf.clear();
    QByteArray pack = getPack();
    if( pack.isEmpty() )
    {
        return;
    }
    emit dataReady(pack);
}

QByteArray ScRxClient::getPack()
{
    QByteArray pack;
    int count = 0;
    while( read_bufs[curr_id].length() )
    {
        pack += read_bufs[curr_id];
        read_bufs[curr_id].clear();
        curr_id++;
        count++;
        if( curr_id>SC_MAX_PACKID )
        {
            curr_id = 0;
        }
        if( count>SC_MAX_PACKID )
        {
            break;
        }
    }

    //    qDebug() << "ScRxClient::getPack start:"
    //             << rx_curr_id-count
    //             << "count:" << count << "tx_con:"
    //             << tx_con->cons.length();
    return pack;
}
