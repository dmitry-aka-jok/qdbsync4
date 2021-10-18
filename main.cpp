#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>

#include <QDebug>

#include "synccontrollercli.h"

int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Sync");
    QCoreApplication::setApplicationVersion("4.0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Tool for syncing databases");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("file", "File with rules to sync.");

    QCommandLineOption verboseOption(QStringList() << "b" << "verbose", "Verbose mode. Prints a lot of debug information.");
    parser.addOption(verboseOption);
    QCommandLineOption silentOption(QStringList() << "i" << "silent", "Silent mode. Prints only errors.");
    parser.addOption(silentOption);
    QCommandLineOption consoleOption(QStringList() << "o" << "console", "Console mode. Prints results for every string.");
    parser.addOption(consoleOption);
    QCommandLineOption stopOption(QStringList() << "s" << "stop-on-error", "When got an error - stop process");
    parser.addOption(stopOption);
    QCommandLineOption continueOption(QStringList() << "c" << "continue-on-error", "When got an error - skip line process");
    parser.addOption(continueOption);
    QCommandLineOption pretendOption(QStringList() << "n" << "pretend", "Only prepeare query but didn't run");
    parser.addOption(pretendOption);
    QCommandLineOption rollbackOption(QStringList() << "r" << "rollback", "Execute query but didn't run");
    parser.addOption(rollbackOption);

    // An option with a value
    QCommandLineOption paramsOption(QStringList() << "p" << "params", "List of overrided params","params like param1=value,param2=value");
    parser.addOption(paramsOption);

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if(args.length()==0){
        qCritical()<<"No file to sync ";
        return -1;
    }
    QString filename = args.at(0);

    SyncControllerCLI::OutputType output = SyncControllerCLI::CLI;
    if (parser.isSet(verboseOption))
        output = SyncControllerCLI::Verbose;
    if (parser.isSet(silentOption))
        output = SyncControllerCLI::Silent;
    if (parser.isSet(consoleOption))
        output = SyncControllerCLI::Results;

    DataSync::ErrorBehavior errorBehavior = DataSync::StopOnErrorDefault;
    if (parser.isSet(continueOption))
        errorBehavior= DataSync::ContinueOnError;
    if (parser.isSet(stopOption))
        errorBehavior= DataSync::StopOnError;

    DataSync::ExecutionType execType = DataSync::ExecutionNormal;
    if (parser.isSet(pretendOption))
        execType = DataSync::ExecutionPretend;
    if (parser.isSet(rollbackOption))
        execType = DataSync::ExecutionRollback;

    QMap<QString, QString> params;
    QStringList paramsList = parser.value(paramsOption).split(",");
    for(const QString &paramSingle : qAsConst(paramsList))
    {
        QStringList paramSingleList = paramSingle.split("=");
        if (paramSingleList.count() == 2)
            params[paramSingleList.at(0)] = paramSingleList.at(1);
    }


    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)){
        qCritical()<<"Cannot open file: "<<filename;
        return -1;
    }
    QByteArray config = file.readAll();


   SyncControllerCLI *cn = new  SyncControllerCLI();
   cn->run(config, params, execType, errorBehavior, output);


    return 0; //app.exec();
}
