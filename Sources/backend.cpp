#include "backend.h"

QByteArray sc_mkPacket(QByteArray *send_buf, int *count)
{
    (*count)++;
    if( (*count)>SC_MAX_PACKID )
    {
        (*count) = 0;
    }

    QString len_s = QString::number(send_buf->length());
    len_s = len_s.rightJustified(SC_LEN_PACKLEN, '0');
    send_buf->prepend(len_s.toStdString().c_str());

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

// skip if already received the packet to prevent refilling
// cleared buf, skipping this step will cause issue
// on next cycle
int sc_skipPacket(int current, int id)
{
    int diff = current - id;
    if( qAbs(diff)<SC_MAX_PACKID/2 )
    {
        if( id<=current )
        {
            return 1;
        }
    }
    else if( id>SC_MAX_PACKID/2 )
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

ScPacket sc_processPacket(QByteArray *buf, int curr_id)
{
    ScPacket ret;
    QString id_s  = buf->mid(0, SC_LEN_PACKID);
    QString len_s = buf->mid(SC_LEN_PACKID, SC_LEN_PACKLEN);
    bool int_ok  = 0;
    ret.id       = id_s.toInt(&int_ok);
    ret.skip     = sc_skipPacket(curr_id, ret.id);
    int start    = SC_LEN_PACKID + SC_LEN_PACKLEN;
    int end      = buf->indexOf(SC_DATA_EOP);

    if( int_ok==0 )
    {
        qDebug() << "sc_processPacket shit has happened id"
                 << id_s << "should be int";
        exit(1);
    }

    ret.len = len_s.toInt(&int_ok);
    if( int_ok==0 )
    {
        qDebug() << "sc_processPacket shit has happened len"
                 << len_s << "should be int";
        exit(1);
    }

    ret.data = buf->mid(start, end-SC_LEN_PACKID);
    if( ret.len!=ret.data.length() )
    {
        ret.skip = 1;
    }

    // Extract the packet including the EOP marker
    buf->remove(0, end + strlen(SC_DATA_EOP));
    return ret;
}
