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

public slots:
    void readyRead();
    void acceptConnection();
    void displayError();
    void tcpDisconnected();

    // rx
    void rxReadyRead(int id);
    void rxAcceptConnection();

private:
    QByteArray processBuffer(int id);

    // rx
    int  rxPutInFree();
    void rxSetupConnection(int con_id);

    ScRemoteClient *tx_client;
    QTcpSocket     *client;
    QVector<QByteArray> read_bufs;
    QString con_name;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QTcpServer     *rx_server;
};

#endif // SC_APACHE_SE_H
