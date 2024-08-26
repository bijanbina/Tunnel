#include <QCoreApplication>
#include "local.h"

int     ScSetting::state       = SC_STATE_CLIENT;
int     ScSetting::local_port  = 1088;
int     ScSetting::tx_port     = 5510;
int     ScSetting::rx_port     = 5511;
int     ScSetting::dbg_port    = 5512;
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
    if( argc>2 )
    {
        QString arg = argv[1];
        if( arg=="t" )
        {
            ScSetting::state = SC_STATE_TEST;
        }
        else
        {
            ScSetting::state = SC_STATE_SERVER;
            int exchange;
            exchange = ScSetting::tx_port;
            ScSetting::tx_port = ScSetting::rx_port;
            ScSetting::rx_port = exchange;
        }
    }
    ScLocal local;

    return app.exec();
}
