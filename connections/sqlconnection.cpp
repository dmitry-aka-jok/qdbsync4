#include "sqlconnection.h"

#include <QtSql>

#include "sqlparser.h"

bool SQLConnection::init()
{
    if (isInitialized)
        return true;

    resolvers.clear();


    QSqlDatabase db;
    if (connectionSettings["type"].toString() == "current")
        db = QSqlDatabase::database();
    else
        db = QSqlDatabase::database(connectionSettings["name"].toString());

    if (!db.isValid())
    {
        db = QSqlDatabase::addDatabase(connectionSettings["driver"].toString(), connectionSettings["name"].toString());

        db.setConnectOptions(connectionSettings["options"].toString());
        db.setHostName(connectionSettings["hostname"].toString());
        db.setDatabaseName(connectionSettings["database"].toString());
        db.setUserName(connectionSettings["user"].toString());
        db.setPassword(connectionSettings["password"].toString());

        if (!db.open())
        {
            lastErrorString = QStringLiteral("Error connecting to the database\nSync: %1\nDatabase: %2\nQuery: %3\nError: %4")
                    .arg(name, connectionSettings["type"].toString(), connectionSettings["database"].toString(), db.lastError().text());

            emit errorOccurred(lastErrorString);
            return false;
        }
    }


    querySource     = new QSqlQuery(db); // yes, this query will be created and for destination-ony bases, but thats easier create it here
    queryService    = new QSqlQuery(db);


    QStringList queryList = SqlParser::parseScript(connectionSettings["on_init"].toString());
    foreach(QString query, queryList)
    {
        if(!query.contains(QRegExp("\\S")))
            continue;

        emit eventOccurred("Query (on init):"+query);
        if (!queryService->exec(query))
        {
            lastErrorString = QStringLiteral("Error executing on_init query\nSync: %1.on_init\nDatabase: %2 %3\nQuery: %4\nError: %5")
                    .arg(name
                    ,connectionSettings["type"].toString()
                    ,connectionSettings["database"].toString()
                    ,queryService->lastQuery()
                    ,queryService->lastError().text());

            emit errorOccurred(lastErrorString);

            querySource=nullptr;
            queryService=nullptr;
            return false;
        }
    }
    isInitialized = true;

    return true;
}

QVariant SQLConnection::calcResolver(const QString &resolver, Connection *src, bool &isOk)
{
    if (!init())
    {
        isOk = false;
        lastErrorString = QStringLiteral("Error initing resolver\nSync: %1\nDatabase: %2 %3\nResolver: %4")
                .arg(name, connectionSettings["type"].toString(), connectionSettings["database"].toString(),resolver);

        emit errorOccurred(lastErrorString);
        return QVariant();
    }

    QString r = resolver;
    QRegularExpression re("\\%(\\w+|\\d+)"); // %field  or %fieldnumber

    QRegularExpressionMatchIterator remi = re.globalMatch(resolver);
    while (remi.hasNext()) {
        QRegularExpressionMatch match = remi.next();
        QString s = match.captured();

        QVariant res = src->value(s.mid(1));

        if (res.isValid())
            r = r.replace(s, res.toString());
    }

    if (!resolvers.contains(r)){

        emit eventOccurred("Query (resolver):"+r);

        QSqlDatabase db;
        if (connectionSettings["type"].toString() == "current")
            db = QSqlDatabase::database();
        else
            db = QSqlDatabase::database(connectionSettings["name"].toString());

        QSqlQuery *queryResolver  = new QSqlQuery(db); // yes, this query will be created and for destination-ony bases, but thats easier create it here


        //   qDebug()<<r;
        isOk = queryResolver->exec(r);
        if(isOk)
            isOk = queryResolver->next();
        if(isOk)
            isOk = queryResolver->isActive();

        if (!isOk)
        {
            lastErrorString = QStringLiteral("Error executing resolver\nSync: %1.%2, resolver\nDatabase: %3 %4\nQuery: %5\nError: %6")
                    .arg(name, replName, connectionSettings["type"].toString(), connectionSettings["database"].toString(), queryResolver->lastQuery(), queryResolver->lastError().text());

            emit errorOccurred(lastErrorString);
            return 0;
        }
        QVariant val = queryResolver->value(0);
        delete queryResolver;

        resolvers[r] = val;
    }

    return resolvers[r];
}

