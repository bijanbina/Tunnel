#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    con       = NULL;
    server    = new QTcpServer;
    tx_con    = new ScTxClient(ScSetting::tx_port);
    rx_con    = new ScRxClient(ScSetting::rx_port);
    tx_dbg    = new ScMetaClient(ScSetting::dbg_tx_port);
    rx_dbg    = new ScRxClient  (ScSetting::dbg_rx_port);
    ack_timer = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    // rx
    connect(rx_con, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (rxReadyRead(QByteArray)));

    // dbg
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
        qDebug() << "created on port "
                 << ScSetting::local_port;
    }
    else
    {
        qDebug() << "Server failed, Error:"
                 << server->errorString();
    }
}

void ScApachePC::reset()
{
    read_bufs.clear();
    read_bufs.resize(SC_MAX_PACKID+1);
    rx_con->reset();
    tx_con->reset();
    rx_dbg->reset();
    rx_con->sendDummy();
    rx_dbg->sendDummy();
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
        qDebug() << "clientConnected rx_buf"
                 << rx_buf.length();
        con->write(rx_buf);
        rx_buf.clear();
    }
    reset();
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
    qDebug() << "clientDisconnected";
}

void ScApachePC::txReadyRead()
{
    QByteArray data = con->readAll();
    //    qDebug() << "read_buf::" << data;
    tx_con->write(data);
}

// rx_con
void ScApachePC::rxReadyRead(QByteArray pack)
{
    rx_buf += pack;
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

// check if we need to resend a packet
void ScApachePC::sendAck()
{
    if( rx_con->curr_id>0 && con->isOpen() )
    {
        QByteArray msg = SC_CMD_ACK;
        msg += QString::number(rx_con->curr_id);
        tx_dbg->write(msg);
    }
}

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
        int resend = sc_resendID(ack_id, tx_con->curr_id);

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
        qDebug() << "ScApachePC::dbgReadyRead cmd:"
                 << data;
    }
}
