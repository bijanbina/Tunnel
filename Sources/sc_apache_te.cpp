#include "sc_apache_te.h"

ScApacheTE::ScApacheTE(QString name, QObject *parent):
    QObject(parent)
{
    con_name = name; // for debug msg
    client   = new ScRemoteClient;
    dbg      = new ScRemoteClient;
    rx_timer = new QTimer;

    // rx
    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT  (rxReadyRead(int)));
    connect(rx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT  (rxError(int)));
    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
            this                , SLOT  (rxDisconnected(int)));

    connect(rx_timer, SIGNAL(timeout()),
            this    , SLOT  (rxRefresh()));
    rx_timer->start(15000);
}

ScApacheTE::~ScApacheTE()
{

}

void ScApacheTE::init()
{
    rx_clients.resize(SC_PC_CONLEN);
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        rx_clients[i] = new QTcpSocket;
        rx_clients[i]->connectToHost(ScSetting::remote_host,
                                     ScSetting::rx_port);
        rx_clients[i]->waitForConnected();
        if( rx_clients[i]->isOpen()==0 )
        {
            qDebug() << "init: failed connection not opened";
            return;
        }
        rx_clients[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);

        // readyRead
        rx_mapper_data->setMapping(rx_clients[i], i);
        connect(rx_clients[i],  SIGNAL(readyRead()),
                rx_mapper_data, SLOT(map()));

        // displayError
        rx_mapper_error->setMapping(rx_clients[i], i);
        connect(rx_clients[i],   SIGNAL(error(QAbstractSocket::SocketError)),
                rx_mapper_error, SLOT(map()));

        // disconnected
        rx_mapper_disconnect->setMapping(rx_clients[i], i);
        connect(rx_clients[i],        SIGNAL(disconnected()),
                rx_mapper_disconnect, SLOT(map()));
    }
}

void ScApacheTE::rxReadyRead(int id)
{
    rx_buf += rx_clients[id]->readAll();
    qDebug() << "rxReadyRead::" << rx_buf;
    rx_buf.clear();
}

void ScApacheTE::rxError(int id)
{
    qDebug() << id << "rxError"
             << rx_clients[id]->state();
}

void ScApacheTE::rxDisconnected(int id)
{
    rx_clients[id]->connectToHost(ScSetting::remote_host,
                                  ScSetting::rx_port);
    rx_clients[id]->waitForConnected();
    if( rx_clients[id]->isOpen()==0 )
    {
        return;
    }
    qDebug() << id << "rxDisconnected:: restored"
             << rx_clients[id]->state();
    rx_clients[id]->setSocketOption(
                QAbstractSocket::LowDelayOption, 1);
}

void ScApacheTE::rxRefresh()
{
    int len = rx_clients.length();
    int count = 0;
    QByteArray buf;
    int buf_count = 1200;
    for( int i=0 ; i<buf_count ; i++ )
    {
        buf += "<";
        buf += QString::number(i);
        buf += ">";
    }
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_clients[i]->isOpen()==0 )
        {
            rx_clients[i]->write(buf);
            count++;
        }
    }
    qDebug() << "rxRefresh" << count;
}