bool SQLConnection::compare(const QVariant &valSrc, const QVariant &valDst, bool &isNewSrc, bool &isOldDst)
{
    isNewSrc = false;
    isOldDst = false;

    if (valDst.type() != valSrc.type())
        if (!valDst.canConvert(static_cast<int>(valSrc.type())))
        {
            lastErrorString = QStringLiteral("Error comparing fields\nSync: %1.%2, role %3\nDatabase: %4 %5\n%6\n%7")
                    .arg(name, replName, connectionSettings["name"].toString(), connectionSettings["type"].toString(), connectionSettings["database"].toString(), valSrc.typeName(), valDst.typeName());

            emit errorOccurred(lastErrorString);

            return false;
        }



    if (valSrc.type() == QVariant::Int)
    {
        if (valDst.toInt() > valSrc.toInt())
            isNewSrc = true;
        if (valDst.toInt() < valSrc.toInt())
            isOldDst = true;
    }
    else
        if (valSrc.type() == QVariant::UInt)
        {
            if (valDst.toUInt() > valSrc.toUInt())
                isNewSrc = true;
            if (valDst.toUInt() < valSrc.toUInt())
                isOldDst = true;
        }
        else
            if (valSrc.type() == QVariant::LongLong)
            {
                if (valDst.toLongLong() > valSrc.toLongLong())
                    isNewSrc = true;
                if (valDst.toLongLong() < valSrc.toLongLong())
                    isOldDst = true;
            }
            else
                if (valSrc.type() == QVariant::ULongLong)
                {
                    if (valDst.toULongLong() > valSrc.toULongLong())
                        isNewSrc = true;
                    if (valDst.toULongLong() < valSrc.toULongLong())
                        isOldDst = true;
                }
                else
                    if (valSrc.type() == QVariant::String)
                    {
                        if (valDst.toString() > valSrc.toString())
                            isNewSrc = true;
                        if (valDst.toString() < valSrc.toString())
                            isOldDst = true;
                    }
                    else
                        if (valSrc.type() == QVariant::Date)
                        {
                            if (valDst.toDate() > valSrc.toDate())
                                isNewSrc = true;
                            if (valDst.toDate() < valSrc.toDate())
                                isOldDst = true;
                        }
                        else
                            if (valSrc.type() == QVariant::DateTime)
                            {
                                if (valDst.toDateTime() > valSrc.toDateTime())
                                    isNewSrc = true;
                                if (valDst.toDateTime() < valSrc.toDateTime())
                                    isOldDst = true;
                            }
                            else
                                if (valSrc.type() == QVariant::Double)
                                {
                                    if (qlonglong(valDst.toDouble() * 1000000) > qlonglong(valSrc.toDouble() * 1000000))
                                        isNewSrc = true;
                                    if (qlonglong(valDst.toDouble() * 1000000) < qlonglong(valSrc.toDouble() * 1000000))
                                        isOldDst = true;
                                }
                                else
                                {
                                    lastErrorString = QStringLiteral("Error comparing fields\nSync: %1.%2\nDatabase: %3 %4\n%5\n%6")
                                            .arg(name, replName, connectionSettings["type"].toString(), connectionSettings["database"].toString(), valSrc.typeName(), valDst.typeName());

                                    emit errorOccurred(lastErrorString);

                                    return false;
                                }
    return true;
}



