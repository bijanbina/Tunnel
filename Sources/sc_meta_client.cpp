#include "sc_meta_client.h"

ScMetaClient::ScMetaClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port  = port;
    curr_id  = -1;
    tx_timer = new QTimer;

    cons = new QUdpSocket;

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (writeBuf()));
    tx_timer->start(SC_TXWRITE_TIMEOUT);
}

void ScMetaClient::reset()
{
    curr_id = -1;
    buf.clear();
}

void ScMetaClient::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

void ScMetaClient::writeBuf()
{
    if( buf.isEmpty() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MAX_PACKLEN;
    while( buf.length() )
    {
        int len = split_size;
        if( buf.length()<split_size )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        sc_mkPacket(&send_buf, &curr_id);

        if( sendData(send_buf) )
        {
            buf.remove(0, len);
        }
        else
        {
            qDebug() << "-----SC_TX_CLIENT ERROR: DATA LOST-----";
            break;
        }
    }
}

// return 1 when sending data is successful
int ScMetaClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
//    qDebug() << "ScDbgClient::sendData send_buf:" << send_buf;
    int s = cons->writeDatagram(send_buf,
               QHostAddress(ScSetting::remote_host), tx_port);

    if( s!=send_buf.length() )
    {
        qDebug() << "writeBuf: Error"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}
