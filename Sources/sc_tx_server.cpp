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
    if( cons==NULL )
    {
        return;
    }
    if( cons->isOpen() )
    {
        cons->close();
    }
}

void ScTxServer::openPort(int port)
{
    tx_port = port;
    if( server->listen(QHostAddress::Any, tx_port) )
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

void ScTxServer::reset()
{
    curr_id = -1;
    conn_i  = 0;
    buf.clear();
}

void ScTxServer::txConnected()
{
    write(""); // to send buff data
}

void ScTxServer::txError(int id)
{
    if( cons->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ApacheSe::txError"
                 << cons->errorString()
                 << cons->state();
        cons->close();
    }
}

void ScTxServer::writeBuf()
{
    if( buf.isEmpty() || ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
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
}

void ScTxServer::resendBuf(int id)
{
    QByteArray send_buf;
    qDebug() << "ScTxServer::ACK"
             << id << curr_id;
    if( cons->isOpen() &&
        cons->state()==QTcpSocket::ConnectedState )
    {
        send_buf = tx_buf;
        qDebug() << "ScTxServer::resendBuf";
        return;
    }
    qDebug() << "ScTxServer::resendBuf Failed";
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

void ScTxServer::addCounter(QByteArray *send_buf)
{
    curr_id++;
    QString tx_id = QString::number(curr_id);
    tx_id = tx_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(tx_id.toStdString().c_str());
    tx_buf = *send_buf;
    if( curr_id>SC_MAX_PACKID-1 )
    {
        curr_id = -1;
    }
}

// return 1 when sending data is successful
int ScTxServer::sendData(QByteArray send_buf)
{
    int ret = cons->writeDatagram(send_buf, ipv4,
                                  tx_port);
    cons->flush();
    cons->close();
    if( ret!=send_buf.length() )
    {
        qDebug() << "ScTxServer::sendData Error"
                 << send_buf.length() << ret;
        return 0;
    }

    return 1;
}

