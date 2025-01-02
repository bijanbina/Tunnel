#include "backend.h"

QByteArray sc_mkPacket(QByteArray *send_buf, int *count)
{
    (*count)++;
    if( (*count)>SC_MAX_PACKID )
    {
        (*count) = 0;
    }

    QString tx_id = QString::number(*count);
    tx_id = tx_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(tx_id.toStdString().c_str());
    *send_buf += SC_DATA_EOP;
    return *send_buf;
}

int sc_needResend(int ack, int curr_index)
{
    int diff = curr_index-ack;

    if( qAbs(diff)>SC_MAX_PACKID/2 )
    {
        if( ack>SC_MAX_PACKID/2 && curr_index<SC_MAX_PACKID/2 )
        {
            diff = SC_MAX_PACKID-ack+curr_index;
        }
        else if( ack<SC_MAX_PACKID/2 &&
                 curr_index>SC_MAX_PACKID/2 )
        {
            diff = SC_MAX_PACKID-curr_index+ack-1;
        }
    }

    if( diff>0 )
    {
        return 1;
    }
    return 0;
}

int sc_hasPacket(QVector<QByteArray> *buf, int id)
{
    int next = id+1;
    if( next>SC_MAX_PACKID )
    {
        next = 0;
    }

    if( (*buf)[next].length() )
    {
        return 1;
    }
    return 0;
}
