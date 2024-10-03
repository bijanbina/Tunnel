#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    rx_server  = new QTcpServer;
    tx_server  = new ScTxServer;
    dbg_rx     = new QTcpServer;
    dbg_tx     = new ScTxServer;
    ack_timer  = new QTimer;
    rx_curr_id = 0;
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);

    connect(rx_server   , SIGNAL(newConnection()),
            this        , SLOT(rxConnected()));
    connect(dbg_rx, SIGNAL(newConnection()),
            this        , SLOT(dbgRxConnected()));
    connect(dbg_tx, SIGNAL(newConnection()),
            this        , SLOT(dbgTxConnected()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT(rxReadyRead(int)));
    connect(rx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT(rxError(int)));
    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
            this                , SLOT(rxDisconnected(int)));

    tx_mapper_data     = new QSignalMapper(this);

    dbgrx_mapper_data  = new QSignalMapper(this);
    dbgrx_mapper_error = new QSignalMapper(this);

    connect(dbgrx_mapper_data , SIGNAL(mapped(int)),
            this              , SLOT(dbgRxReadyRead(int)));
    connect(dbgrx_mapper_error, SIGNAL(mapped(int)),
            this              , SLOT(dbgRxError(int)));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);
}

ScApacheSe::~ScApacheSe()
{
    int len = rx_cons.size();
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_cons[i]==NULL )
        {
            continue;
        }
        if( rx_cons[i]->isOpen() )
        {
            rx_cons[i]->close();
        }
    }
}

void ScApacheSe::connectApp()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // readyRead
    connect(&client, SIGNAL(readyRead()),
            this,   SLOT(txReadyRead()));

    // connected
    connect(&client, SIGNAL(error(QAbstractSocket::SocketError)),
            this,    SLOT(clientError()));

    // connected
    connect(&client, SIGNAL(connected()),
            this,    SLOT(clientConnected()));

    // disconnected
    connect(&client, SIGNAL(disconnected()),
            this,    SLOT(clientDisconnected()));

    if( rx_server->listen(QHostAddress::Any, ScSetting::rx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::rx_port;
    }
    else
    {
        qDebug() << "RxServer failed, Error message is:"
                 << rx_server->errorString();
    }

    if( dbg_rx->listen(QHostAddress::Any,
                           ScSetting::dbg_rx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::dbg_rx_port;
    }
    else
    {
        qDebug() << "DbgServer failed, Error message is:"
                 << dbg_rx->errorString();
    }

    tx_server->openPort(ScSetting::tx_port);
    dbg_tx->openPort(ScSetting::dbg_tx_port);
}

void ScApacheSe::reset()
{
    rx_curr_id = 0;
    tx_server->reset();
    dbg_tx->reset();
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize(rx_cons.length());
    read_bufs.resize(SC_MAX_PACKID+1);
}

void ScApacheSe::clientDisconnected()
{
    qDebug() << "ScApacheSe::Client disconnected----------------";
    reset();
}

void ScApacheSe::clientConnected()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    qDebug() << "ScApacheSe::Client Connected############";
}