QString SQLConnection::sqlF(const QVariant &val, const QString &vexp)
{

    if (val.type() == QVariant::Time || val.type() == QVariant::Date || val.type() == QVariant::DateTime)
        return QStringLiteral("%1 '%2'")
                .arg(vexp, val.toString().replace("T", " "));

    if (val.type() == QVariant::ULongLong || val.type() == QVariant::UInt || val.type() == QVariant::LongLong || val.type() == QVariant::Int || val.type() == QVariant::Double)
        return QStringLiteral("%1 %2")
                .arg(vexp, val.toString());

    if (val.type() == QVariant::Invalid || (val.type() == QVariant::String && val.isNull()))
    {
        if (vexp == "<>")
            return QStringLiteral(" is not null");
        if (vexp == "=")
            return QStringLiteral(" is null");
        if (vexp == "")
            return QStringLiteral("null");
    }

    if (val.type() == QVariant::String)
        return QStringLiteral("%1 '%2'")
                .arg(vexp, val.toString().replace("'", "''"));

    return QStringLiteral("%1 '%2'")
            .arg(vexp, val.toString()); // vexp.append(val.toString());
}


SQLConnection::SQLConnection(QObject *parent)
    : Connection(parent) , isInitialized(false)
{
}


SQLConnection::~SQLConnection()
{
    QStringList queryList = SqlParser::parseScript(connectionSettings["on_done"].toString());
    qDebug()<<"done";
    foreach(QString query, queryList)
    {
        if(!query.contains(QRegExp("\\S")))
            continue;

        emit eventOccurred("Query (on done):"+query);
        if (!queryService->exec(query))
        {
            lastErrorString = QStringLiteral("Error executing on_done query\nSync: %1.on_done \nDatabase: %2 %3\nQuery: %4\nError: %5")
                    .arg(name, connectionSettings["type"].toString(), connectionSettings["database"].toString(), queryService->lastQuery(), queryService->lastError().text());

            emit errorOccurred(lastErrorString);
        }
    }

    querySource=nullptr;
    queryService=nullptr;


    if (connectionSettings["type"].toString() != "current")
        QSqlDatabase::removeDatabase(connectionSettings["name"].toString());

}

QStringList SQLConnection::supportedTypes()
{
    return QStringList()<<"database";
}

QByteArray SQLConnection::dump()
{
    return QByteArray();
}

bool SQLConnection::fromDump(const QByteArray &dumpByteArray)
{
    Q_UNUSED(dumpByteArray)
    return true;
}

QVariant SQLConnection::getSingleValue(const QString &calculate, bool *ok)
{

    lastErrorString.clear();
    if (!init())
    {
        *ok = false;
        return QVariant();
    }

    if (!querySource->exec(calculate))
    {
        lastErrorString = QStringLiteral("Error executing query\nSync: %1, single shot\nDatabase: %2 %3\nQuery: %4\nError: %5")
                .arg(name, connectionSettings["type"].toString(), connectionSettings["database"].toString(), querySource->lastQuery(), querySource->lastError().text());

        *ok = false;
        return QVariant();
    }
    if (!querySource->next())
    {
        lastErrorString = QStringLiteral("No results for query\nSync: %1, single shot\nDatabase: %2 %3\nQuery: %4")
                .arg(name, connectionSettings["type"].toString(), connectionSettings["database"].toString(), querySource->lastQuery());

        *ok = false;
        return QVariant();
    }

    *ok = true;
    return querySource->value(0);
}



bool SQLConnection::open(const QJsonObject &syncSettings)
{
    if (!init())
        return false;

    QJsonObject sourceSyncSettings = syncSettings["source"].toObject();

    QString query = sourceSyncSettings["query"].toString();
    for (QMap<QString, QVariant>::const_iterator it = params.constBegin(), itEnd = params.constEnd(); it != itEnd; ++it)
    {
        query.replace(QStringLiteral("%1").arg(it.key()), it.value().toString());
    }


    emit eventOccurred("Query (source):"+query);

    if (!querySource->exec(query))
    {
        lastErrorString = QStringLiteral("Error executing query\nSync: %1.%2, source\nDatabase: %3 %4\nQuery: %5\nError: %6")
                .arg(name,syncSettings["name"].toString(),connectionSettings["type"].toString(),connectionSettings["database"].toString(),querySource->lastQuery(),querySource->lastError().text());

        emit errorOccurred(lastErrorString);


        return false;
    }

    return true;
}


