#include "sc_test_se.h"

ScTestSe::ScTestSe(QObject *parent):
    QObject(parent)
{
    con      = NULL;
    server   = new QTcpServer;
    tx_con   = new QUdpSocket();
    rx_con   = new QUdpSocket();
    tx_timer = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    // tx
    connect(tx_con, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT(txError()));

    // rx
    rx_con->bind(QHostAddress::Any, ScSetting::rx_port);
    connect(rx_con, SIGNAL(readyRead()),
            this  , SLOT(rxReadyRead()));

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txTest()));
    tx_timer->start(SC_TEST_TIMEOUT);
}

void ScTestSe::init()
{
    if( server->listen(QHostAddress::Any, ScSetting::local_port) )
    {
        qDebug() << "Created server on port"
                 << ScSetting::local_port;
    }
    else
    {
        qDebug() << "Server failed, error message is:"
                 << server->errorString();
    }
}

void ScTestSe::clientConnected()
{
    con = server->nextPendingConnection();
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ScTestSe::setupConnection";

    ipv4 = QHostAddress(ip_32);
    msg += " accepted connection from " + ipv4.toString();
    qDebug() << msg.toStdString().c_str();
}

void ScTestSe::clientError()
{
    qDebug() << "clientError"
             << con->errorString()
             << con->state()
             << ipv4.toString();

    con->close();
}

void ScTestSe::txError()
{
    qDebug() << "txError"
             << tx_con->errorString()
             << tx_con->state();

    tx_con->close();
}

void ScTestSe::rxReadyRead()
{
    QByteArray rx_buf;

    while( rx_con->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(rx_con->pendingDatagramSize());

        quint16 sender_port;

        rx_con->readDatagram(data.data(), data.size(),
                             &ipv4, &sender_port);

        rx_buf += data;
    }

    if( con )
    {
        con->write(rx_buf);
    }
}

void ScTestSe::txTest()
{
    int count = 0;
    QByteArray buf;
    buf.clear();
    for( int j=0 ; j<ScSetting::limit ; j++ )
    {
        buf += "<";
        buf += QString::number(j).rightJustified(6, '0');
        buf += ">";
    }
    
    tx_con->writeDatagram(buf, ipv4, ScSetting::tx_port);
    count++;
    qDebug() << "txRefresh" << count;
}
