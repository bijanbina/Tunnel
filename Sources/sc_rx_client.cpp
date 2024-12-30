#include "sc_rx_client.h"

ScRxClient::ScRxClient(int rx_port, QObject *parent):
    QObject(parent)
{
    port     = rx_port;

    curr_id       = 0;
    read_bufs.resize(SC_MAX_PACKID+1);

    // dbg
    client = new QUdpSocket;

    connect(client, SIGNAL(readyRead()),
            this  , SLOT  (readyRead()));
    connect(client, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT  (error()));
}

void ScRxClient::reset()
{
    curr_id = 0;
    rx_buf.clear();
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

void ScRxClient::readyRead()
{
    QByteArray data;
    data.resize(client->pendingDatagramSize());
    QHostAddress sender_ip;
    quint16 sender_port;

    client->readDatagram(data.data(), data.size(),
                         &sender_ip, &sender_port);
    rx_buf += data;
    processBuf();
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
        read_bufs[buf_id] = rx_buf.mid(SC_LEN_PACKID,
                                       end-SC_LEN_PACKID);
        if( port==ScSetting::dbg_rx_port )
        {
            emit dataReady(read_bufs[buf_id]);
        }
        else
        {
            qDebug() << "ScRxClient::processBuf"
                     << read_bufs[buf_id].length()
                     << "buf_id:" << buf_id
                     << "curr_id:" << curr_id;
            QByteArray pack = getPack();
            if( pack.length() )
            {
                emit dataReady(pack);
            }
        }

        // Remove the processed packet from the buffer
        rx_buf.remove(0, end + strlen(SC_DATA_EOP));
    }
}

void ScRxClient::sendDummy()
{
    client->writeDatagram("a", 1, QHostAddress(
                          ScSetting::remote_host), port);
    qDebug() << "dummy sent";
}
