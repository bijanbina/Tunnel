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
    explicit ScTxServer(int port, QObject *parent = 0);

    void reset();
    void resendBuf(int id);

    QHostAddress ipv4;
    int          curr_id;
    quint16      tx_port;
    QVector<QByteArray> tx_buf;

public slots:
    void write(QByteArray data);
    void txError();
    void readyRead();
    void timerTick();

signals:
    void init();

private:
    QByteArray rx_buf; // data comming, only start and ack


private:
    void writeBuf();
    int  sendData(QByteArray send_buf);

    QByteArray  buf;
    QUdpSocket *server;
    QTimer     *timer;
    int         is_dbg;
    int         tick_counter; // 0 to 9
    int         data_counter; // data transmitted in 1 sec
};

#endif // SC_TX_SERVER_H
