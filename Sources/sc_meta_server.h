#ifndef SC_META_SERVER_H
#define SC_META_SERVER_H

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

class ScMetaServer: public QObject
{
    Q_OBJECT

public:
    explicit ScMetaServer(QObject *parent = 0);
    ~ScMetaServer();

    void openPort(int port);
    void reset();
    void resendBuf(int id);

    QHostAddress         ipv4;
    QVector<QByteArray>  tx_buf;
    int                  curr_id;

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
    int         conn_i;
    int         tx_port;
};

#endif // SC_META_SERVER_H
