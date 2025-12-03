#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    con       = NULL;
    server    = new QTcpServer;
    tx_con    = new ScTxClient(ScSetting::tx_port);
    rx_con    = new ScRxClient(ScSetting::rx_port);
    rx_dbg    = new ScRxClient(ScSetting::dbg_rx_port);
    ack_timer = new QTimer;
    last_ack  = 0;

    connect(server, SIGNAL(newConnection()),
            this  , SLOT  (clientConnected()));
    connect(rx_con, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (rxReadyRead(QByteArray)));
    connect(rx_dbg, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (dbgReadyRead(QByteArray)));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);

    read_bufs.resize(SC_MAX_PACKID+1);
}

ScApachePC::~ScApachePC()
{
    if( con )
    {
        if( con->isOpen() )
        {
            con->close();
        }
    }
}

void ScApachePC::init()
{
    if( server->listen(QHostAddress::Any, ScSetting::local_port) )
    {
        qDebug() << "ScApachePC Listening on "
                 << ScSetting::local_port;
    }
    else
    {
        qDebug() << "ScApachePC Server failed, Error:"
                 << server->errorString();
    }

    rx_con->write(SC_CMD_START);
    rx_dbg->write(SC_CMD_START);
}

void ScApachePC::reset()
{
    read_bufs.clear();
    read_bufs.resize(SC_MAX_PACKID+1);
    rx_con->reset();
    tx_con->reset();
    rx_dbg->reset();
}

void ScApachePC::clientConnected()
{
    qDebug() << "ScApachePC::clientConnected";
    if( con )
    {
        if( con->isOpen() )
        {
            con->close();
            delete con;
        }
    }
    con = server->nextPendingConnection();
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // signals
    connect(con , SIGNAL(readyRead()),
            this, SLOT(txReadyRead()));
    connect(con , SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(clientError()));
    connect(con , SIGNAL(disconnected()),
            this, SLOT(clientDisconnected()));

    if( rx_buf.length() )
    {
        qDebug() << "ScApachePC::clientConnected Send rx_buf"
                 << rx_buf.length();
        con->write(rx_buf);
        rx_buf.clear();
    }
}

void ScApachePC::clientError()
{
    qDebug() << "clientError"
             << con->errorString()
             << con->state();

    con->close();
}

void ScApachePC::clientDisconnected()
{
    delete con;
    con = NULL;
    qDebug() << "clientDisconnected";
}

void ScApachePC::txReadyRead()
{
    QByteArray data = con->readAll();
//    qDebug() << "read_buf::" << data;
    tx_con->write(data);
}

// send id of last received packet
void ScApachePC::sendAck()
{
    if( con==NULL )
    {
        return;
    }

    if( rx_con->curr_id>0 && con->isOpen() )
    {
        QByteArray msg = SC_CMD_ACK;
        msg += QString::number(rx_con->curr_id);
        rx_con->write(msg);
        qDebug() << "ACK:"  << rx_con->curr_id
                 << "Port:" << rx_con->con->localPort()
                 << "ack_streak:" << ack_streak
                 << "last_ack:"   << last_ack;

        if( last_ack==rx_con->curr_id )
        {
            ack_streak++;

            if( ack_streak>2 )
            {
                ack_timer->setInterval(SC_ACK_TIMEOUT_SLOW);
            }
        }
        else
        {
            ack_timer->setInterval(SC_ACK_TIMEOUT);
            last_ack   = rx_con->curr_id;
            ack_streak = 0;
        }
    }
}

// rx_con
void ScApachePC::rxReadyRead(QByteArray pack)
{
    // dont send ack till u were idle for a while
    ack_timer->start(SC_ACK_TIMEOUT);
    rx_buf += pack;
    if( con==NULL )
    {
        return;
    }

    if( con->isOpen() )
    {
        int ret = con->write(pack);
        if( ret==0 )
        {
            qDebug() << "FAILED PACK:" << pack
                     << "write:" << pack.length() << ret;
        }
        return;
    }
}

// rx_dbg
void ScApachePC::dbgReadyRead(QByteArray data)
{
    if( data.isEmpty() )
    {
        return;
    }
//    qDebug() << "ScApachePC::dbgReadyRead"
//             << data;
    if( data.startsWith(SC_CMD_ACK) )
    {
        int cmd_len = strlen(SC_CMD_ACK);
        data.remove(0, cmd_len);
        int ack_id = data.toInt();
        int resend = sc_needResend(ack_id, tx_con->curr_id);

        if( resend!=-1 )
        {
            tx_con->resendBuf(resend);
            qDebug() << "ScApachePC::dbgReadyRead RESEND_ID:"
                     << resend << "curr_id:" << tx_con->curr_id;
        }
        else if( ack_id!=tx_con->curr_id )
        {
            qDebug() << "ScApachePC::dbgReadyRead RES_FAILED:"
                     << ack_id << "curr_id:" << tx_con->curr_id;
        }
    }
    else
    {
        qDebug() << "ScApachePC::Debug Received Unknown Packet:"
                 << data;
    }
}
