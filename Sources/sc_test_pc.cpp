#include "sc_test_pc.h"

ScTestPc::ScTestPc(QObject *parent):
    QObject(parent)
{
    con      = NULL;
    server   = new QTcpServer;
    udp_con  = new QUdpSocket();
    tx_timer = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    // tx
    connect(udp_con, SIGNAL(error(QAbstractSocket::SocketError)),
            this   , SLOT(txError()));

    // rx
    connect(udp_con, SIGNAL(readyRead()),
            this   , SLOT  (rxReadyRead()));

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txTest()));
    tx_timer->start(SC_TEST_TIMEOUT);
}

void ScTestPc::init()
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

void ScTestPc::clientConnected()
{
    con = server->nextPendingConnection();
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ScTestPc::setupConnection";

    ipv4 = QHostAddress(ip_32);
    msg += " accepted connection from " + ipv4.toString();
    qDebug() << msg.toStdString().c_str();
}

void ScTestPc::clientError()
{
    qDebug() << "clientError"
             << con->errorString()
             << con->state()
             << ipv4.toString();

    con->close();
}

void ScTestPc::txError()
{
    qDebug() << "txError"
             << udp_con->errorString()
             << udp_con->state();

    udp_con->close();
}

void ScTestPc::rxReadyRead()
{
    QByteArray rx_buf;

    while( udp_con->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(udp_con->pendingDatagramSize());

        QHostAddress sender;
        quint16 sender_port;

        udp_con->readDatagram(data.data(), data.size(),
                              &sender, &sender_port);

        rx_buf += data;
    }

    if( con )
    {
        qDebug() << "rx" << rx_buf.length();
        con->write(rx_buf);
    }
}

void ScTestPc::txTest()
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
    udp_con->writeDatagram(buf, ScSetting::remote_host,
                           ScSetting::tx_port);
    count++;
    qDebug() << "txRefresh" << ScSetting::remote_host
             << ScSetting::tx_port;
}
