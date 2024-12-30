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
    int diff1    = abs(ack-curr_index);
    int diff2    = abs(curr_index-ack);

    int min_diff = qMin(diff1, diff2);
    if( min_diff )
    {
        return 1;
    }
    return 0;
}
