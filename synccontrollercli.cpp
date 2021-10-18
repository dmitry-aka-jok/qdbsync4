#include "synccontrollercli.h"

#include <QString>

#include <QDebug>

SyncControllerCLI::SyncControllerCLI(QObject *parent) : QObject(parent), lastLength(0)
{
    out = new QTextStream(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered);
}

SyncControllerCLI::~SyncControllerCLI()
{
    delete out;
}

void SyncControllerCLI::run(const QByteArray& settings, QMap<QString, QString>& params, DataSync::ExecutionType executionType, DataSync::ErrorBehavior errorBehavior, OutputType outputType)
{
    this->outputType    = outputType;
    this->executionType = executionType;


    if(!ds.setConfig(settings))
       *out<<"Error in config:"<<Qt::endl<<ds.lastError();

    ds.setParams(params);
    ds.setErrorBehavior(errorBehavior);
    ds.setExecutionType(executionType);


    iterationsCount = 1;
    iterationsToSend= 1;
    DataSync::DataSyncEngine dd;
    connect(&ds, &DataSync::DataSyncEngine::eventOccurred, this, &SyncControllerCLI::eventOccurred);

    connect(&ds, &DataSync::DataSyncEngine::errorOccurred, this, &SyncControllerCLI::errorOccurred);

    /*
    if (outputType==SyncControllerCLI::Verbose)
    {
        connect(&ds, &DataSync::DataSyncEngine::queryExecuted, this, SLOT(queryExecuted(QString)));
    }
    */

    ds.run();

    if(ds.getState()==DataSync::Fail)
    {
        *out<<"Error:"<<ds.lastError()<<Qt::endl;
    }
}

void SyncControllerCLI::errorOccurred(const QString &text)
{
    *out<<Qt::endl<<Qt::endl<<text<<Qt::endl;
}

void SyncControllerCLI::eventOccurred(const QString &text)
{

    if(text=="Iteration finished" && outputType == OutputType::CLI)
    {
      printState(ds.getSyncEntry(), true, false);
    }
    if(text=="Comparsion started" && outputType == OutputType::CLI)
    {
      printState(ds.getSyncEntry(), true, false);
    }
    if(text=="Comparsion finished"  && (outputType == OutputType::CLI || outputType == OutputType::Results))
    {
      printState(ds.getSyncEntry(), true, true);
    }
    if(text=="Synchronization started" && (outputType == OutputType::CLI || outputType == OutputType::Results))
    {
      printState(ds.getSyncEntry(), false, true);
    }
    if(text=="Synchronization finished" && (outputType == OutputType::CLI || outputType == OutputType::Results))
    {
      printState(ds.getSyncEntry(), true, true);
    }


    if(outputType == OutputType::Verbose)
        if(!text.startsWith("Iteration") && !text.startsWith("Comparsion") && !text.startsWith("Synchronization"))
        {
           QStringList sl = text.split("/n");
           for(const auto &s : qAsConst(sl))
            *out<<s<<Qt::endl;

        }
}

void SyncControllerCLI::printState(const DataSync::SyncEntry &ss, bool clearPrev, bool moveToNext)
{
    QRegExp re("\\d{1,2}\\.\\d{1,2}\\.\\d{1,4} \\d{1,2}:\\d{1,2}:\\d{1,2}");

    QString answer;
    if(re.exactMatch(ss.name))
    {
        answer = QStringLiteral("|%1|  count | inserts|   falis| deletes|   fails| updates|   fails|")
                    .arg(ss.name.leftJustified(30,' ',true));
    }
    else
    {
        answer = QStringLiteral("|%1|%2|%3|%4|%5|%6|%7|%8|")
                .arg(ss.name.leftJustified(30,' ',true),
                    QString::number(ss.qty).rightJustified(8,' '),
                    QString::number(ss.qtyIns).rightJustified(8,' '),
                    QString::number(ss.qtyInsFail).rightJustified(8,' '),
                    QString::number(ss.qtyDel).rightJustified(8,' '),
                    QString::number(ss.qtyDelFail).rightJustified(8,' '),
                    QString::number(ss.qtyUpd).rightJustified(8,' '),
                    QString::number(ss.qtyUpdFail).rightJustified(8,' ')
                    );
    }

    int lastLengthSave = lastLength;
    if(clearPrev)
        for(int i=0; i<lastLength; ++i)
            *out<<"\b";

    *out<<answer;
    lastLength = answer.length();

    if(clearPrev)
    {
        for(int i=lastLength; i<lastLengthSave; ++i)
            *out<<" ";
    }

    if(moveToNext)
        *out<<Qt::endl;

    out->flush();
}

