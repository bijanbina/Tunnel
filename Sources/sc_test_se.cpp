#include "sc_test_se.h"

ScTestSe::ScTestSe(QObject *parent):
    QObject(parent)
{
    con      = NULL;
    server   = new QTcpServer;
    udp_con  = new QUdpSocket;
    tx_timer = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    // tx
    connect(udp_con, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT(txError()));

    // rx
    udp_con->bind(QHostAddress::Any, ScSetting::rx_port);
    connect(udp_con, SIGNAL(readyRead()),
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
             << udp_con->errorString()
             << udp_con->state();

    udp_con->close();
}

void ScTestSe::rxReadyRead()
{
    QByteArray rx_buf;

    while( udp_con->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(udp_con->pendingDatagramSize());

        udp_con->readDatagram(data.data(), data.size(),
                             &ipv4, &tx_port);

        rx_buf += data;
    }

    if( con )
    {
        con->write(rx_buf);
    }
}

void ScTestSe::txTest()
{
    QByteArray buf;
    for( int j=0 ; j<ScSetting::limit ; j++ )
    {
        buf += "<";
        buf += QString::number(j).rightJustified(6, '0');
        buf += ">";
    }

    udp_con->writeDatagram(buf, ipv4, tx_port);
    qDebug() << "txRefresh" << QHostAddress(ipv4.toIPv4Address())
             << tx_port;
}
