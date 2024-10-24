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
    void resendBuf(int id);

    QVector<QTcpSocket *> cons;
    QVector<QHostAddress> ipv4;
    QVector<QByteArray>   tx_buf;
    int                   curr_id;

public slots:
    void write(QByteArray data);
    void txConnected();
    void txError(int id);
    void writeBuf();

private:
    void addCounter(QByteArray *send_buf);
    int  sendData(QByteArray send_buf);

    int  txPutInFree();
    void txSetupConnection(int con_id);

    QByteArray  buf;

    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    QTimer         *timer;
    int             conn_i;
    int             tx_port;
};

#endif // SC_TX_SERVER_H
