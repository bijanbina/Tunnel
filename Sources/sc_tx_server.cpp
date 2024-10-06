#include "sc_tx_server.h"
#include <QThread>

ScTxServer::ScTxServer(QObject *parent):
    QObject(parent)
{
    server  = new QTcpServer;
    timer   = new QTimer;
    curr_id = -1;
    conn_i  = 0;
    tx_buf.resize(SC_MAX_PACKID);
    connect(server,  SIGNAL(newConnection()),
            this  ,  SLOT(txConnected()));

    mapper_error      = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);

    connect(mapper_error, SIGNAL(mapped(int)),
            this        , SLOT(txError(int)));

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (writeBuf()));
    timer->start(SC_TXSERVER_TIMEOUT);
}

ScTxServer::~ScTxServer()
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

void ScTxServer::openPort(int port)
{
    if( server->listen(QHostAddress::Any, port) )
    {
        qDebug() << "created on port "
                 << port;
    }
    else
    {
        qDebug() << "TxServer failed, Error message is:"
                 << server->errorString();
    }
}

void ScTxServer::reset()
{
    curr_id = -1;
    conn_i  = 0;
    buf.clear();
}

void ScTxServer::txConnected()
{
//    qDebug() << tx_cons.length() << "txAccept";
    if( txPutInFree() )
    {
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    txSetupConnection(new_con_id);
    write(""); // to send buff data
}

void ScTxServer::txError(int id)
{
    if( cons[id]->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ApacheSe::txError"
                 << cons[id]->errorString()
                 << cons[id]->state();
        cons[id]->close();
    }
}

void ScTxServer::writeBuf()
{
    if( buf.isEmpty() || cons.isEmpty() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
    int con_len = cons.length();
    for( int i=0 ; i<con_len ; i++ )
    {
        if( cons[conn_i]->isOpen() )
        {
            int len = split_size;
            if( buf.length()<split_size )
            {
                len = buf.length();
            }
            send_buf = buf.mid(0, len);
            addCounter(&send_buf);

            qDebug() << "ScApacheSe::writeBuf curr_id:"
                     << curr_id << len;
            if( sendData(send_buf) )
            {
                buf.remove(0, len);
            }

            if( buf.length()==0 )
            {
                conn_i++;
                if( SC_PC_CONLEN<=conn_i )
                {
                    conn_i = 0;
                }
                return;
            }
        }
        conn_i++;
        if( SC_PC_CONLEN<=conn_i )
        {
            conn_i = 0;
        }
    }
}

void ScTxServer::resendBuf(int id)
{
    QByteArray send_buf;
    int con_len = cons.length();
    for( int i=0 ; i<con_len ; i++ )
    {
        if( cons[conn_i]->isOpen() )
        {
            send_buf = tx_buf[id];

            qDebug() << "ScTxServer::resendBuf id:"
                     << id;
            sendData(send_buf);
            conn_i++;
            if( SC_PC_CONLEN<=conn_i )
            {
                conn_i = 0;
            }
            return;
        }
        conn_i++;
        if( SC_PC_CONLEN<=conn_i )
        {
            conn_i = 0;
        }
    }
}

void ScTxServer::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

// return id in array where connection is free
int  ScTxServer::txPutInFree()
{
    int len = cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 )
        {
//            qDebug() << i << "txPutFree"
//                     << tx_cons[i]->state();
            mapper_error->removeMappings(cons[i]);
            delete cons[i];
            txSetupConnection(i);

            return 1;
        }
        else
        {
            //qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScTxServer::txSetupConnection(int con_id)
{
    QTcpSocket *con = server->nextPendingConnection();
    cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ApacheSe::txSetup";
    if( con_id<ipv4.length() )
    { // put in free
        ipv4[con_id] = QHostAddress(ip_32);
        msg += " refreshing connection";
    }
    else
    {
        ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
        qDebug() << con_id << msg.toStdString().c_str();
    }

    // displayError
    mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            mapper_error, SLOT(map()));
}

void ScTxServer::addCounter(QByteArray *send_buf)
{
    curr_id++;
    QString tx_id = QString::number(curr_id);
    tx_id = tx_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(tx_id.toStdString().c_str());
    tx_buf[curr_id] = *send_buf;
    if( curr_id>SC_MAX_PACKID-1 )
    {
        curr_id = -1;
    }
}

// return 1 when sending data is successful
int ScTxServer::sendData(QByteArray send_buf)
{
    int s = cons[conn_i]->write(send_buf);
    cons[conn_i]->flush();
    cons[conn_i]->close();
    if( s!=send_buf.length() )
    {
        qDebug() << "writeBuf: Error"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}

