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

// return -1 if no packet should be send
int sc_resendID(int ack, int curr_index)
{
    int ret = -1;
    int diff = curr_index - ack;

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

    if( diff>1 )
    {
        ret = ack + 1;
        if( ret>SC_MAX_PACKID )
        {
            ret = 0;
        }
    }

    return ret;
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

QByteArrayList sc_splitPacket(QByteArray  data,
                              const char *separator)
{
    QString sep =  separator;
    QByteArrayList ret;
    int start = 0;
    int index = data.indexOf(separator, start);

    while( index!=-1 )
    {
        ret.append(data.mid(start, index-start));
        start = index + sep.length();
        index = data.indexOf(separator, start);
    }

    // Add the last part
    ret.append(data.mid(start));
    return ret;
}
