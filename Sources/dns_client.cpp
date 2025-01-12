#include "dns_client.h"

DnsClient::DnsClient(QObject *parent)
    : QObject(parent)
{
    socket = new QUdpSocket(this);
}

void DnsClient::sendDataAsDns(const QByteArray &data, const QHostAddress &serverAddress, quint16 serverPort)
{
    // 1. Create a fake DNS header
    DnsHeader header;
    // Fill with dummy values, converting to big-endian
    header.id      = qToBigEndian<quint16>(0x1234);
    header.flags   = qToBigEndian<quint16>(0x8180);
    header.qdCount = qToBigEndian<quint16>(1);
    header.anCount = qToBigEndian<quint16>(0);
    header.nsCount = qToBigEndian<quint16>(0);
    header.arCount = qToBigEndian<quint16>(0);

    // 2. Combine the fake DNS header with our payload
    QByteArray dnsPacket;
    dnsPacket.append(reinterpret_cast<const char*>(&header), sizeof(DnsHeader));
    dnsPacket.append(data);

    // 3. Send it over UDP
    qDebug() << "Sending DNS-disguised packet to"
             << serverAddress.toString() << ":" << serverPort
             << "Payload size =" << data.size()
             << "Total packet size =" << dnsPacket.size();

    socket->writeDatagram(dnsPacket, serverAddress, serverPort);
}
