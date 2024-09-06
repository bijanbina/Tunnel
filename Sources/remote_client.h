#ifndef SC_REMOTECLIENT_H
#define SC_REMOTECLIENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "backend.h"

class ScRemoteClient : public QObject
{
    Q_OBJECT
public:
    explicit ScRemoteClient(int port, QObject *parent = nullptr);
    void writeBuf();

    QByteArray  buf;

private slots:
    void disconnected();
    void displayError(QAbstractSocket::SocketError socketError);
    void conRefresh();

private:
    int     counter;
    int     tx_port;
    int     curr_id;
    QTimer *timer;
    QVector<QTcpSocket *> cons;
};

#endif // SC_REMOTECLIENT_H
