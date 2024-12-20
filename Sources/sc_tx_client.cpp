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

        addCounter(&send_buf);
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

void ScTxClient::addCounter(QByteArray *send_buf)
{
    curr_id++;
    QString buf_id = QString::number(curr_id);
    buf_id = buf_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(buf_id.toStdString().c_str());
    tx_buf = *send_buf;
    if( curr_id>SC_MAX_PACKID-1 )
    {
        curr_id = -1;
    }
}

void ScTxClient::resendBuf()
{
    QByteArray send_buf = tx_buf;

    qDebug() << "ScApacheSe::resendBuf";
    sendData(send_buf);
}

// return 1 when sending data is successful
int ScTxClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
    //    qDebug() << "ScTxClient::sendData send_buf:" << send_buf;
    int s = 0;
    s = cons->writeDatagram(send_buf,
                            QHostAddress(ScSetting::remote_host),
                            tx_port);

    if( s!=send_buf.length() )
    {
        qDebug() << "writeBuf: Error"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}

