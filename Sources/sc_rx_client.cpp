#include "sc_rx_client.h"

ScRxClient::ScRxClient(int rx_port, QObject *parent):
    QObject(parent)
{
    port    = rx_port;

    curr_id = -1;
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
    curr_id = -1;
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
    while( client->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(client->pendingDatagramSize());

        QHostAddress sender;
        quint16 sender_port;

        client->readDatagram(data.data(), data.size(),
                             &sender, &sender_port);

        rx_buf += data;
    }

    processBuf();
}

void ScRxClient::processBuf()
{
    while( rx_buf.contains(SC_DATA_EOP) )
    {
        ScPacket pack = sc_processPacket(&rx_buf, curr_id);
        if( port!=ScSetting::dbg_rx_port )
        {
//            qDebug() << "b";
        }

        if( pack.skip )
        {
            // skip already received packet
            continue;
        }

        // Save data to buffer
        read_bufs[pack.id] = pack.data;

        if( port==ScSetting::dbg_rx_port )
        {
            emit dataReady(pack.data);
        }
        else
        {
            qDebug() << "ScRxClient::processBuf"
                     << pack.data.length()
                     << "buf_id:"  << pack.id
                     << "curr_id:" << curr_id;
            QByteArray pack = getPack();
            if( pack.length() )
            {
                emit dataReady(pack);
            }
        }
    }
}


QByteArray ScRxClient::getPack()
{
    QByteArray pack;
    int count = 0;
    while( sc_hasPacket(&read_bufs, curr_id) )
    {
        curr_id++;
        if( curr_id>SC_MAX_PACKID )
        {
            curr_id = 0;
        }
        pack += read_bufs[curr_id];
        read_bufs[curr_id].clear();
        count++;
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

void ScRxClient::sendDummy()
{
    int ret = client->writeDatagram("a", 1,
                                    ScSetting::remote_host, port);
    if( ret!=1 )
    {
        qDebug() << "ScRxClient::sendDummy Error:"
                 << client->errorString();
        return;
    }
    client->flush();
    qDebug() << "dummy sent" << port;
}
