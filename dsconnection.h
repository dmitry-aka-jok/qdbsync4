#ifndef DSCONNECTION_H
#define DSCONNECTION_H

#include <QVariant>
#include <QMap>
#include <QObject>

#include <QJsonDocument>
#include <QJsonObject>

namespace DataSync
{

enum ErrorBehavior
{
    StopOnErrorDefault,
    StopOnError,
    ContinueOnError
};
enum ExecutionType
{
    ExecutionNormal,
    ExecutionPretend,
    ExecutionRollback
};
struct SyncEntry
{
  QString name;
  int qty;
  int qtyIns;
  int qtyInsFail;
  int qtyUpd;
  int qtyUpdFail;
  int qtyDel;
  int qtyDelFail;
  int qtyResolvFail;
};



class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = nullptr);
    ~Connection();

    static QStringList supportedTypes();

    void setSettings(const QString &name, const QJsonObject &connectionSettings);

    // for single value
    virtual QVariant getSingleValue(const QString &calculate, bool *ok = nullptr) = 0;


    // for opening as a source
    virtual bool open(const QJsonObject &syncSettings) = 0;
    virtual bool isValid() = 0;
    virtual bool next() = 0;
    virtual QVariant value(const QString &index) = 0;


    virtual QByteArray dump() = 0;
    virtual bool fromDump(const QByteArray &dumpByteArray) = 0;

    // compare as destination
    void setErrorBehavior(ErrorBehavior errorBehavior);
    void setExecutionType(ExecutionType executionType);
    void setParams(const QMap<QString, QVariant> &params);

    virtual bool compareTo(const QJsonObject &syncSettings, Connection *src) = 0;

    QString errorString();
    SyncEntry getSyncEntry();

signals:
    void eventOccurred(const QString&);
    void errorOccurred(const QString&);

protected:
    QJsonObject connectionSettings;

    QString name;
    QString lastErrorString;

    QMap<QString, QVariant> params;
    ErrorBehavior errorBehavior;
    ExecutionType executionType;
    SyncEntry state;
};
}

#endif // DSCONNECTION_H
