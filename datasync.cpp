#include "datasync.h"

#include <QDate>
#include <QRegularExpression>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>


#include "dsconnection.h"
#include "dsconnectionfactory.h"

#include <QDebug>

DataSync::DataSyncEngine::DataSyncEngine(QObject *parent) :
    QThread(parent),
    state(DataSync::Idle),
    errorBehavior(DataSync::StopOnErrorDefault),
    executionType(DataSync::ExecutionNormal)
{

}


bool DataSync::DataSyncEngine::setConfig(const QByteArray &config)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(config, &err);

    if(err.error != QJsonParseError::NoError){
        int currstart = config.lastIndexOf('\n', err.offset);
        if (currstart > 0)
            currstart = config.lastIndexOf('\n', currstart-1);
        if (currstart > 0)
            currstart = config.lastIndexOf('\n', currstart-1);
        if(currstart < 0)
            currstart = 0;

        int currend = config.indexOf('\n', err.offset);
        if (currend > 0)
            currend = config.indexOf('\n', currend+1);
        if (currend > 0)
            currend = config.indexOf('\n', currend+1);
        if(currend < 0)
            currend = config.length()-err.offset;

        lastErrorString = QStringLiteral("Error parsing json:\n%1\nString: %2\n%3<<<here>>>%4")
                .arg(err.errorString())
                .arg(config.left(err.offset).count('\n'))
                .arg(config.mid(currstart,err.offset-1-currstart).constData())
                .arg(config.mid(err.offset-1,currend-err.offset).constData());
        return false;
    }

    this->obj = doc.object();
    return true;
}

void DataSync::DataSyncEngine::setErrorBehavior(DataSync::ErrorBehavior errorBehavior)
{
    this->errorBehavior = errorBehavior;
}

void DataSync::DataSyncEngine::setExecutionType(DataSync::ExecutionType executionType)
{
    this->executionType = executionType;
}

void DataSync::DataSyncEngine::setParams(QMap<QString, QString> params)
{
    this->dparams = params;
}