bool SQLConnection::isValid()
{
    return querySource->isValid();
}

bool SQLConnection::next()
{
    return querySource->next();
}

QVariant SQLConnection::value(const QString &index)
{
    if (!querySource->isValid())
    {
        return QVariant::Invalid;
    }

    bool ok;
    int i = index.toInt(&ok) - 1;
    if (!ok)
        i = querySource->record().indexOf(index);

    if (i >= 0)
        return querySource->value(i);
    else
        return QVariant::Invalid;
}



// Открываем как приемник
bool SQLConnection::compareTo(const QJsonObject &syncSettings, Connection *src)
{
    QJsonObject destinationSyncSettings = syncSettings["destination"].toObject();
    if(destinationSyncSettings["type"].toString().contains("off")){
        return true;
    }

    if (!init())
        return false;

    replName = syncSettings["name"].toString();
    state.name = replName;
    state.qty = 0;
    state.qtyIns = 0;
    state.qtyInsFail = 0;
    state.qtyUpd = 0;
    state.qtyUpdFail = 0;
    state.qtyDel = 0;
    state.qtyDelFail = 0;
    state.qtyResolvFail = 0;

    emit eventOccurred("Comparsion started");


    QSqlDatabase db;
    if (syncSettings["name"].toString() == "current")
        db = QSqlDatabase::database();
    else
        db = QSqlDatabase::database(syncSettings["destination"].toObject()["connection"].toString());


    if (!db.open())
    {
        lastErrorString = QStringLiteral("Error connecting to the database\nSync: %1.%2, destination\nDatabase: %3 %4\nError: %5")
                .arg(name, replName, connectionSettings["type"].toString(), connectionSettings["database"].toString(), db.lastError().text());

        emit errorOccurred(lastErrorString);
        return false;
    }

    DataSync::ErrorBehavior currentErrorBehavior = errorBehavior;
    if(currentErrorBehavior==DataSync::StopOnErrorDefault)
    {
        currentErrorBehavior = DataSync::StopOnError;

        QString errB = syncSettings["on_error"].toString();
        if(errB.toLower()=="continueonerror" || errB.toLower()=="continue")
            currentErrorBehavior = DataSync::ContinueOnError;
    }
    else
        currentErrorBehavior = DataSync::StopOnError; // replace without default

    QSqlQuery     queryDestSelect(db);
    QSqlQuery     queryDestAction(db);
    // QSqlQuery     queryResolver(db);


    QStringList lst  = destinationSyncSettings["table"].toString().split(" ");
    QString     destTableName     = lst.at(0);
    QString     destTableAlias    = lst.count() == 1 ? QStringLiteral("sel_table") : lst.at(1);

    QJsonArray fields = destinationSyncSettings["fields"].toArray();

    QStringList destFields;
    QStringList destOrderFields;

    for (int i = 0; i < fields.count(); ++i)
    {
        QJsonObject field = fields.at(i).toObject();

        destFields.append(QStringLiteral("%1.%2").arg(destTableAlias, field["destination"].toString()));

        if (!field["orderer"].toString().isEmpty())
            destOrderFields.append(field["orderer"].toString());
        else
            if (field["type"].toString() == "key") // add keyfileds by numbers
                destOrderFields.append(QString::number(i+1));

        //        if (field["resolver"].toString() != "")
        //            isAnyResolver = true;
    }

    QString destOrder;
    if (!destOrderFields.isEmpty())
        destOrder = QStringLiteral("order by %1").arg(destOrderFields.join(","));

    QString destJoins = destinationSyncSettings["joins"].toString();
    QString destCondition = destinationSyncSettings["conditions"].toString();
    QString destConditionForJoins = destinationSyncSettings["joins_conditions"].toString();


    QString destWhere;

    if (!destCondition.isEmpty() || !destConditionForJoins.isEmpty())
    {
        if (!destCondition.isEmpty() && !destConditionForJoins.isEmpty())
            destWhere = QStringLiteral("where ((%1) and (%2))").arg(destCondition, destConditionForJoins); //together
        else
            destWhere =  QStringLiteral("where (%1%2)").arg(destCondition, destConditionForJoins);        //one of
    }


    bool canInsert = false;
    bool canUpdate = false;
    bool canDelete = false;

    if (destinationSyncSettings["type"].toString().contains("full")){
        canInsert = true;
        canDelete = true;
        canUpdate = true;
    }
    if (destinationSyncSettings["type"].toString().contains("insert"))
        canInsert = true;
    if (destinationSyncSettings["type"].toString().contains("update"))
        canUpdate = true;
    if (destinationSyncSettings["type"].toString().contains("delete"))
        canDelete = true;


    QString destSelect = QStringLiteral("select %1 from %2 %3 %4 %5 %6")
            .arg(destFields.join(","))
            .arg(destTableName)
            .arg(destTableAlias)
            .arg(destJoins)
            .arg(destWhere)
            .arg(destOrder);

    for (QMap<QString, QVariant>::const_iterator it = params.constBegin(), itEnd = params.constEnd(); it != itEnd; ++it)
    {
        destSelect.replace(QStringLiteral("%1").arg(it.key()), it.value().toString());
        destCondition.replace(QStringLiteral("%1").arg(it.key()), it.value().toString()); // may be used in update query
    }


    if (queryDestSelect.exec(destSelect))
    {
        emit eventOccurred("Query (destination): "+destSelect);
    }
    else
    {
        lastErrorString = QStringLiteral("Error executing query\nSync: %1.%2, destination\nDatabase: %3 %4\nQuery: %5\nError: %6")
                .arg(name, replName, connectionSettings["type"].toString(), connectionSettings["database"].toString(), queryDestSelect.lastQuery(), queryDestSelect.lastError().text());

        emit errorOccurred(lastErrorString);

        if(currentErrorBehavior==DataSync::StopOnError)
            return false;
    }

    queryDestSelect.next();
    src->next();


    if (db.driver()->hasFeature(QSqlDriver::Transactions)){
        db.transaction();
        emit eventOccurred("Transaction started");
    } else {
        emit eventOccurred("No single transaction");
    }


    //QRegularExpression re("\\%(\\w+|\\d+)"); // %field  or %fieldnumber

    while (src->isValid() || queryDestSelect.isValid())
    {
        emit eventOccurred("Iteration finished");
        state.qty += 1;


        bool isOldDst = !src->isValid() && queryDestSelect.isValid();
        bool isNewSrc = src->isValid() && !queryDestSelect.isValid();
        // qDebug()<<isOldDst<<isNewSrc;


        // check is keyfields is same
        bool isOk = true;
        if (!isOldDst && !isNewSrc)
        {
            for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
            {
                if ((*it).toObject()["type"]!="key")
                    continue;

                QVariant valSrc;

                QString resolver = (*it).toObject()["resolver"].toString();

                if (resolver.isEmpty())
                    valSrc = src->value((*it).toObject()["source"].toString());
                else
                {
                    valSrc = calcResolver(resolver, src, isOk);
                    if (!isOk)
                    {
                        if(currentErrorBehavior==DataSync::StopOnError)
                            return false;
                        else
                            break;
                    }
                }


                QVariant valDst = queryDestSelect.value((*it).toObject()["destination"].toString());

                QString s = "Compare src: "+valSrc.toString()+"; dst: "+valDst.toString()+"; need insert: "+(isNewSrc?"yes":"no")+"; need delete: "+(isOldDst?"yes":"no");
                emit eventOccurred(s);
                isOk = compare(valSrc, valDst, isNewSrc, isOldDst);

                if (!isOk || isNewSrc || isOldDst)
                    break;
            }

            if(!isOk){
                if(currentErrorBehavior==DataSync::StopOnError)
                    return false;
                else
                    continue;
            }

        }



        if (isNewSrc)
        {
            if(canInsert)
            {
                QString insertFields = "";
                QString insertValues = "";

                for (auto it = fields.constBegin(), itEnd = fields.constEnd(); it != itEnd; ++it)
                {
                    QVariant valSrc;

                    QString resolver = (*it).toObject()["resolver"].toString();

                    if (resolver.isEmpty())
                        valSrc = src->value((*it).toObject()["source"].toString());
                    else
                    {
                        valSrc = calcResolver(resolver, src, isOk);

                        if (!isOk)
                        {
                            if(currentErrorBehavior==DataSync::StopOnError)
                                return false;
                            else
                                break;
                        }
                    }

                    insertFields += QStringLiteral("%1 %2")
                            .arg(insertFields.isEmpty() ? "" : ",", (*it).toObject()["destination"].toString());

                    insertValues += QStringLiteral("%1 %2")
                            .arg(insertValues.isEmpty() ? "" : ",", sqlF(valSrc, ""));
                }

                QString insQuery = QStringLiteral("insert into %1 (%2) values (%3)")
                        .arg(destTableName, insertFields, insertValues);


                bool result;
                emit eventOccurred("Query (insert):"+insQuery);
                if(executionType ==DataSync::ExecutionPretend)
                {
                    result = true;
                }
                else
                {
                    result = queryDestAction.exec(insQuery);
                }

                if (result)
                    state.qtyIns +=1;
                else
                {
                    state.qtyInsFail +=1;

                    lastErrorString = QStringLiteral("Error executing query\nSync: %1.%2, destination\nDatabase: %3 %4\nQuery: %5\nError: %6")
                            .arg(name, replName, connectionSettings["type"].toString(), connectionSettings["database"].toString(), queryDestAction.lastQuery(), queryDestAction.lastError().text());

                    emit errorOccurred(lastErrorString);

                    if(currentErrorBehavior==DataSync::StopOnError)
                        return false;
                }
            }
            src->next();
            continue;
        }


        if (isOldDst)
        {
            if (canDelete)
            {
                QString deleteKeys = "";

                for (auto it = fields.constBegin(), itEnd = fields.constEnd(); it != itEnd; ++it)
                {
                    if((*it).toObject()["type"].toString() != "key")
                        continue;

                    QVariant valDst = queryDestSelect.value((*it).toObject()["destination"].toString());

                    deleteKeys += QStringLiteral("%1 %2 %3")
                            .arg(deleteKeys.isEmpty() ? "" : " and "
                            ,(*it).toObject()["destination"].toString()
                            ,sqlF(valDst, "="));
                }


                QString delQuery = QStringLiteral("delete from %1 where %2 (%3)")
                        .arg(destTableName
                        ,destCondition.isEmpty() ? "" : QStringLiteral("(%1) and").arg(destCondition)
                        ,deleteKeys);


                bool result;

                emit eventOccurred("Query (delete):"+delQuery);
                if(executionType ==DataSync::ExecutionPretend)
                {
                    result = true;
                }
                else
                {
                    result = queryDestAction.exec(delQuery);
                }

                if (result)
                    state.qtyDel +=1;
                else
                {
                    state.qtyDelFail +=1;

                    lastErrorString = QStringLiteral("Error executing query\nSync: %1.%2, destination\nDatabase: %3 %4\nQuery: %5\nError: %6")
                            .arg(name
                            ,replName
                            ,connectionSettings["type"].toString()
                            ,connectionSettings["database"].toString()
                            ,queryDestAction.lastQuery()
                            ,queryDestAction.lastError().text());

                    emit errorOccurred(lastErrorString);

                    if(currentErrorBehavior==DataSync::StopOnError)
                        return false;
                }
            }
            queryDestSelect.next();
            continue;
        }



        if (canUpdate)
        {
            QStringList updatePrimary;
            QStringList updateSecondary;
            QString     updateKeys = "";


            for (auto it = fields.constBegin(), itEnd = fields.constEnd(); it != itEnd; ++it)
            {
                QVariant valSrc;

                QString resolver = (*it).toObject()["resolver"].toString();

                if (resolver.isEmpty())
                    valSrc = src->value((*it).toObject()["source"].toString());
                else
                {
                    valSrc = calcResolver(resolver, src, isOk);
                    if (!isOk)
                    {
                        if(currentErrorBehavior==DataSync::StopOnError)
                            return false;
                        else
                            break;
                    }
                }

                if ((*it).toObject()["type"].toString() == "key")
                {
                    QVariant value = queryDestSelect.value((*it).toObject()["destination"].toString());
                    updateKeys += QStringLiteral(" %1 %2 %3")
                            .arg(updateKeys.isEmpty() ? "" : " and "
                            ,(*it).toObject()["destination"].toString()
                            ,sqlF(value, "="));
                }
                if ((*it).toObject()["type"].toString() == "compare")
                {
                    QVariant valDst = queryDestSelect.value((*it).toObject()["destination"].toString());

                    QVariant valDstCompare = valDst;
                    QVariant valSrcCompare = valSrc;

                    if (valDst.type() == QVariant::Double) // дробные сравним как строки
                    {
                        valDstCompare = valDst.toString();
                        valSrcCompare = valSrc.toString();
                    }
                    //qDebug()<<valSrcCompare;

                    if(valDst.type() == QVariant::Invalid && valSrc.type() == QVariant::String)
                        valDstCompare = QString();

                    if(valSrc.type() == QVariant::Invalid && valDst.type() == QVariant::String)
                        valSrcCompare = QString();

                    if (valSrcCompare != valDstCompare)
                    {
                        updatePrimary += QStringLiteral("%1 = %2")
                                .arg((*it).toObject()["destination"].toString(),sqlF(valSrc, ""));

                    }
                }
                if ((*it).toObject()["type"].toString() == "update")
                {
                    updateSecondary += QStringLiteral("%1 = %2")
                            .arg((*it).toObject()["destination"].toString(), sqlF(valSrc, ""));
                }
            }


            if (!updatePrimary.isEmpty())
            {
                // TODO Check aliaces
                QString updQuery = QStringLiteral("update %1 set %2 %3 %4 where %5 (%6)")
                        .arg(destTableName
                            ,updatePrimary.join(",")
                            ,updateSecondary.isEmpty() ? "" : ","
                            ,updateSecondary.join(",")
                            ,destCondition.isEmpty() ? "" : QStringLiteral(" (%1) and").arg(destCondition)
                            ,updateKeys);


                bool result;
                emit eventOccurred("Query (update):"+updQuery);
                if(executionType == DataSync::ExecutionPretend)
                {
                    result = true;
                }
                else
                {
                    result = queryDestAction.exec(updQuery);
                }

                if (result)
                    state.qtyUpd +=1;
                else
                {
                    state.qtyUpdFail +=1;

                    lastErrorString = QStringLiteral("Error executing query\nSync: %1.%2, destination\nDatabase: %3 %4\nQuery: %5\nError: %6")
                            .arg(name
                            ,replName
                            ,connectionSettings["type"].toString()
                            ,connectionSettings["database"].toString()
                            ,queryDestAction.lastQuery()
                            ,queryDestAction.lastError().text());

                    emit errorOccurred(lastErrorString);

                    if(currentErrorBehavior==DataSync::StopOnError)
                        return false;
                }
            }
        }


        queryDestSelect.next();
        src->next();
    }

    if (db.driver()->hasFeature(QSqlDriver::Transactions))
    {
        if (executionType == DataSync::ExecutionRollback)
        {
            db.rollback();
        }
        else
        {
            if (!db.commit())
            {
                lastErrorString = QStringLiteral("Error commiting transaction\nSync: %1.%2, destination\nDatabase: %3 %4\nError: %5")
                        .arg(name
                        ,replName
                        ,connectionSettings["type"].toString()
                        ,connectionSettings["database"].toString()
                        ,db.lastError().text());

                emit errorOccurred(lastErrorString);

                return false;
            }
        }
    }

    emit eventOccurred("Comparsion finished");

    return true;
}


