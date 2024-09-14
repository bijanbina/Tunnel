#ifndef SC_TX_CLIENT_H
#define SC_TX_CLIENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "backend.h"

class ScTxClient : public QObject
{
    Q_OBJECT
public:
    explicit ScTxClient(int port, QObject *parent = nullptr);
    void write(QByteArray data);

private slots:
    void disconnected();
    void displayError(QAbstractSocket::SocketError socketError);
    void conRefresh();
    void writeBuf();

private:
    void addCounter(QByteArray *send_buf);
    int  sendData(QByteArray send_buf);

    int     counter;
    int     tx_port;
    int     curr_id;
    QTimer *refresh_timer;
    QTimer *tx_timer;
    QByteArray  buf;
    QVector<QTcpSocket *> cons;
};

#endif // SC_TX_CLIENT_H