void DataSync::DataSyncEngine::run()
{
    syncEntryCompleted.clear();
    currSync = 0;
    state = DataSync::Working;

    // first sync - datetime
    clearSyncEntry();
    syncEntry.name          = QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss");
    syncEntryCompleted.insert(currSync, syncEntry);
    emit eventOccurred("Synchronization started");


    QMap<QString, Connection*> conMap;
    QJsonArray connections = obj["connections"].toArray();

    // create connections
    for(QJsonValue val: connections)
    {
        QJsonObject conJSon = val.toObject();

        Connection *cn = ConnectionFactory::getConnection(conJSon["name"].toString(), conJSon);

        if (cn == nullptr)
        {
            lastErrorString =  QStringLiteral("Error in json - type of connection not supported\nSync: %1\nType:  %2")
                    .arg(conJSon["name"].toString())
                    .arg(conJSon["type"].toString());
            emit errorOccurred(lastErrorString);
            return;
        }

        connect(cn, &Connection::eventOccurred, [=](const QString &event){
            if(event=="Iteration finished")
              syncEntry=cn->getSyncEntry();
            if(event=="Comparsion finished")
              syncEntryCompleted[currSync]=cn->getSyncEntry();
            if(event=="Comparsion started"){
              currSync++;
              clearSyncEntry();
              syncEntryCompleted[currSync]=cn->getSyncEntry();
            }
          });

        connect(cn, &Connection::eventOccurred, this, &DataSyncEngine::eventOccurred);
        connect(cn, &Connection::errorOccurred, this, &DataSyncEngine::errorOccurred);


        conMap[conJSon["name"].toString()] = cn;
    }

    QMap<QString, QVariant> params;

    //
    QJsonArray paramTemplates = obj["parameters"].toArray();

    QRegularExpression red("^last (?<value>\\d+) days date$");
    QRegularExpression ret_h("^last (?<value>\\d+) hours time$");
    QRegularExpression ret_m("^last (?<value>\\d+) minutes time$");
    QRegularExpression redt_d("^last (?<value>\\d+) days datetime$");
    QRegularExpression redt_h("^last (?<value>\\d+) hours datetime$");
    QRegularExpression redt_m("^last (?<value>\\d+) minutes datetime$");
    QRegularExpressionMatch matcher;

    // create connections
    for(QJsonValue val: paramTemplates)
    {
        QJsonObject paramJSon = val.toObject();

        if (paramJSon["type"].toString()=="calculated")
        {
            QString def = paramJSon["eval"].toString();

            matcher = red.match(def);
            if (matcher.hasMatch())
                def = "'" + QDate::currentDate().addDays(-matcher.captured("value").toInt()).toString("dd.MM.yyyy") + "'";

            matcher = ret_h.match(def);
            if (matcher.hasMatch())
                def = "'" + QTime::currentTime().addSecs(-matcher.captured("value").toInt() * 60 * 60).toString("hh:mm") + "'";

            matcher = ret_m.match(def);
            if (matcher.hasMatch())
                def = "'" + QTime::currentTime().addSecs(-matcher.captured("value").toInt() * 60).toString("hh:mm") + "'";

            matcher = redt_d.match(def);
            if (matcher.hasMatch())
                def = "'" + QDateTime::currentDateTime().addDays(-matcher.captured("value").toInt()).toString("dd.MM.yyyy hh:mm") + "'";

            matcher = redt_h.match(def);
            if (matcher.hasMatch())
                def = "'" + QDateTime::currentDateTime().addSecs(-matcher.captured("value").toInt() * 60 * 60).toString("dd.MM.yyyy hh:mm") + "'";

            matcher = redt_m.match(def);
            if (matcher.hasMatch())
                def = "'" + QDateTime::currentDateTime().addSecs(-matcher.captured("value").toInt() * 60).toString("dd.MM.yyyy hh:mm") + "'";

            if (def == "current date")
                def = "'" + QDate::currentDate().toString("dd.MM.yyyy") + "'";

            if (def == "current time")
                def = "'" + QTime::currentTime().toString("hh:mm") + "'";

            if (def == "current datetime")
                def = "'" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm") + "'";

            if (def == "last month date")
                def = "'" + QDate::currentDate().addMonths(-1).toString("dd.MM.yyyy") + "'";

            if (def == "last month datetime")
                def = "'" + QDateTime::currentDateTime().addMonths(-1).toString("dd.MM.yyyy 00:01") + "'";

            if (def == "last quarter date")
                def = "'" + QDate::currentDate().addMonths(-3).toString("dd.MM.yyyy") + "'";

            if (def == "last quarter datetime")
                def = "'" + QDateTime::currentDateTime().addMonths(-3).toString("dd.MM.yyyy 00:01") + "'";

            QString name = paramJSon["name"].toString();
            params[name] = def;
        }


        if (paramJSon["type"].toString()=="query")
        {
            QString query = paramJSon["query"].toString();

            if(!conMap.contains(paramJSon["connection"].toString()))
            {
                lastErrorString =  QStringLiteral("Error in json - no such connection for params\nSync: %1\n role %2")
                        .arg(obj["name"].toString())
                        .arg(paramJSon["connection"].toString());
                emit eventOccurred(lastErrorString);
                return;
            }
            Connection *cn = conMap[paramJSon["connection"].toString()];

            bool ok;
            QVariant v = cn->getSingleValue(query, &ok);

            QString name = paramJSon["name"].toString();
            params[name] = v;
        }

        if (paramJSon["type"].toString()=="value")
        {
            QString v = paramJSon["value"].toString();

            QString name = paramJSon["name"].toString();
            params[name] = v;
        }

    }



    QJsonArray syncs = obj["synchronizations"].toArray();
    for (auto val : syncs)
    {
        QJsonObject syncJSon = val.toObject();

        if(syncJSon["state"].toString()=="off")
        {
            emit eventOccurred("Comparsion skipped");
            continue;
        }

        if(!conMap.contains(syncJSon["source"].toObject()["connection"].toString()))
        {
            lastErrorString =  QStringLiteral("Error in json - no source connection\nSync: %1\n role %2")
                    .arg(obj["name"].toString())
                    .arg(syncJSon["source"].toObject()["connection"].toString());
            emit eventOccurred(lastErrorString);
            return;
        }
        Connection *src = conMap[syncJSon["source"].toObject()["connection"].toString()];

        if(!conMap.contains(syncJSon["destination"].toObject()["connection"].toString()))
        {
            lastErrorString =  QStringLiteral("Error in json - no destination connection\nSync: %1\n role %2")
                    .arg(obj["name"].toString())
                    .arg(syncJSon["destination"].toObject()["connection"].toString());
            emit eventOccurred(lastErrorString);
            return;
        }
        Connection *dest = conMap[syncJSon["destination"].toObject()["connection"].toString()];

        src->setParams(params);
        if (!src->open(syncJSon)){
            lastErrorString =  QStringLiteral("Error in source connection\nSync: %1\n role %2")
                    .arg(obj["name"].toString())
                    .arg(syncJSon["source"].toObject()["connection"].toString());
            emit eventOccurred(lastErrorString);
            return;
        }

        dest->setParams(params);
        if(!dest->compareTo(syncJSon, src)){
            lastErrorString =  QStringLiteral("Error in dest connection\nSync: %1\n role %2")
                    .arg(obj["name"].toString())
                    .arg(syncJSon["source"].toObject()["connection"].toString());
            emit eventOccurred(lastErrorString);
            return;
        }

        }
    }


    clearSyncEntry();
    syncEntry.name          = QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss");
    currSync++;
    syncEntryCompleted.insert(currSync, syncEntry);
    emit eventOccurred("Synchronization finished");

}


QString DataSync::DataSyncEngine::lastError()
{
    return lastErrorString;
}

QString DataSync::DataSyncEngine::lastExecuted()
{
    return lastQuery;
}

DataSync::State DataSync::DataSyncEngine::getState()
{
    return state;
}

DataSync::SyncEntry DataSync::DataSyncEngine::getSyncEntry()
{
    return syncEntry;
}

void DataSync::DataSyncEngine::clearSyncEntry()
{
    syncEntry.name.clear();
    syncEntry.qty           = 0;
    syncEntry.qtyIns        = 0;
    syncEntry.qtyInsFail    = 0;
    syncEntry.qtyDel        = 0;
    syncEntry.qtyDelFail    = 0;
    syncEntry.qtyUpd        = 0;
    syncEntry.qtyUpdFail    = 0;
    syncEntry.qtyResolvFail = 0;
}

QMap<int, DataSync::SyncEntry> DataSync::DataSyncEngine::getSyncEntryCompleted()
{
    return syncEntryCompleted;
}
