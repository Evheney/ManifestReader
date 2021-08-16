#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QXmlStreamReader>
#include <qtconcurrentmap.h>

//using namespace QtConcurrent;

/*
bool XmlManifestReader::read(QIODevice *device)
{
    reader.setDevice(device);

    if (reader.readNextStartElement()) {
        if (reader.name() == "ePM-Xray") {
            readEpmXrayInfo();

            createComponents();
            //readBoard();
        }
        else {
            reader.raiseError(QObject::tr("Not a Scheme file"));
        }
    }


    return !reader.error();
}
while(reader.readNextStartElement()){
    if(reader.name() == "format") {
        info.setFormatVer(reader.attributes().value("ver").toString());
        info.setFormatName(reader.attributes().value("name").toString());
        //scheme->addEpmXrayInfo(info);
        reader.skipCurrentElement();
    }

*/
/*
    Utility function that recursivily searches for files.
*/
QStringList findFiles(const QString &startDir, const QStringList &filters)
{
    QStringList names;
    QDir dir(startDir);

    const auto files = dir.entryList(filters, QDir::Files);
    for (const QString &file : files)
        names += startDir + '/' + file;

    const auto subdirs =  dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &subdir : subdirs)
        names += findFiles(startDir + '/' + subdir, filters);
    return names;
}

int runBatchQtQProcessWaited(const std::string& BatchFile) {
    QProcess Process;
    auto Command = QString("cmd.exe");
    auto Arguments = QStringList{
            QString("/C"),
            QString::fromStdString(BatchFile)
    };

    //Process.start(Command, Arguments);
    //Process.waitForFinished(-1);
    int rc = Process.execute(Command, Arguments);

//    QByteArray out_data = Process.readAllStandardOutput();
//    QString out_string(out_data);
//    qDebug() << out_string.toStdString().c_str();

    return rc; //Process.exitCode();
}

void Extract(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString line = fi.path()+"/"+fi.baseName();
    qDebug() << line;

    QString systemCommand("apktool d -s --force-all -o " + line + " " + fileName);
    qDebug() << systemCommand;

    runBatchQtQProcessWaited(systemCommand.toStdString());
}

typedef QMap<QString, int> PermissionCount;

typedef QMap<QString, int> BuiltPermissionCount;
typedef QMap<QString, int> CustomPermissionCount;

typedef QMap<QString, int> IntentCount ;

class PermissionsAndIntents
{
public:
    BuiltPermissionCount builtPerms;
    CustomPermissionCount customPerms;
    IntentCount intentsActivity;
    IntentCount intentsReceiver;
};

