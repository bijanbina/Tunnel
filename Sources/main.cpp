#include <QCoreApplication>
#include <QUdpSocket>
#include <QByteArray>
#include <QDebug>
#include <QtEndian>
#include "backend.h"
#include "dns_client.h"

int          ScSetting::state       = SC_STATE_CLIENT;
int          ScSetting::local_port  = 1088;
int          ScSetting::tx_port     = 5510;
int          ScSetting::rx_port     = 5511;
int          ScSetting::dbg_tx_port = 5512;
int          ScSetting::dbg_rx_port = 5513;
QString      ScSetting::password    = "pass";
QHostAddress ScSetting::remote_host = QHostAddress("5.255.113.20");

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString serverIp   = "5.255.113.20";
    quint16 serverPort = 5000;
    QByteArray message = "sag sag sag";

    DnsClient client;
    client.sendDataAsDns(message, QHostAddress(serverIp),
                         serverPort);

    return 0;
}
