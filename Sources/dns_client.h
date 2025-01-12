#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include <QCoreApplication>
#include <QUdpSocket>
#include <QByteArray>
#include <QDebug>
#include <QtEndian>
#include "backend.h"

// A small, fake DNS header (12 bytes)
#pragma pack(push, 1)
struct DnsHeader
{
    quint16 id;       // Identification
    quint16 flags;    // Flags
    quint16 qdCount;  // # of question entries
    quint16 anCount;  // # of answer entries
    quint16 nsCount;  // # of authority entries
    quint16 arCount;  // # of resource entries
};
#pragma pack(pop)

class DnsClient : public QObject
{
    Q_OBJECT
public:
    DnsClient(QObject* parent = nullptr);

    // This function takes a QByteArray, disguises it as a DNS packet, and sends it.
    void sendDataAsDns(const QByteArray &data,
                       const QHostAddress &serverAddress,
                       quint16 serverPort);

private:
    QUdpSocket *socket;
};


#endif // DNS_CLIENT_H
