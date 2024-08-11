#include "sc_apache_se.h"

ScApacheSe::ScApacheSe(QString name, QObject *parent):
                       QObject(parent)
{
    con_name  = name; // for debug msg
    tx_client = new ScRemoteClient;
    rx_server = new QTcpServer;
    client    = new QTcpSocket;
    connect(rx_server, SIGNAL(newConnection()),
            this,      SLOT(acceptConnection()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT(rxReadyRead(int)));
//    connect(rx_mapper_error     , SIGNAL(mapped(int)),
//            this                , SLOT(displayError(int)));
//    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
//            this                , SLOT(tcpDisconnected(int)));
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
    client->connectToHost(QHostAddress::LocalHost,
                          ScSetting::local_port);
    client->waitForConnected();
    if( client->isOpen()==0 )
    {
        qDebug() << "WriteBuf: failed connection not opened";
        return;
    }
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // readyRead
    connect(client, SIGNAL(readyRead()),
            this,   SLOT(readyRead()));

    // displayError
    connect(client, SIGNAL(error(QAbstractSocket::SocketError)),
            this,   SLOT(displayError()));

    // disconnected
    connect(client, SIGNAL(disconnected()),
            this,   SLOT(tcpDisconnected()));

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
}

void ScApacheSe::acceptConnection()
{

}

void ScApacheSe::rxAcceptConnection()
{
    if( rxPutInFree() )
    {
        return;
    }
    int new_con_id = rx_cons.length();
    rx_cons.push_back(NULL);
    rxSetupConnection(new_con_id);
    readyRead(); // to send buff data
}

void ScApacheSe::displayError()
 {
    QString msg = "FaApacheSe::" + con_name;
    msg += " Error";
    qDebug() << msg.toStdString().c_str()
             << client->errorString()
             << client->state();

    client->close();

    qDebug() << "FaApacheSe::displayError";
//    if( cons[id]->error()==QTcpSocket::RemoteHostClosedError )
//    {
//    }
}

void ScApacheSe::tcpDisconnected()
{
    QString msg = "FaApacheSe::" + con_name;
    msg += " disconnected";
    qDebug() << msg.toStdString().c_str();
}

void ScApacheSe::readyRead()
{
    read_buf += client->readAll();
    qDebug() << "read_bufs::" << read_buf.length();

    if( rx_ipv4.empty() )
    { // buff till someone connects
        return;
    }

    int split_size = 7000;
    while( read_buf.length() )
    {
        int len = split_size;
        if( read_buf.length()<split_size )
        {
            len = read_buf.length();
        }
        tx_client->buf = read_buf.mid(0, len);
        read_buf.remove(0, len);

        tx_client->writeBuf(rx_ipv4[0]);
    }
}

void ScApacheSe::rxReadyRead(int id)
{
    QByteArray data_rx = rx_cons[id]->readAll();
    client->write(data_rx);
}

//QByteArray ScApacheSe::processBuffer(int id)
//{
//    if( read_bufs[id].contains(FA_START_PACKET)==0 )
//    {
//        return "";
//    }
//    if( read_bufs[id].contains(FA_END_PACKET)==0 )
//    {
//        return "";
//    }
//    int start_index = read_bufs[id].indexOf(FA_START_PACKET);
//    start_index += strlen(FA_START_PACKET);
//    read_bufs[id].remove(0, start_index);

//    int end_index = read_bufs[id].indexOf(FA_END_PACKET);
//    QByteArray data = read_bufs[id].mid(0, end_index);

//    end_index += strlen(FA_END_PACKET);
//    read_bufs[id].remove(0, end_index);

//    return data;
//}

// return id in array where connection is free
int ScApacheSe::rxPutInFree()
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
            qDebug() << "conn is open" << i;
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
    QString msg = "FaApacheSe::" + con_name;
    if( con_id<rx_ipv4.length() )
    { // put in free
        rx_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refereshing connection";
    }
    else
    {
        rx_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
    }
    qDebug() << msg.toStdString().c_str() << con_id
             << rx_ipv4[con_id].toString();

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
