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
        QString buf_id_s = rx_buf.mid(0, SC_LEN_PACKID);
        bool    int_ok   = 0;
        int     buf_id   = buf_id_s.toInt(&int_ok);
        int     end      = rx_buf.indexOf(SC_DATA_EOP);

        if( int_ok==0 )
        {
            qDebug() << "ScRxClient::processBuf shit has happened"
                     << buf_id_s << "should be int";
            exit(1);
        }
        if( port!=ScSetting::dbg_rx_port )
        {
//            qDebug() << "b";
        }

        // skip already received packages
        int diff = curr_id - buf_id;
        if( qAbs(diff)<SC_MAX_PACKID/2 )
        {
            if( buf_id<=curr_id )
            {
                // Remove the processed packet from the buffer
                rx_buf.remove(0, end + strlen(SC_DATA_EOP));
                continue;
            }
        }
        else if( buf_id>SC_MAX_PACKID/2 )
        {
            // Remove the processed packet from the buffer
            rx_buf.remove(0, end + strlen(SC_DATA_EOP));
            continue;
        }

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
                     << "buf_id:"  << buf_id
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
