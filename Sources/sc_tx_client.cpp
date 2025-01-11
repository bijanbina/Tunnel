#include "sc_tx_client.h"

ScTxClient::ScTxClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    curr_id = -1;
    tx_buf.resize(SC_MAX_PACKID+1);
    tx_timer      = new QTimer;

    cons = new QUdpSocket;

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (writeBuf()));
    tx_timer->start(SC_TXWRITE_TIMEOUT);
}

void ScTxClient::reset()
{
    curr_id = -1;
    buf.clear();
}

void ScTxClient::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

void ScTxClient::writeBuf()
{
    QByteArray send_buf;
    while( buf.length() )
    {
        int len = SC_MAX_PACKLEN;
        if( buf.length()<SC_MAX_PACKLEN )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        tx_buf[curr_id] = sc_mkPacket(&send_buf, &curr_id);

        if( sendData(send_buf) )
        {
            buf.remove(0, len);
        }
    }
}

void ScTxClient::resendBuf(int id)
{
    sendData(tx_buf[id]);
}

// return 1 when sending data is successful
int ScTxClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
    cons->waitForBytesWritten();
    int s = cons->writeDatagram(send_buf, ScSetting::remote_host,
                                tx_port);
//    qDebug() << "ScTxClient::sendData:" << s;

    if( s!=send_buf.length() )
    {
        qDebug() << "ScTxClient::sendData Error:"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}

