#ifndef SYNCCONTROLLERCLI_H
#define SYNCCONTROLLERCLI_H

#include <QObject>
#include <QFile>
#include <QTextStream>

#include "dsconnection.h"
#include "datasync.h"

class SyncControllerCLI : public QObject
{
    Q_OBJECT
public:
    explicit SyncControllerCLI(QObject *parent = nullptr);
    ~SyncControllerCLI();

    enum OutputType
    {
        CLI,
        Results,
        Verbose,
        Silent
    };

    void run(const QByteArray& settings, QMap<QString, QString>& params, DataSync::ExecutionType executionType, DataSync::ErrorBehavior errorBehavior, SyncControllerCLI::OutputType outputType);


public slots:
    void eventOccurred(const QString& text);
    void errorOccurred(const QString& text);
//    void queryExecuted(const QString& text);

signals:

protected:
    void printState(const DataSync::SyncEntry &ss, bool clearPrev, bool moveToNext);

    DataSync::DataSyncEngine ds;
    QTextStream *out;

    int iterationsCount;
    int iterationsToSend;
    int lastLength;

    DataSync::ExecutionType executionType;
    SyncControllerCLI::OutputType outputType;

};

#endif // SYNCCONTROLLERCLI_H
