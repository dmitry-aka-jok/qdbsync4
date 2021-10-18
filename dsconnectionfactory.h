#ifndef CONNECTIONFACTORY_H
#define CONNECTIONFACTORY_H

#include "dsconnection.h"

namespace DataSync {

class ConnectionFactory
{
public:
    ConnectionFactory();

    static DataSync::Connection* getConnection(const QString &name, const QJsonObject &connectionSettings);

};

}

#endif // CONNECTIONFACTORY_H
