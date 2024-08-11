#ifndef SC_APACHE_PC_H
#define SC_APACHE_PC_H

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

class ScApachePC : public QObject
{
    Q_OBJECT

public:
    explicit ScApachePC(QString name="", QObject *parent = 0);
    ~ScApachePC();

    void bind();

    QVector<QTcpSocket *> cons;
    QVector<QHostAddress> ipv4;

    // rx
    QVector<QTcpSocket *> rx_cons;
    QVector<QHostAddress> rx_ipv4;

signals:
    void connected(int id);
    void dataReady(int id, QString data);

public slots:
    void readyRead(int id);
    void acceptConnection();
    void displayError(int id);
    void tcpDisconnected(int id);

    // rx
    void rxReadyRead(int id);
    void rxAcceptConnection();

private:
    QByteArray processBuffer(int id);
    int  putInFree();
    void setupConnection(int con_id);

    // rx
    int  rxPutInFree();
    void rxSetupConnection(int con_id);

    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    ScRemoteClient *client;
    QVector<QByteArray> read_bufs;
    QString con_name;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QTcpServer     *rx_server;
};

#endif // SC_APACHE_PC_H