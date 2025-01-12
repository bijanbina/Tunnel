#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include <QCoreApplication>
#include <QUdpSocket>
#include <QDebug>

class DnsServer : public QObject
{
    Q_OBJECT
public:
    explicit DnsServer(quint16 port, QObject* parent = nullptr);

private slots:
    void onReadyRead();

private:
    QUdpSocket *socket;
};

#endif // DNS_SERVER_H
