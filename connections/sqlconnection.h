#ifndef SQLCONNECTION_H
#define SQLCONNECTION_H

#include <QSqlQuery>
#include <QSharedPointer>

#include "../dsconnection.h"


class SQLConnection : public DataSync::Connection
{
    Q_OBJECT
public:
    explicit SQLConnection(QObject *parent = nullptr);
    ~SQLConnection();

//    void setSettings(const QString &name, const QJsonObject &connectionSettings);
    static QStringList supportedTypes();

    QByteArray dump();
    bool fromDump(const QByteArray &dumpByteArray);

    QVariant getSingleValue(const QString &calculate, bool *ok = nullptr);

    // for opening as a source
    bool open(const QJsonObject &syncSettings);
    bool isValid();
    bool next();
    QVariant value(const QString &index);

    // for opening as destination
    bool compareTo(const QJsonObject &syncSettings, Connection *src);

protected:
    bool isInitialized;
    bool init();
    QVariant calcResolver(const QString &resolver, Connection *src, bool &isOk);
    bool compare(const QVariant &valSrc, const QVariant &valDst, bool &isNewSrc, bool &isOldDst);
    QString sqlF(const QVariant &val, const QString &vexp);

    QMap<QString, QVariant> resolvers;
    QString replName;
    QSqlQuery* querySource;
    QSqlQuery* queryService;
};
#endif // SQLCONNECTION_H
