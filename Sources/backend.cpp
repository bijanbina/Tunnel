#include "backend.h"

QByteArray sc_mkPacket(QByteArray *send_buf, int *count)
{
    (*count)++;
    QString tx_id = QString::number(*count);
    tx_id = tx_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(tx_id.toStdString().c_str());
    *send_buf += SC_DATA_EOP;
    if( (*count)>SC_MAX_PACKID-1 )
    {
        (*count) = -1;
    }
    return *send_buf;
}
