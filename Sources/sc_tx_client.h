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

    void reset();
    void write(QByteArray data);
    void resendBuf(int id);

    int  curr_id;
    QVector<QTcpSocket *> cons;

private slots:
    void disconnected();
    void displayError(QAbstractSocket::SocketError socketError);
    void conRefresh();
    void writeBuf();

private:
    void addCounter(QByteArray *send_buf);
    int  sendData  (QByteArray  send_buf);

    int     conn_i;
    int     tx_port;
    QTimer *refresh_timer;
    QTimer *tx_timer;
    QByteArray  buf;
    QVector<QByteArray>   tx_buf;
};

#endif // SC_TX_CLIENT_H
