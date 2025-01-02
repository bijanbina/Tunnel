#include "sc_meta_client.h"

ScMetaClient::ScMetaClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port  = port;
    curr_id  = -1;
    cons = new QUdpSocket;
}

void ScMetaClient::reset()
{
    curr_id = -1;
    buf.clear();
}

void ScMetaClient::write(QByteArray data)
{
    sc_mkPacket(&data, &curr_id);

    if( sendData(data)==0 )
    {
        qDebug() << "-----SC_TX_CLIENT ERROR: DATA LOST-----";
    }
}

// return 1 when sending data is successful
int ScMetaClient::sendData(QByteArray send_buf)
{
    if( send_buf.isEmpty() )
    {
        return 0;
    }
    //    qDebug() << "ScDbgClient::sendData send_buf:" << send_buf;
    int s = cons->writeDatagram(send_buf,
                                QHostAddress(ScSetting::remote_host), tx_port);

    if( s!=send_buf.length() )
    {
        qDebug() << "writeBuf: Error"
                 << send_buf.length() << s;
        return 0;
    }

    return 1;
}
