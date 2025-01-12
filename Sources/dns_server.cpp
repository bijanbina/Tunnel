#include "dns_server.h"

DnsServer::DnsServer(quint16 port, QObject *parent)
    : QObject(parent)
{
    socket = new QUdpSocket(this);

    if (!socket->bind(QHostAddress::Any, port)) {
        qDebug() << "Failed to bind on port" << port;
        return;
    }

    connect(socket, &QUdpSocket::readyRead, this, &DnsServer::onReadyRead);
    qDebug() << "DNS Server listening on UDP port" << port;
}

void DnsServer::onReadyRead()
{
    while (socket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 sender_port;

        socket->readDatagram(data.data(), data.size(),
                             &sender, &sender_port);


        // For demo, just print out the raw bytes (in hex)
        qDebug() << "Received packet from"
                 << sender.toString()
                 << ":" << sender_port;
        qDebug() << "Size:" << data.size();
        qDebug() << "Hex dump:" << data.toHex(' ');
        qDebug() << "-----------------------------------";
    }
}
