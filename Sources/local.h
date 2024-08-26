#ifndef SC_LOCAL_H
#define SC_LOCAL_H

#include <QString>
#include <QDebug>
#include "remote_client.h"
#include "sc_apache_pc.h"
#include "sc_apache_se.h"
#include "sc_apache_te.h"

class ScLocal : public QObject
{
    Q_OBJECT
public:
    explicit ScLocal(QObject *parent = nullptr);
};

#endif // SC_LOCAL_H
