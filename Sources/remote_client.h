#ifndef SC_REMOTECLIENT_H
#define SC_REMOTECLIENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include "backend.h"

class ScRemoteClient : public QObject
{
    Q_OBJECT
public:
    explicit ScRemoteClient(int port, QObject *parent = nullptr);
    void open();
    void writeBuf(QHostAddress host);

    QTcpSocket *remote;
    QByteArray  buf;
    int         direct;

private slots:
    void disconnected();
    void displayError(QAbstractSocket::SocketError socketError);

private:
    int counter;
    int tx_port;
};

#endif // SC_REMOTECLIENT_H
