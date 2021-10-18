#include "dsconnectionfactory.h"

#include "connections/sqlconnection.h"

DataSync::ConnectionFactory::ConnectionFactory()
{

}

DataSync::Connection* DataSync::ConnectionFactory::getConnection(const QString &name, const QJsonObject &connectionSettings)
{
        QString type = connectionSettings["type"].toString();

        if(SQLConnection::supportedTypes().contains(type))
        {
            SQLConnection *conn = new SQLConnection();
            conn->setSettings(name, connectionSettings);
            return conn;
        }
        /*
        if(type==_S("remotedb"))
        {
            RemoteSQLConnection *conn = new RemoteSQLConnection();
            conn->setConnectionSettings(connectionSettings);
            return conn;
        }
        */

        return nullptr;

}
