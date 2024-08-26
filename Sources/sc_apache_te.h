#ifndef SC_APACHE_TE_H
#define SC_APACHE_TE_H

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

#define SC_PC_CONLEN    10

class ScApacheTE : public QObject
{
    Q_OBJECT

public:
    explicit ScApacheTE(QString name="", QObject *parent = 0);
    ~ScApacheTE();

    void init();

public slots:
    void rxReadyRead(int id);
    void rxError(int id);
    void rxDisconnected(int id);
    void rxRefresh();

private:
    ScRemoteClient *client;
    ScRemoteClient *dbg;
    QVector<QByteArray> read_bufs;
    QString        con_name;
    QByteArray     tx_buf;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QVector<QTcpSocket *> rx_clients;
    QByteArray      rx_buf;
    QTimer         *rx_timer;
};

#endif // SC_APACHE_TE_H
