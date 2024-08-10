#include <QCoreApplication>
#include "local.h"

int     ScSetting::is_server   = 0;
int     ScSetting::local_port  = 1088;
int     ScSetting::tx_port     = 5510;
int     ScSetting::rx_port     = 5511;
QString ScSetting::password    = "pass";
QString ScSetting::remote_host = "5.255.113.20";

// tunnel <local_port> <is_server=s>
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("WBT");
    app.setOrganizationDomain("WBT.com");
    app.setApplicationName("PolyBar");

    QString port = argv[1];
    ScSetting::local_port = port.toUInt();
    if( argc >2 )
    {
        ScSetting::is_server = 1;
        int exchange;
        exchange = ScSetting::tx_port;
        ScSetting::tx_port = ScSetting::rx_port;
        ScSetting::rx_port = exchange;
    }
    ScLocal local;

    return app.exec();
}
