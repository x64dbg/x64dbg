#include "ApiFingerprints.h"
#include "Configuration.h"



/**
 * @brief retrieves information (name, arguments) for given api function from database
 * @param name of dll without ".dll"
 * @param name of function
 * @remark upper or lower case doesn't matter
 * @return
 */
const APIFunction ApiFingerprints::function(QString dllname,QString functionname) const
{
    return Library.constFind(dllname.toLower().trimmed()).value().constFind(functionname.toLower().trimmed()).value();
    /*
     * example
     * --------------------
     * "int MessageBoxA(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType)"
     *
     * APIFunction f = function("UsEr32 ","messAgebOxa");
     * f.Name = "MessageBoxA";
     * f.DLLName = "user32";
     * f.ReturnType = "int"
     * f.Arguments.at(0).Name = "hWnd";
     * f.Arguments.at(0).Type = "HWND";
     * f.Arguments.at(1).Name = "lpText";
     * f.Arguments.at(1).Type = "LPCTSTR";
     * ...
     *
     * upper / lower case doesn't matter and additional whitespace will be trimmed
     */


}


ApiFingerprints::ApiFingerprints()
{
    // the config file should contain a list of possible data files for api calls
    QList<QString> files = Configuration::instance()->ApiFingerprints();

    // iterate all given files
    foreach(QString file, files)
    {
        QFile mFile("data/"+file+".txt");
        if(mFile.open(QFile::ReadOnly | QFile::Text))
        {
            // if file exists --> parse file

            QMap<QString, APIFunction> Functions;
            QTextStream in(&mFile);
            while ( !in.atEnd() )
            {
                // reads raw line like "int;MessageBoxA;HWND hWnd;LPCTSTR lpText;LPCTSTR lpCaption;UINT uType;"
                QString rawFunctionDescr = in.readLine();
                QStringList functionParts = rawFunctionDescr.split(";");
                // format :    retType;FunctionName;Arg1;Arg2;Arg3;...


                if(functionParts.count()<2)
                {
                    // each function description needs at least a return type and a name
                    // if not, we ignore the data
                    continue;
                }

                // function data
                APIFunction func;
                func.DLLName = file;
                func.ReturnType = functionParts.at(0);
                func.Name = functionParts.at(1);

                // read parameters
                for(int i=2; i<functionParts.count(); i++)
                {
                    QString rawArgument = functionParts.at(i).trimmed();
                    if(rawArgument.length() == 0)
                        break;

                    // some functions have pointers as arguments --> use "*" to split type and name of argument
                    QStringList par = rawArgument.split("*");
                    APIArgument arg;
                    if(par.count() > 1)
                    {
                        arg.Name = par.at(1).trimmed();
                        arg.Type = par.at(0).trimmed()+"*";

                    }
                    else
                    {
                        // current argument is no pointer --> use " " to split
                        par = rawArgument.split(" ");
                        if(par.count()>1)
                        {
                            arg.Name = par.at(1).trimmed();
                            arg.Type = par.at(0).trimmed();
                        }
                        else
                        {
                            // we assume that there is only the type
                            arg.Name = "";
                            arg.Type = rawArgument.trimmed();
                        }
                    }

                    func.Arguments.append(arg);

                }

                Functions.insert(func.Name.toLower().trimmed(),func);

            }

            Library.insert(file,Functions);
            mFile.close();
        }

    }

}
