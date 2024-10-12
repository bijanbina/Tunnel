#ifndef SC_DBG_CLIENT_H
#define SC_DBG_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>
#include "backend.h"

class ScDbgClient : public QObject
{
    Q_OBJECT
public:
    explicit ScDbgClient(int port, QObject *parent = nullptr);
    void write(QByteArray data);
    void reset();

public slots:
    void conRefresh();
    void writeBuf();

private:
    void addCounter(QByteArray *send_buf);
    int  sendData(QByteArray send_buf);

    int tx_port;
    int curr_id;
    int conn_i;
    QTimer *refresh_timer;
    QTimer *tx_timer;
    QByteArray buf;
    QVector<QTcpSocket *> cons;
    QVector<int>          cons_count;
    QVector<QByteArray> tx_buf;
};

#endif // SC_DBG_CLIENT_H
