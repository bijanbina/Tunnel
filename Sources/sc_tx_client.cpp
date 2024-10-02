#include "sc_tx_client.h"

ScTxClient::ScTxClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    counter = 0;
    curr_id = 0;
    tx_buf.resize(SC_MAX_PACKID);
    refresh_timer = new QTimer;
    tx_timer      = new QTimer;

    cons.resize(SC_PC_CONLEN);
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        cons[i] = new QTcpSocket;
        cons[i]->connectToHost(ScSetting::remote_host,
                               tx_port);
        if( cons[i]->isOpen()==0 )
        {
            qDebug() << "init: failed connection not opened";
            return;
        }
        cons[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);
    }

    connect(refresh_timer, SIGNAL(timeout()),
            this         , SLOT  (conRefresh()));
    refresh_timer->start(SC_TXCLIENT_TIMEOUT);

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (writeBuf()));
    tx_timer->start(SC_TXWRITE_TIMEOUT);
}

void ScTxClient::reset()
{
    counter = 0;
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

void ScTxClient::disconnected()
{
    buf.clear();
//    qDebug() << "ScRemoteClient: Disconnected";
}

void ScTxClient::displayError(QAbstractSocket::SocketError
                                  socketError)
{
//    if( socketError==QTcpSocket::RemoteHostClosedError )
//    {
//        return;
//    }

//    qDebug() << "Network error The following error occurred"
//             << remote->errorString();
//    remote->close();
}

void ScTxClient::conRefresh()
{
    int len = cons.length();
    int count = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 &&
            cons[i]->state()!=QTcpSocket::ConnectingState &&
            cons[i]->state()!=QTcpSocket::ClosingState )
        {
            cons[i]->connectToHost(ScSetting::remote_host,
                                   tx_port);
            count++;
        }
    }

    if( count && tx_port!=ScSetting::dbg_tx_port )
    {
        qDebug() << "ScTxClient::conRefresh port:"
                 << tx_port << "alive:"
                 << cons.length()-count << "count:"
                 << count;
    }
}

void ScTxClient::writeBuf()
{
    if( buf.isEmpty() || cons.isEmpty() )
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
    QString buf_id = QString::number(counter);
    buf_id = buf_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(buf_id.toStdString().c_str());
    counter++;
    if( counter>SC_MAX_PACKID )
    {
        counter = 0;
    }
    tx_buf[curr_id] = *send_buf;
}

void ScTxClient::resendBuf(int id)
{
    QByteArray send_buf = tx_buf[id];

    qDebug() << "ScApacheSe::resendBuf curr_id:"
             << id;
    sendData(send_buf);
}
// return 1 when sending data is successful
int ScTxClient::sendData(QByteArray send_buf)
{
    int s = 0;
    for( int count=0 ; count<SC_PC_CONLEN ; count++ )
    {
        if( cons[curr_id]->isOpen() )
        {
            s = cons[curr_id]->write(send_buf);
            cons[curr_id]->disconnectFromHost();
            cons[curr_id]->close();
            curr_id++;
            if( curr_id>=SC_PC_CONLEN )
            {
                curr_id = 0;
            }

            if( s!=send_buf.length() )
            {
                qDebug() << "writeBuf: Error"
                         << send_buf.length() << s;
                return 0;
            }

            return 1;
        }
        curr_id++;
        if( curr_id>=SC_PC_CONLEN )
        {
            curr_id = 0;
        }
    }

    return 0;
}

