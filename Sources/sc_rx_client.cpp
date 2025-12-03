#include "sc_rx_client.h"

ScRxClient::ScRxClient(int rx_port, QObject *parent):
    QObject(parent)
{
    port      = rx_port;
    curr_id   = -1;
    last_drop = 0;
    read_bufs.resize(SC_MAX_PACKID+1);

    // dbg
    con = new QUdpSocket;

    connect(con, SIGNAL(readyRead()),
            this  , SLOT  (readyRead()));
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT  (error()));
}

void ScRxClient::reset()
{
    curr_id = -1;
    rx_buf.clear();
}

void ScRxClient::error()
{
    qDebug() << "ScRxClient::error"
             << con->state()
             << con->errorString();
}

void ScRxClient::readyRead()
{
    while( con->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(con->pendingDatagramSize());

        QHostAddress sender;
        quint16 sender_port;

        con->readDatagram(data.data(), data.size(),
                          &sender    , &sender_port);

        rx_buf += data;
    }

    processBuf();
}

void ScRxClient::processBuf()
{
    while( rx_buf.contains(SC_DATA_EOP) )
    {
        ScPacket p = sc_processPacket(&rx_buf, curr_id);

        if( p.skip )
        {
            // skip already received packet
            if( port!=ScSetting::dbg_rx_port )
            {
                qDebug() << "SKIP curr_id:" << curr_id
                         << p.id;
            }
            continue;
        }

        // Save data to buffer
        read_bufs[p.id] = p.data;

        if( port==ScSetting::dbg_rx_port )
        {
            emit dataReady(p.data);
        }
        else // no debug
        {
            QByteArray pack = getPack();
            if( pack.length() )
            {
                qDebug() << "ScRxClient::RX"
                         << p.data.length()
                         << "buf_id:"  << p.id
                         << "curr_id:" << curr_id
                         << "pack_len" << pack.length();
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

    // this only runs in non-debug
    if( (curr_id-last_drop)>SC_MAX_DROP )
    {
        qDebug() << "DROOOOOOOOOOP"
                 << curr_id
                 << "count:" << (curr_id-last_drop);
        con->close();
        write(SC_CMD_DUMMY);
        last_drop = curr_id;
    }

    //    qDebug() << "ScRxClient::getPack start:"
    //             << rx_curr_id-count
    //             << "count:" << count << "tx_con:"
    //             << tx_con->cons.length();
    return pack;
}

// send dummy data to get ip address of receiving side
// as the client IP is dynamic
int ScRxClient::write(QString data)
{
    int ret = con->writeDatagram(data.toStdString().c_str(),
                                 data.length(),
                                 ScSetting::remote_host, port);
    if( ret!=data.length() )
    {
        qDebug() << "ScRxClient::write" << data
                 << "Error:" << con->errorString();
        return ret;
    }
    con->flush();
    return ret;
}
