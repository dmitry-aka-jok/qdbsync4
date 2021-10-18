#include "sqlparser.h"

SqlParser::SqlParser()
{

}

QStringList SqlParser::parseScript(const QString &s)
{
    QStringList result;

    int triggerDepth=0;
    int stringDepth=0;
    bool skipOneBegin=false;
    QString queryFromParts;

    for(int i=0; i<s.count();++i)
    {
        if(stringDepth==0  &&  s.mid(i,14).toUpper() == "CREATE TRIGGER")
        {
            triggerDepth++;
            skipOneBegin=true;
        }
        if(stringDepth==0  &&  triggerDepth > 0 && s.mid(i,5).toUpper() == "BEGIN")
        {
            if(skipOneBegin)
                skipOneBegin=false;
            else
                triggerDepth++;
        }
        if(stringDepth==0  &&  triggerDepth > 0 && s.mid(i,4).toUpper() == "CASE")
                triggerDepth++;

        if(stringDepth==0  &&  triggerDepth > 0 && s.mid(i,3).toUpper() == "END")
                triggerDepth--;

        if(s.at(i) == '\'')
        {
           stringDepth = (stringDepth + 1) & 1;
        }

        queryFromParts.append(s.at(i));

        if((s.at(i)==';' || i+1 == s.length()) && triggerDepth==0 && stringDepth==0)
        {
            bool skipQuery = false;
            if(queryFromParts.contains("CREATE TABLE sqlite_sequence"))
                skipQuery=true;

            if(queryFromParts.trimmed().isEmpty())
                skipQuery=true;

            if(queryFromParts.trimmed()==";")
                skipQuery=true;

            if(!skipQuery)
            {
                result<<queryFromParts;
            }
            queryFromParts.clear();
        }

    }
    return result;
}

ParsedQuery SqlParser::parseQuery(const QString &s)
{
    ParsedQuery result;

    int stringDepth=0;
    int bracketsDepth=0;
    QString part;
    SqlParserParts::QueryPart current = SqlParserParts::queryNone;
    SqlParserParts::QueryPart before = SqlParserParts::queryNone;
    QString test;
    int skipWordsToFields = 0;

    for(int i=0; i<s.count();++i)
    {
        before = current;

        if(stringDepth == 0 && bracketsDepth == 0)
        {
            test = "SELECT";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
            {
                current = SqlParserParts::queryCommand;
                skipWordsToFields = 1;
            }
            test = "FROM";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryTables;
            test = "WHERE";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryConditions;
            test = "ORDER BY";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryOrderers;
            test = "HAVING BY";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryHaving;
            test = "GROUP BY";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryGroupers;

            test = "LIMIT";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
                current = SqlParserParts::queryFinisher;
            test = "FIRST";
            if(s.mid(i,test.length()).toUpper() == test && s.at(i+test.length()).isSpace())
            {
                current = SqlParserParts::queryStarter;
                skipWordsToFields = 2;
            }
        }

        if(skipWordsToFields!=0)
            if(s.at(i).isSpace() and !s.at(i-1).isSpace())
            {
                --skipWordsToFields;
                if(skipWordsToFields==0)
                    current = SqlParserParts::queryFields;
            }

        if(current != before)
        {
            result[before] = part;
            part.clear();
        }

        if(s.at(i) == '(' && stringDepth == 0)
        {
           bracketsDepth++;
        }
        if(s.at(i) == ')' && stringDepth == 0)
        {
           bracketsDepth--;
        }

        if(s.at(i) == '\'')
        {
           stringDepth = (stringDepth + 1) & 1;
        }

        part.append(s.at(i));

    }
    result[current] = part;

    return result;
}

QStringList SqlParser::parseFields(const QString &s)
{
    QStringList result;

    int stringDepth=0;
    int bracketsDepth=0;
    QString part;

    for(int i=0; i<s.count();++i)
    {
        if(s.at(i) == '\'')
        {
           stringDepth = (stringDepth + 1) & 1;
        }
        if(s.at(i) == '(' && stringDepth == 0)
        {
           bracketsDepth++;
        }
        if(s.at(i) == ')' && stringDepth == 0)
        {
           bracketsDepth--;
        }

        if(s.at(i)==',' && stringDepth==0 && bracketsDepth==0)
        {
            if(!part.trimmed().isEmpty())
                result<<part.trimmed();

            part.clear();
        }
        else
            part.append(s.at(i));
    }
    if(!part.trimmed().isEmpty())
        result<<part.trimmed();

    return result;
}




