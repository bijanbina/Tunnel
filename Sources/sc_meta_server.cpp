#include "sc_meta_server.h"
#include <QThread>

ScMetaServer::ScMetaServer(QObject *parent):
    QObject(parent)
{
    server  = new QUdpSocket;
    timer   = new QTimer;
    curr_id = -1;
    tx_buf.resize(SC_MAX_PACKID);

    mapper_error      = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);

    connect(mapper_error, SIGNAL(mapped(int)),
            this        , SLOT(txError(int)));

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (writeBuf()));
    timer->start(SC_TXSERVER_TIMEOUT);
}

ScMetaServer::~ScMetaServer()
{
    if( server==NULL )
    {
        return;
    }
    if( server->isOpen() )
    {
        server->close();
    }
}

void ScMetaServer::openPort(int port)
{
    tx_port = port;
    if( server->bind(QHostAddress::Any, tx_port) )
    {
        qDebug() << "created on port "
                 << tx_port;
    }
    else
    {
        qDebug() << "TxServer failed, Error message is:"
                 << server->errorString();
    }
}

void ScMetaServer::reset()
{
    curr_id = -1;
    conn_i  = 0;
    buf.clear();
}

void ScMetaServer::txConnected()
{
    //    qDebug() << tx_server.length() << "txAccept";
    write(""); // to send buff data
}

void ScMetaServer::txError()
{
    if( server->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << "ApacheSe::txError"
                 << server->errorString()
                 << server->state();
        server->close();
    }
}

void ScMetaServer::writeBuf()
{
    if( buf.isEmpty() || ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
    if( server->isOpen() &&
        server->state()==QTcpSocket::ConnectedState )
    {
        int len = split_size;
        if( buf.length()<split_size )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        addCounter(&send_buf);

        if( tx_port!=ScSetting::dbg_tx_port )
        {
            qDebug() << "ScTxServer::writeBuf curr_id:"
                     << curr_id << "len:" << len;
        }
        if( sendData(send_buf) )
        {
            buf.remove(0, len);
        }

        if( buf.length()==0 )
        {
            return;
        }
        return;
    }
}

void ScMetaServer::resendBuf(int id)
{
    QByteArray send_buf;
    qDebug() << "ScTxServer::ACK"
             << id << curr_id;
    if( server->isOpen() &&
        server->state()==QTcpSocket::ConnectedState )
    {
        send_buf = tx_buf[id];

        qDebug() << "ScTxServer::resendBuf";
        sendData(send_buf);
        return;
    }
    qDebug() << "ScTxServer::resendBuf Failed";
}

void ScMetaServer::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

void ScMetaServer::addCounter(QByteArray *send_buf)
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
int ScMetaServer::sendData(QByteArray send_buf)
{
    int ret = server->writeDatagram(send_buf,
                                    ipv4, tx_port);
    server->flush();

    if( ret!=send_buf.length() )
    {
        qDebug() << "ScTxServer::sendData Error"
                 << send_buf.length() << ret;
        return 0;
    }

    return 1;
}

