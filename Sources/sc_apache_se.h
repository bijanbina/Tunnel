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
#include "remote_client.h"

#define FA_START_PACKET "<START>\r\n"
#define FA_END_PACKET   "\r\n<END>\r\n"

class ScApacheSe : public QObject
{
    Q_OBJECT

public:
    explicit ScApacheSe(QString name="", QObject *parent = 0);
    ~ScApacheSe();

    void connectApp();

    // rx
    QVector<QTcpSocket *> rx_cons;
    QVector<QHostAddress> rx_ipv4;

    // tx
    QVector<QTcpSocket *> tx_cons;
    QVector<QHostAddress> tx_ipv4;

public slots:
    void txReadyRead();
    void txConnected();
    void txError(int id);
    void clientConnected();
    void clientError();
    void clientDisconnected();

    // rx
    void rxReadyRead(int id);
    void rxConnected();
    void rxError(int id);
    void rxTimeout();

private:
    // rx
    int  rxPutInFree();
    void rxSetupConnection(int con_id);

    // tx
    int  txPutInFree();
    void txSetupConnection(int con_id);

    int         dbg1;
    QTimer     *rx_timer;
    QTcpSocket  client;
    QByteArray  tx_buf;
    QByteArray  rx_buf;
    QString     con_name;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QTcpServer     *rx_server;

    // rx
    QSignalMapper  *tx_mapper_data;
    QSignalMapper  *tx_mapper_disconnect;
    QSignalMapper  *tx_mapper_error;
    QTcpServer     *tx_server;
};

#endif // SC_APACHE_SE_H
