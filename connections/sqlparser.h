#ifndef SQLPARSER_H
#define SQLPARSER_H

#include <QStringList>
#include <QMap>
#include <QString>

namespace SqlParserParts {
    enum QueryPart
    {
        queryNone,
        queryCommand,
        queryStarter,
        queryFields,
        queryTables,
        queryConditions,
        queryGroupers,
        queryOrderers,
        queryHaving,
        queryFinisher
    };
}

typedef QMap<SqlParserParts::QueryPart, QString> ParsedQuery;


class SqlParser
{
public:
    SqlParser();

    static QStringList parseScript(const QString&);

    static ParsedQuery parseQuery(const QString&);
    static QStringList parseFields(const QString&);
};

#endif // SQLPARSER_H
