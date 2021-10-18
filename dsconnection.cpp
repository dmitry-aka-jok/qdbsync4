#include "dsconnection.h"

DataSync::Connection::Connection(QObject *parent) : QObject(parent), errorBehavior(DataSync::StopOnErrorDefault)
{
}

DataSync::Connection::~Connection()
{
}

QString DataSync::Connection::errorString()
{
    return this->lastErrorString;
}

DataSync::SyncEntry DataSync::Connection::getSyncEntry()
{
    return state;
}

void DataSync::Connection::setErrorBehavior(DataSync::ErrorBehavior errorBehavior)
{
    this->errorBehavior = errorBehavior;
}

void DataSync::Connection::setExecutionType(DataSync::ExecutionType executionType)
{
    this->executionType = executionType;
}

void DataSync::Connection::setParams(const QMap<QString, QVariant> &params)
{
    this->params = params;
}


void DataSync::Connection::setSettings(const QString &name, const QJsonObject &connectionSettings)
{
    this->name = name;
    this->connectionSettings = connectionSettings;
}

QStringList DataSync::Connection::supportedTypes()
{
    return QStringList();
}
