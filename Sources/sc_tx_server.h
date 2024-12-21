#ifndef SC_TX_SERVER_H
#define SC_TX_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QObject>
#include <QVector>
#include <stdio.h>
#include <stdlib.h>
#include <QTimer>
#include <QSignalMapper>
#include <QUdpSocket>
#include "backend.h"
#include "sc_tx_client.h"

class ScTxServer : public QObject
{
    Q_OBJECT

public:
    explicit ScTxServer(QObject *parent = 0);
    ~ScTxServer();

    void openPort(int port);
    void reset();
    void resendBuf();

    QHostAddress ipv4;
    QByteArray   tx_buf;
    int          curr_id;
    int          tx_port;

public slots:
    void write(QByteArray data);
    void txConnected();
    void txError();
    void writeBuf();

private:
    void addCounter(QByteArray *send_buf);
    int  sendData(QByteArray send_buf);

    QByteArray  buf;
    QUdpSocket *server;
    QTimer     *timer;
};

#endif // SC_TX_SERVER_H
