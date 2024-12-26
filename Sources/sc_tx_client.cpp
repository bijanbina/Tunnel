#include "sc_tx_client.h"

ScTxClient::ScTxClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    curr_id = -1;
    tx_buf.resize(SC_MAX_PACKID);
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
    if( buf.isEmpty() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
    while( buf.length() )
    {
        int len = split_size;
        if( buf.length()<split_size )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        tx_buf[curr_id] = sc_mkPacket(&send_buf, &curr_id);

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

void ScTxClient::resendBuf(int id)
{
    qDebug() << "ScTxClient::resendBuf curr_id:" << curr_id
             << "id:" << id;
    sendData(tx_buf[id]);
}

// return 1 when sending data is successful
int ScTxClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
    qDebug() << "ScTxClient::sendData send_buf:" << send_buf.length()
             << tx_port <<curr_id;
    int s = 0;
    s = cons->writeDatagram(send_buf,
                            QHostAddress(ScSetting::remote_host),
                            tx_port);

    if( s!=send_buf.length() )
    {
        qDebug() << "ScTxClient::sendData Error:"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}

