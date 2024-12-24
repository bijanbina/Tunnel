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

    rx_buf.resize (SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);

    // dbg
    client = new QUdpSocket;
    client->bind(port);

    connect(client, SIGNAL(readyRead()),
            this  , SLOT  (readyRead()));
    connect(client, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT  (error()));
}

ScRxClient::~ScRxClient()
{
    ;
}

void ScRxClient::readyRead()
{
    QByteArray data;
    data.resize(client->pendingDatagramSize());
    QHostAddress sender_ip;
    quint16 sender_port;

    client->readDatagram(data.data(), data.size(),
                         &sender_ip, &sender_port);
    if( is_debug )
    {
        dataReady(data);
    }
    else
    {
        rx_buf += data;
        processBuf();
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

void ScRxClient::processBuf()
{
    while( rx_buf.contains(SC_DATA_EOP) )
    {
        QString buf_id_s = rx_buf.mid(0, SC_LEN_PACKID);
        int     buf_id   = buf_id_s.toInt();
        int     end      = rx_buf.indexOf(SC_DATA_EOP);

        // Extract the packet including the EOP marker
        read_bufs[buf_id] = rx_buf.mid(SC_LEN_PACKID, end);
        QByteArray pack = getPack();
        qDebug() << "ScRxClient::rxReadyRead"
                 << pack.length()
                 << "buf_id:" << buf_id
                 << "curr_id:" << curr_id;
        emit dataReady(pack);

        // Remove the processed packet from the buffer
        rx_buf.remove(0, end + strlen(SC_DATA_EOP));
    }
}
