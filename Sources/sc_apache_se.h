#ifndef SC_APACHE_SE_H
#define SC_APACHE_SE_H

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
#include "sc_tx_server.h"

#define FA_START_PACKET "<START>\r\n"
#define FA_END_PACKET   "\r\n<END>\r\n"

class ScApacheSe : public QObject
{
    Q_OBJECT

public:
    explicit ScApacheSe(QObject *parent = 0);
    ~ScApacheSe();

    void connectApp();

    // rx
    QVector<QTcpSocket *> rx_cons;
    QVector<QHostAddress> rx_ipv4;

    // dbg
    QVector<QTcpSocket *> dbg_cons;
    QVector<QHostAddress> dbg_ipv4;

public slots:
    // client
    void clientConnected();
    void clientError();
    void clientDisconnected();

    // rx
    void rxReadyRead(int id);
    void rxDisconnected(int id);
    void rxConnected();
    void rxError(int id);

    // tx
    void txReadyRead();

    // dbg
    void dbgReadyRead(int id);
    void dbgConnected();

private:
    void reset();

    // rx
    int  rxPutInFree();
    void rxSetupConnection(int con_id);
    QByteArray getPack();

    // dbg
    int  dbgPutInFree();
    void dbgSetupConnection(int con_id);

    QTimer     *rx_timer;
    QTcpSocket  client;

    // rx
    QSignalMapper   *rx_mapper_data;
    QSignalMapper   *rx_mapper_disconnect;
    QSignalMapper   *rx_mapper_error;
    QTcpServer      *rx_server;
    int              rx_curr_id;
    QVector<QByteArray> rx_buf;
    QVector<QByteArray> read_bufs;

    // tx
    QSignalMapper *tx_mapper_data;
    ScTxServer    *tx_server;

    // dbg
    QSignalMapper  *dbg_mapper_data;
    QTcpServer     *dbg_server;
};

#endif // SC_APACHE_SE_H
