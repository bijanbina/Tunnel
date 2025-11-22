#include <QCoreApplication>
#include "local.h"

int          ScSetting::state       = SC_STATE_CLIENT;
int          ScSetting::limit       = 50;
int          ScSetting::local_port  = 1088;
int          ScSetting::tx_port     = 5510;
int          ScSetting::rx_port     = 5511;
int          ScSetting::dbg_tx_port = 5512;
int          ScSetting::dbg_rx_port = 5513;
QString      ScSetting::password    = "pass";
//QHostAddress ScSetting::remote_host = QHostAddress("5.255.113.20");
QHostAddress ScSetting::remote_host = QHostAddress("188.121.116.220");

// tunnel <local_port> <is_server=s>
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("WBT");
    app.setOrganizationDomain("WBT.com");
    app.setApplicationName("PolyBar");

    QString arg = argv[1];
    if( arg=="p" )
    {
        ScSetting::state = SC_STATE_PC_TEST;
    }
    else if( arg=="l" ) // limit
    {
        ScSetting::state = SC_STATE_TEST;
    }
    else if( arg=="s" ) // limit server
    {
        int tmp;
        tmp = ScSetting::tx_port;
        ScSetting::tx_port = ScSetting::rx_port;
        ScSetting::rx_port = tmp;
        ScSetting::state = SC_STATE_LIMIT;
    }
    else
    {
        ScSetting::local_port = arg.toUInt();
    }

    if( argc>2 )
    {
        QString arg2 = argv[2];
        if( arg2=="t" )
        {
            ScSetting::state = SC_STATE_SE_TEST;
        }
        else
        {
            ScSetting::state = SC_STATE_TEST;
        }
        int tmp;
        tmp = ScSetting::tx_port;
        ScSetting::tx_port = ScSetting::rx_port;
        ScSetting::rx_port = tmp;

        tmp = ScSetting::dbg_tx_port;
        ScSetting::dbg_tx_port = ScSetting::dbg_rx_port;
        ScSetting::dbg_rx_port = tmp;
    }
    ScLocal local;

    return app.exec();
}