void ScApacheSe::clientError()
{
    qDebug() << "FaApacheSe::clientError"
             << client.errorString()
             << client.state();

    if( client.error()!=QTcpSocket::RemoteHostClosedError )
    {
        reset();
        QThread::msleep(100);
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
}

void ScApacheSe::rxConnected()
{
    if( rxPutInFree() )
    {
        return;
    }
    int new_con_id = rx_cons.length();
    rx_cons.push_back(NULL);
    rxSetupConnection(new_con_id);
}

void ScApacheSe::rxError(int id)
{
    //qDebug() << "FaApacheSe::rxError"
    //         << rx_cons[id]->errorString()
    //         << rx_cons[id]->state();

    if( id<rx_cons.length() )
    {
        rx_cons[id]->close();
    }
    //    if( cons[id]->error()==QTcpSocket::RemoteHostClosedError )
    //    {
    //    }
}

void ScApacheSe::rxReadyRead(int id)
{
    rx_buf[id] += rx_cons[id]->readAll();
}

QByteArray ScApacheSe::getPack()
{
    QByteArray pack;
    int count = 0;
    while( read_bufs[rx_curr_id].length() )
    {
        pack += read_bufs[rx_curr_id];
        read_bufs[rx_curr_id].clear();
        rx_curr_id++;
        count++;
        if( rx_curr_id>SC_MAX_PACKID )
        {
            rx_curr_id = 0;
        }
        if( count>SC_MAX_PACKID )
        {
            break;
        }
    }

    qDebug() << "ScApacheSe::getPack start:"
             << rx_curr_id-count << count
             << "rx_len:" << rx_cons.length()
             << "tx_len:" << tx_server->cons.length();

    return pack;
}

void ScApacheSe::rxDisconnected(int id)
{
    if( rx_buf[id].length()==0 )
    {
        return;
    }

    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    rx_buf[id].remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf[id];
    rx_buf[id].clear();

    if( client.isOpen() )
    {
        QByteArray pack = getPack();
        int w = client.write(pack);
        qDebug() << id << "rxDisconnected"
                 << pack.length()
                 << "buf_id:" << buf_id
                 << "rx_curr_id:" << rx_curr_id;
        rx_buf[id].clear();
    }
    else
    {
        qDebug() << id << "ScApacheSe::rxDisconnected"
                 << "client is not open" << rx_buf[id].length();
    }
}

// return id in array where connection is free
int  ScApacheSe::rxPutInFree()
{
    int len = rx_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_cons[i]->isOpen()==0 )
        {
            rx_mapper_data->removeMappings(rx_cons[i]);
            rx_mapper_error->removeMappings(rx_cons[i]);
            rx_mapper_disconnect->removeMappings(rx_cons[i]);
            delete rx_cons[i];
            rxSetupConnection(i);

            return 1;
        }
        else
        {
            //            qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApacheSe::rxSetupConnection(int con_id)
{
    QTcpSocket *con = rx_server->nextPendingConnection();
    rx_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "FaApacheSe::rxSetup";
    if( con_id<rx_ipv4.length() )
    { // put in free
        rx_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refereshing connection";
    }
    else
    {
        rx_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
        qDebug() << con_id << msg.toStdString().c_str()
                 << rx_ipv4[con_id].toString();
    }
    if( con_id>=rx_buf.length() )
    {
        rx_buf.push_back("");
    }

    // readyRead
    rx_mapper_data->setMapping(con, con_id);
    connect(con,            SIGNAL(readyRead()),
            rx_mapper_data, SLOT(map()));

    // displayError
    rx_mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            rx_mapper_error, SLOT(map()));

    // disconnected
    rx_mapper_disconnect->setMapping(con, con_id);
    connect(con,                  SIGNAL(disconnected()),
            rx_mapper_disconnect, SLOT(map()));
}

void ScApacheSe::txReadyRead()
{
    QByteArray data = client.readAll();
    tx_server->write(data);
}

void ScApacheSe::dbgRxConnected()
{
    if( dbgRxPutInFree() )
    {
        return;
    }
    int new_con_id = dbgrx_cons.length();
    dbgrx_cons.push_back(NULL);
    dbgRxSetupConnection(new_con_id);
}

void ScApacheSe::dbgRxReadyRead(int id)
{
    QByteArray dbg_buf = dbgrx_cons[id]->readAll();
    if( dbg_buf.isEmpty() )
    {
        return;
    }

    dbg_buf.remove(0, SC_LEN_PACKID);
    QByteArrayList cmd = dbg_buf.split(SC_CMD_EOP_CHAR);
    for( int i=0 ; i<cmd.length() ; i++ )
    {
        if( cmd[i]==SC_CMD_DISCONNECT )
        {
            // connect and reset automatically
            client.disconnectFromHost();
        }
        else if( cmd[i]==SC_CMD_INIT )
        {
            reset();
            if( client.isOpen() )
            {
                client.disconnectFromHost();
                client.waitForDisconnected();
                client.connectToHost(QHostAddress::LocalHost,
                                     ScSetting::local_port);
            }
            else
            {
                client.connectToHost(QHostAddress::LocalHost,
                                     ScSetting::local_port);
            }
        }
        else if( cmd[i].contains(SC_CMD_ACK) )
        {
            int cmd_len = strlen(SC_CMD_ACK);
            cmd[i].remove(0, cmd_len);
            int     ack_id   = cmd[i].toInt();

            if( ack_id>tx_server->curr_id )
            {
                return;
            }
            tx_server->resendBuf(ack_id);
            qDebug() << "ScApacheSe::ACK"
                     << ack_id << tx_server->curr_id;
            return;
        }
        qDebug() << "ScApacheSe::Debug"
                 << cmd[i];
    }
}

void ScApacheSe::dbgRxError(int id)
{
    if( id<dbgrx_cons.length() )
    {
        dbgrx_cons[id]->close();
        if( dbgrx_cons[id]->error()!=
                QTcpSocket::RemoteHostClosedError )
        {
            qDebug() << "ScApacheSe::dbgError"
                     << dbgrx_cons[id]->errorString()
                     << dbgrx_cons[id]->state();
        }
    }
}

// return id in array where connection is free
int  ScApacheSe::dbgRxPutInFree()
{
    int len = dbgrx_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( dbgrx_cons[i]->isOpen()==0 )
        {
            //            qDebug() << i << "dbgRxPutFree"
            //                     << dbgrx_cons[i]->state();
            dbgrx_mapper_data->removeMappings(dbgrx_cons[i]);
            dbgrx_mapper_error->removeMappings(dbgrx_cons[i]);
            delete dbgrx_cons[i];
            dbgRxSetupConnection(i);

            return 1;
        }
        else
        {
            //qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApacheSe::dbgRxSetupConnection(int con_id)
{
    QTcpSocket *con = dbg_rx->nextPendingConnection();
    dbgrx_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ApacheSe::dbgSetup";
    if( con_id<dbgrx_ipv4.length() )
    { // put in free
        dbgrx_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refreshing connection";
    }
    else
    {
        dbgrx_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
        qDebug() << con_id << msg.toStdString().c_str();
    }

    // readyRead
    dbgrx_mapper_data->setMapping(con, con_id);
    connect(con              , SIGNAL(readyRead()),
            dbgrx_mapper_data, SLOT(map()));

    dbgrx_mapper_error->setMapping(con, con_id);
    connect(con     , SIGNAL(error(QAbstractSocket::SocketError)),
            dbgrx_mapper_error, SLOT(map()));
}

// check if we need to resend a packet
void ScApacheSe::sendAck()
{
    QByteArray msg = SC_CMD_ACK;
    msg += QString::number(rx_curr_id);
    msg += SC_CMD_EOP;
    dbg_tx->write(msg);
}
