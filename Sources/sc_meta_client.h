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

public slots:
    void writeBuf();

private:
    int  sendData(QByteArray send_buf);

    int         tx_port;
    int         curr_id;
    QTimer     *tx_timer;
    QByteArray  buf;
    QUdpSocket *cons;
    QVector<QByteArray> tx_buf;
};

#endif // SC_META_CLIENT_H
