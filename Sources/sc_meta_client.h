#ifndef SC_META_CLIENT_H
#define SC_META_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#include <QVector>
#include "backend.h"

class ScMetaClient : public QObject
{
    Q_OBJECT
public:
    explicit ScMetaClient(int port, QObject *parent = nullptr);
    void write(QByteArray data);
    void reset();

private:
    int  sendData(QByteArray send_buf);

    int         tx_port;
    int         curr_id;
    QByteArray  buf;
    QUdpSocket *cons;
};

#endif // SC_META_CLIENT_H
