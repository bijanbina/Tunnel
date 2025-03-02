#ifndef SC_TX_CLIENT_H
#define SC_TX_CLIENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#include "backend.h"

class ScTxClient : public QObject
{
    Q_OBJECT
public:
    explicit ScTxClient(int port, QObject *parent = nullptr);

    void reset();
    void write(QByteArray data);
    void resendBuf(int id);

    int         curr_id;
    QUdpSocket *cons;

private slots:
    void writeBuf();

private:
    int  sendData(QByteArray send_buf);

    int         tx_port;
    QTimer     *tx_timer;
    QByteArray  buf;
    QVector<QByteArray> tx_buf;
};

#endif // SC_TX_CLIENT_H
