#include "sc_apache_pcte.h"

ScApachePcTE::ScApachePcTE(QObject *parent):
    QObject(parent)
{
    server   = new QTcpServer;
    tx_con   = new ScTxClient(ScSetting::tx_port);
    rx_con   = new ScRxClient(ScSetting::rx_port);
    tx_dbg   = new ScTxClient(ScSetting::dbg_tx_port);
    tx_timer = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    // client
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

    read_bufs .resize(SC_MAX_PACKID+1);

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txTest()));
    tx_timer->start(10000);
}

ScApachePcTE::~ScApachePcTE()
{
}

void ScApachePcTE::init()
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

    rx_buf.clear();
    read_bufs.clear();
    read_bufs .resize(SC_MAX_PACKID+1);
    rx_con->reset();
    tx_con->reset();
    rx_con->sendDummy();
}

void ScApachePcTE::clientConnected()
{
    if( putInFree() )
    {
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    setupConnection(new_con_id);
    if( rx_buf.length() )
    {
        cons[0]->write(rx_buf);
        rx_buf.clear();
    }
}

void ScApachePcTE::clientError(int id)
{
    qDebug() << id << "clientError"
             << cons[id]->errorString()
             << cons[id]->state()
             << ipv4[id].toString();

    cons[id]->close();
}

void ScApachePcTE::clientDisconnected(int id)
{
    qDebug() << id << "clientDisconnected";
    tx_dbg->write("client_disconnected");
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize    (SC_PC_CONLEN);
    read_bufs .resize(SC_MAX_PACKID+1);
}

void ScApachePcTE::txReadyRead(int id)
{
    QByteArray data = cons[id]->readAll();
//    qDebug() << "read_buf::" << data.length();
    tx_con->write(data);
}

void ScApachePcTE::rxReadyRead(QByteArray data)
{
    int len = cons.length();
    if( len )
    {
        for( int i=0 ; i<len ; i++ )
        {
            if( cons[i]->isOpen() )
            {
                cons[i]->write(data);
                break;
            }
        }
    }
    else
    {
        qDebug() << "rxReadyRead:: Client is closed";
    }
}

void ScApachePcTE::txTest()
{
    int count = 0;
    QByteArray buf;
    int buf_count = 5000;
    buf.clear();
    for( int j=0 ; j<buf_count ; j++ )
    {
        buf += "<";
        buf += QString::number(j).rightJustified(6, '0');
        buf += ">";
    }
    tx_con->write(buf);
    count++;
    qDebug() << "txRefresh" << count;
}

// return id in array where connection is free
int ScApachePcTE::putInFree()
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

void ScApachePcTE::setupConnection(int con_id)
{
    QTcpSocket *con = server->nextPendingConnection();
    cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ScApachePcTE::setupConnection ";
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
    connect(con, SIGNAL(txReadyRead()), mapper_data, SLOT(map()));

    // displayError
    mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            mapper_error, SLOT(map()));

    // disconnected
    mapper_disconnect->setMapping(con, con_id);
    connect(con, SIGNAL(disconnected()),
            mapper_disconnect, SLOT(map()));
}
