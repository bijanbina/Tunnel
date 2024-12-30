#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    server = new QTcpServer;
    tx_con = new ScTxClient(ScSetting::tx_port);
    rx_con = new ScRxClient(ScSetting::rx_port);
    tx_dbg = new ScMetaClient(ScSetting::dbg_tx_port);
    rx_dbg = new ScRxClient  (ScSetting::dbg_rx_port);
    ack_timer     = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    mapper_data       = new QSignalMapper(this);
    mapper_error      = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);

    connect(mapper_data      , SIGNAL(mapped(int)),
            this             , SLOT(txReadyRead(int)));
    connect(mapper_error     , SIGNAL(mapped(int)),
            this             , SLOT(clientError(int)));
    connect(mapper_disconnect, SIGNAL(mapped(int)),
            this             , SLOT(clientDisconnected(int)));

    // rx
    connect(rx_con, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (rxReadyRead(QByteArray)));

    // dbg
    connect(rx_dbg, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (dbgReadyRead(QByteArray)));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);

    read_bufs .resize(SC_MAX_PACKID+1);
}

ScApachePC::~ScApachePC()
{
    int len = cons.size();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]==NULL )
        {
            continue;
        }
        if( cons[i]->isOpen() )
        {
            cons[i]->close();
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
        qDebug() << "Server failed, Error message is:"
                 << server->errorString();
    }
}

void ScApachePC::reset()
{
    rx_buf.clear();
    read_bufs.clear();
    read_bufs .resize(SC_MAX_PACKID+1);
    rx_con->reset();
    tx_con->reset();
    rx_dbg->reset();
    rx_con->sendDummy();
    rx_dbg->sendDummy();
    tx_dbg->write(SC_CMD_INIT SC_CMD_EOP);
}

void ScApachePC::clientConnected()
{
    if( putInFree() )
    {
        reset();
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    setupConnection(new_con_id);
    if( rx_buf.length() )
    {
        qDebug() << "saaalam" << new_con_id;
        cons[0]->write(rx_buf);
        rx_buf.clear();
    }
    reset();
}

void ScApachePC::clientError(int id)
{
    qDebug() << id << "clientError"
             << cons[id]->errorString()
             << cons[id]->state()
             << ipv4[id].toString();

    cons[id]->close();
}

void ScApachePC::clientDisconnected(int id)
{
    qDebug() << id << "clientDisconnected";
}

void ScApachePC::txReadyRead(int id)
{
    QByteArray data = cons[id]->readAll();
    //    qDebug() << "read_buf::" << data;
    tx_con->write(data);
}

void ScApachePC::rxReadyRead(QByteArray pack)
{
    rx_buf += pack;
    int len = cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen() )
        {
            int ret = cons[i]->write(pack);
            if( ret==0 )
            {
                qDebug() << "FAILED PACK:" << pack << i
                         << "write:" << pack.length() << ret;
            }
            return;
        }
    }
}

// check if we need to resend a packet
void ScApachePC::sendAck()
{
    if( rx_con->curr_id>0 )
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
    if( data.contains(SC_CMD_ACK) )
    {
        int cmd_len = strlen(SC_CMD_ACK);
        data.remove(0, cmd_len);
        int ack_id  = data.toInt();

        if( sc_needResend(ack_id, tx_con->curr_id) )
        {
            tx_con->resendBuf(ack_id);
            qDebug() << "ScApachePC::dbgReadyRead ACK_ID:"
                     << ack_id << tx_con->curr_id;
        }
        return;
    }
}

// return id in array where connection is free
int ScApachePC::putInFree()
{
    int len = cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 )
        {
            mapper_data->removeMappings(cons[i]);
            mapper_error->removeMappings(cons[i]);
            mapper_disconnect->removeMappings(cons[i]);
            delete cons[i];
            setupConnection(i);

            return 1;
        }
        else
        {
            qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApachePC::setupConnection(int con_id)
{
    QTcpSocket *con = server->nextPendingConnection();
    cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ScApachePC::setupConnection ";
    if( con_id<ipv4.length() )
    { // put in free
        ipv4[con_id] = QHostAddress(ip_32);
        msg += "refereshing connection";
    }
    else
    {
        ipv4.push_back(QHostAddress(ip_32));
        msg += "accept connection";
    }
    qDebug() << msg.toStdString().c_str() << con_id
             << ipv4[con_id].toString();

    // readyRead
    mapper_data->setMapping(con, con_id);
    connect(con        , SIGNAL(readyRead()),
            mapper_data, SLOT(map()));

    // displayError
    mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            mapper_error, SLOT(map()));

    // disconnected
    mapper_disconnect->setMapping(con, con_id);
    connect(con, SIGNAL(disconnected()),
            mapper_disconnect, SLOT(map()));
}
