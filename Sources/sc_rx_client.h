#ifndef SC_RX_CLIENT_H
#define SC_RX_CLIENT_H

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
#include "sc_dbg_client.h"
#include "sc_tx_client.h"

class ScRxClient : public QObject
{
    Q_OBJECT

public:
    explicit ScRxClient(int rx_port, QObject *parent = 0);
    ~ScRxClient();

    void init();

    int  curr_id;

signals:
    void dataReady(QByteArray data);

public slots:
    void readyRead(int id);
    void error(int id);
    void disconnected(int id);
    void refresh();
    void timeout(int id);

private:
    QByteArray getPack();
    void processBuffer(int id);

    QVector<QByteArray> read_bufs;

    // rx
    QVector<QTcpSocket *> clients;
    QVector<QByteArray>   rx_buf;
    QTimer               *refresh_timer;
    QVector<QTimer *>     conn_timers;
    int                   port;
    int                   is_debug;

    // dbg rx
    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_connect;
    QSignalMapper  *mapper_error;
    QSignalMapper  *mapper_timer;
};

#endif // SC_APACHE_PC_H