PermissionsAndIntents countPermissions(const QString &file){
    QFile f(file);
    f.open(QIODevice::ReadOnly| QFile::Text);

    QXmlStreamReader reader;
    reader.setDevice(&f);
    PermissionsAndIntents pi;

    while(reader.readNextStartElement()) {
        //qDebug() << reader.name();
        if(reader.name() == "manifest"){
            while(reader.readNextStartElement()) {
                QStringRef name = reader.name();
                if(reader.name() == "uses-permission") {
                    QString v = reader.attributes().value("android:name").toString();
                    if (v.contains("android.permission"))
                        pi.builtPerms[v]++;
                    else
                        pi.customPerms[v]++;
                    reader.skipCurrentElement();
                } else if(reader.name() == "application"){
                    while(reader.readNextStartElement()){
                        if(reader.name() == "activity"){
                            while(reader.readNextStartElement()){
                                  if(reader.name() == "intent-filter"){
                                      while(reader.readNextStartElement()){
                                          if(reader.name() == "action"){
                                              QString v = reader.attributes().value("android:name").toString();
                                              if (v.contains("android.intent.action"))
                                                pi.intentsActivity[v]++;
                                              reader.skipCurrentElement();
                                          }else if(reader.name() == "category"){
                                              QString v = reader.attributes().value("android:name").toString();
                                              if (v.contains("android.intent.category"))
                                                pi.intentsActivity[v]++;
                                              reader.skipCurrentElement();
                                          }
                                          else
                                              reader.skipCurrentElement();
                                      }
                                  } else
                                      reader.skipCurrentElement();
                            }
                        }
                        else if(reader.name() == "receiver"){
                            while(reader.readNextStartElement()){
                                  if(reader.name() == "intent-filter"){
                                      while(reader.readNextStartElement()){
                                          if(reader.name() == "action"){
                                              QString v = reader.attributes().value("android:name").toString();
                                              if (v.contains("android.intent.action"))
                                                pi.intentsReceiver[v]++;
                                              reader.skipCurrentElement();
                                          }else if(reader.name() == "category"){
                                              QString v = reader.attributes().value("android:name").toString();
                                              if (v.contains("android.intent.category"))
                                                pi.intentsReceiver[v]++;
                                              reader.skipCurrentElement();
                                          }
                                          else
                                              reader.skipCurrentElement();
                                      }
                                  } else
                                      reader.skipCurrentElement();
                            }
                        }
                        else
                            reader.skipCurrentElement();
                    }
                }
                else
                    reader.skipCurrentElement();
            }
        }
    }

    return pi;
}
void reduce(PermissionsAndIntents &piTotal,const PermissionsAndIntents &pi) {
    QMapIterator<QString, int> i(pi.builtPerms);
    while (i.hasNext()) {
        i.next();
        piTotal.builtPerms[i.key()] += i.value();
    }
    QMapIterator<QString, int> ic(pi.customPerms);
    while (ic.hasNext()) {
        ic.next();
        piTotal.customPerms[ic.key()] += ic.value();
    }
    QMapIterator<QString, int> ii(pi.intentsActivity);
    while (ii.hasNext()) {
        ii.next();
        piTotal.intentsActivity[ii.key()] += ii.value();
    }
    QMapIterator<QString, int> ir(pi.intentsReceiver);
    while (ir.hasNext()) {
        ir.next();
        piTotal.intentsReceiver[ir.key()] += ir.value();
    }
}
void singleThreadedBothPermissionCount(const QStringList &files, BuiltPermissionCount& builtinPerms, CustomPermissionCount& customPerms)
{
    for (const QString &file : files) {
        QFile f(file);
        f.open(QIODevice::ReadOnly| QFile::Text);

        QXmlStreamReader reader;
        reader.setDevice(&f);

        while(reader.readNextStartElement()) {
            //qDebug() << reader.name();
            if(reader.name() == "manifest"){
                while(reader.readNextStartElement()) {
                    if(reader.name() == "uses-permission") {
                        QString v = reader.attributes().value("android:name").toString();
                        if (v.contains("android.permission"))
                            builtinPerms[v] ++;
                        else
                            customPerms[v] ++;
                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
        }

        // output permissions of file
        //...
    }

}

PermissionCount singleThreadedPermissionCount(const QStringList &files)
{
    PermissionCount permissionCount;
    for (const QString &file : files) {
        QFile f(file);
        f.open(QIODevice::ReadOnly| QFile::Text);

        QXmlStreamReader reader;
        reader.setDevice(&f);

        while(reader.readNextStartElement()) {
            //qDebug() << reader.name();
            if(reader.name() == "manifest"){
                while(reader.readNextStartElement()) {
                    if(reader.name() == "uses-permission") {
                        QString v = reader.attributes().value("android:name").toString();
                        permissionCount[v] += 1;
                        reader.skipCurrentElement();
                    }
                    else
                        reader.skipCurrentElement();
                }
            }
        }
    }
    return permissionCount;
}

void printPermissions(const QMap<QString, int>& map)
{
    QMapIterator<QString, int> i(map);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": " << i.value() ;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // DECLARING parser and setting default options and positional arguments
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Manifest reader."));
    parser.addPositionalArgument("source directory", QObject::tr("Input source directory."));
    parser.addHelpOption();

    // PARSING options
    parser.process(a);

    // CHECKING and USING options
    const QStringList args = parser.positionalArguments();
    for (auto& a: args)
        qDebug() << "Arg:" << a;
    QString sourcedir;// = "D:/Eugene/Damkook/Single20/";
    if(args.size() > 0 ){
        sourcedir = args.at(0);
    } else {
        qDebug() << "Specify source directory as first agrument, e.g.";
        qDebug() << ">manifestreader D:/ExtractedManifestFolders/";
        return 0;
    }

    qDebug() << "Finding manifest files in " << sourcedir;

    QStringList files = findFiles(sourcedir, QStringList() << "AndroidManifest.xml");
    qDebug() << files.count() << "files";

    //qDebug() << "warmup";
    /*{
        PermissionCount total = singleThreadedPermissionCount(files);

        qDebug() << "Output of permissions";

        int builtPerms = 0;
        int customPerms = 0;

        QMapIterator<QString, int> i(total);
        while (i.hasNext()) {
            i.next();

            if (i.key().contains("android.permission"))
                builtPerms++;
            else
                customPerms++;

            qDebug() << i.key() << ": " << i.value() ;
        }
        qDebug() << "Total number of unique build-in permissions:" << builtPerms;
        qDebug() << "number of custom permissions are:" << customPerms;
    }
    */

//    {
//        BuiltPermissionCount builtPerms;
//        CustomPermissionCount customPerms;
//        singleThreadedBothPermissionCount(files, builtPerms, customPerms);

//        qDebug() << "Total number of unique build-in permissions:" << builtPerms.size();
//        qDebug() << "Total number of custom permissions are:" << customPerms.size();

//        qDebug() << "Unique built in permissions:";
//        printPermissions(builtPerms);

//        qDebug() << "Unique custom permissions:";
//        printPermissions(customPerms);
//    }

    int singleThreadTime = 0;
    {
        QTime time;
        time.start();
        PermissionsAndIntents piTotal;
        for (const QString &file : files) {
            PermissionsAndIntents pi = countPermissions(file);

                reduce(piTotal,pi);
//            QMapIterator<QString, int> i(pi.builtPerms);
//            while (i.hasNext()) {
//                i.next();
//                piTotal.builtPerms[i.key()] += i.value();
//            }
//            QMapIterator<QString, int> ic(pi.customPerms);
//            while (ic.hasNext()) {
//                ic.next();
//                piTotal.customPerms[ic.key()] += ic.value();
//            }
        }
        singleThreadTime = time.elapsed();
        qDebug() << "Single thread time elapsed" << singleThreadTime;
        qDebug() << "Total number of unique build-in permissions:" << piTotal.builtPerms.size();
        qDebug() << "Total number of custom permissions are:" << piTotal.customPerms.size();

//        qDebug() << "Unique built in permissions:";
//        printPermissions(piTotal.builtPerms);

//        qDebug() << "Unique custom permissions:";
//        printPermissions(piTotal.customPerms);
    }
    int mapReduceTime = 0;
    {
        QTime time;
        time.start();
        PermissionsAndIntents piTotal = QtConcurrent::mappedReduced(files, countPermissions, reduce);
        mapReduceTime = time.elapsed();
        qDebug() << "MapReduce time" << mapReduceTime;

        qDebug() << "Total number of unique build-in permissions:" << piTotal.builtPerms.size();
        qDebug() << "Total number of custom permissions are:" << piTotal.customPerms.size();

        qDebug() << "Multi thread MapReduce speedup x" << ((double)singleThreadTime - (double)mapReduceTime) / (double)mapReduceTime + 1;

        qDebug() << "\nBuilt-in Permissions:";
        {
            int counter=0;
            QMapIterator<QString, int> i(piTotal.builtPerms);
            while (i.hasNext()) {
                i.next();
                qDebug() << ++counter << ":" << i.key() << ": " << i.value() ;
            }
        }
        qDebug() << "\nCustom Permissions:";
        {
            int counter=0;
            QMapIterator<QString, int> i(piTotal.customPerms);
            while (i.hasNext()) {
                i.next();
                qDebug() << ++counter << ":"<< i.key() << ": " << i.value() ;
            }
        }
        qDebug() << "\nIntentsActivity:";
        {
            int counter=0;
            QMapIterator<QString, int> i(piTotal.intentsActivity);
            while (i.hasNext()) {
                i.next();
                qDebug() << ++counter << ":"<< i.key() << ": " << i.value() ;
            }
        }
        qDebug() << "\nIntentsReceiver:";
        {
            int counter=0;
            QMapIterator<QString, int> i(piTotal.intentsReceiver);
            while (i.hasNext()) {
                i.next();
                qDebug() << ++counter << ":"<< i.key() << ": " << i.value() ;
            }
        }

    }

    return 1;//a.exec();
}
