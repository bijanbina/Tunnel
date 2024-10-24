#include "sc_dbg_client.h"

ScDbgClient::ScDbgClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    curr_id = -1;
    conn_i  = 0;
    tx_buf.resize(SC_MAX_PACKID);
    refresh_timer = new QTimer;
    tx_timer      = new QTimer;

    cons.resize(SC_PC_CONLEN);
    cons_count.fill(0, SC_PC_CONLEN);
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

void ScDbgClient::reset()
{
    curr_id = -1;
    buf.clear();
}

void ScDbgClient::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

void ScDbgClient::conRefresh()
{
    int len = cons.length();
    int count = 0;
    int connecting = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 &&
            cons[i]->state()==QTcpSocket::UnconnectedState )
        {
            cons[i]->connectToHost(ScSetting::remote_host,
                                   tx_port);
            cons_count[i] = 0;
            count++;
        }
        else if( cons[i]->state()!=
                 QAbstractSocket::ConnectedState )
        {
            connecting++;
            if( cons[i]->state()!=QTcpSocket::ConnectingState &&
                cons[i]->state()!=QTcpSocket::ClosingState )
            {
                qDebug() << i << "ScDbgClient::conRefresh"
                         << cons[i]->state();
            }
        }
    }

    if( ( count>1 || connecting>10 ) &&
        tx_port!=ScSetting::dbg_tx_port )
    {
        qDebug() << "ScDbgClient::conRefresh port:"
                 << tx_port << "alive:"
                 << cons.length()-count << "count:"
                 << count << "connecting" << connecting;
    }
}

void ScDbgClient::writeBuf()
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

void ScDbgClient::addCounter(QByteArray *send_buf)
{
    curr_id++;
    QString buf_id = QString::number(curr_id);
    buf_id = buf_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(buf_id.toStdString().c_str());
    tx_buf[conn_i] = *send_buf;
    if( curr_id>SC_MAX_PACKID-1 )
    {
        curr_id = -1;
    }
}

// return 1 when sending data is successful
int ScDbgClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
//    qDebug() << "ScDbgClient::sendData send_buf:" << send_buf;
    int s = 0;
    for( int count=0 ; count<SC_PC_CONLEN ; count++ )
    {
        if( cons[conn_i]->isOpen() &&
            cons[conn_i]->state()==QTcpSocket::ConnectedState )
        {
            s = cons[conn_i]->write(send_buf);
            cons_count[conn_i]++;
            if( cons_count[conn_i]>=SC_MAX_PACK )
            {
                cons[conn_i]->disconnectFromHost();
                cons[conn_i]->close();
                conn_i++;
                if( cons.length()<=conn_i )
                {
                    conn_i = 0;
                }
            }

            if( s!=send_buf.length() )
            {
                qDebug() << "writeBuf: Error"
                         << send_buf.length() << s;
                return 0;
            }

            return 1;
        }
        conn_i++;
        if( conn_i>=SC_PC_CONLEN )
        {
            conn_i = 0;
        }
    }

    return 0;
}
