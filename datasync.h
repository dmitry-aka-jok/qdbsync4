#ifndef DATASYNC_H
#define DATASYNC_H

//#include "datasync_global.h"

#include <QThread>
#include <QString>
#include <QByteArray>

#include "dsconnection.h"

namespace DataSync {

enum State
{
    Idle,
    Working,
    Fail,
    Complete
};

//DATASYNC_EXPORT
class  DataSyncEngine : public QThread
{
    Q_OBJECT
public:
    explicit DataSyncEngine(QObject* parent = nullptr);

    bool setConfig(const QByteArray &config);

    void setErrorBehavior(ErrorBehavior errorBehavior);
    void setExecutionType(ExecutionType executionType);
    void setParams(QMap<QString, QString> params);

    void run() Q_DECL_OVERRIDE;

    QString lastError();
    QString lastExecuted();

    DataSync::State getState();

    SyncEntry getSyncEntry();
    void clearSyncEntry();
    QMap<int, SyncEntry> getSyncEntryCompleted();

signals:
    void eventOccurred(const QString&);
    void errorOccurred(const QString&);

protected:
    int currSync;
    DataSync::State state;

    QString lastErrorString;
    QString lastQuery;

    DataSync::ErrorBehavior errorBehavior;
    DataSync::ExecutionType executionType;
    QJsonObject obj;

    QMap<QString, QString> dparams;
    QMap<int, SyncEntry>syncEntryCompleted;
    SyncEntry syncEntry;

    QMap<QString, Connection*> connections;
};
}

#endif // DATASYNC_H
