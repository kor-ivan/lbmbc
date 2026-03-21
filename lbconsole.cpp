#include "version.h"
#include "lbconsole.h"
#include "discover.h"
#include "lbprocess.h"
#include <iostream>
#include "mbclient.h"

// #include "icmp6scanner.h"
#ifdef Q_OS_LINUX
#include <sys/ioctl.h>
#include <unistd.h>
#endif

lbconsole::lbconsole(QCoreApplication *a)
    : QObject{a}, app(a)
{
    connect(this, &lbconsole::lbQuit, app, &QCoreApplication::quit, Qt::QueuedConnection);
}

lbconsole::~lbconsole()
{

}

void lbconsole::implement()
{
    // QCoreApplication::setApplicationVersion("v.0.0.1");
    QString fullVersionString = QString(APP_VERSION_STRING);
    QCoreApplication::setApplicationVersion(fullVersionString);
    QCommandLineParser parser;
    parser.setApplicationDescription("Client for Logic Box PLC");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(LBclient::KeyGet, "Command for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeySet, "Set variable for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyForse, "Forse variable for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyUnforse, "Unforse variable for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyRestart, "Restart for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeySettime, "Set time for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyStats, "Statistics for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyGetconf, "Get configuration for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyConf, "Command to configuration Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyOta, "Firmware updates for for Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyFboot, "Download fboot to Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyNofboot, "Remove fboot from Logic Box PLC");
    parser.addPositionalArgument(LBclient::KeyLog, "Requesting a log from Logic Box PLC, may be use the options a, e, f, i, r, s, v (by default, only a is used). For example: log a10f");
    parser.addPositionalArgument(LBclient::KeyFsformat, "formats the file system PLC");
    parser.addPositionalArgument("discover", "Sending ICMPv6 discover request");
    parser.addPositionalArgument("ifconfig", "Information about network interfaces available for ICMPv6 requests");
    parser.addPositionalArgument("mactoip", "Converts a mac address to an ipv6 address using EUI-64");
    parser.addPositionalArgument("autoota", "Automatic firmware download in all PLC modules, need --fw option");
    parser.addPositionalArgument("scan", "Reads the configuration of all PLC modules, need --ip or --host or --mac option");
    parser.addPositionalArgument("gethosts", "Outputs all PLC names and mac addresses specified in the Yaml file options --yaml");
    parser.addPositionalArgument("getvar", "Provides an analysis of Yaml variables in the host specified in the --host option");
    parser.addPositionalArgument("test", "Logic box test argument");

    QCommandLineOption hostOption({{"i","ip"},"IP address or host Modbus slave (Default: 127.0.0.1:502)","host:port",DefaultHost});
    parser.addOption(hostOption);
    QCommandLineOption timeoutOption({{"t","time"},"Timeout between requests (Default: 1s)","time",DefaultTimeout});
    parser.addOption(timeoutOption);
    QCommandLineOption functionOption({{"f","func"},"Modbus Function 01 Coil Registers 0x, 02 Input Registers 1x, 03 Holding Registers 4x, 04 Input Registers 3x"
                                                      "(Default: 03 Holding Registers 4x)","func",DefaultFunction});
    parser.addOption(functionOption);
    QCommandLineOption MBaddrOption({{"id","servaddr"},"Modbus network server Address (Default: 1)","servaddr",DefaultMBaddr});
    parser.addOption(MBaddrOption);
    QCommandLineOption RegOption({{"a","addr"},"The starting address for reading (Default: 0)","addr",DefaultReg});
    parser.addOption(RegOption);
    QCommandLineOption QuantOption({{"q","quant"},"Number of registers to be read (Default: 10)","quant",DefaultQuantity});
    parser.addOption(QuantOption);
    QCommandLineOption WriteOption({{"w","write"},"Write single coil or single holding register selected in the addr","value",DefaultReg});
    parser.addOption(WriteOption);
    QCommandLineOption lbhostOption({"n","host"},"Logic Box hostname in conf.yml, cannot be selected together with --ip and --mac option","plcNNN");
    parser.addOption(lbhostOption);
    QCommandLineOption lbYamlConfOption({{"y","yaml"},"Path to configuration file Logic Box","conf.yml", DefaultYamlConf});
    parser.addOption(lbYamlConfOption);
    QCommandLineOption lbInterfaceOption({{"if","eth"},"Network interface index for sending ICMPv6 discover Logic Box Request (Default: sending to all interface)","eth", DefaultInterface});
    parser.addOption(lbInterfaceOption);
    QCommandLineOption lbFileNameOption({{"fw","file"},"The path to the firmware or other file","path", DefaultFileName});
    parser.addOption(lbFileNameOption);
    QCommandLineOption lbMacOption({"m","mac"},"Logic Box mac addres, cannot be selected together with --ip and --host option","00:1A:2B:3C:4D:5E");
    parser.addOption(lbMacOption);
    QCommandLineOption lbYamlExtVar("ext","Extended output of variable analysis in YAML");
    parser.addOption(lbYamlExtVar);
    QCommandLineOption lbTest("test","Logic Box test option","");
    parser.addOption(lbTest);
    parser.process(*app);

    if (!parser.positionalArguments().isEmpty()){
        if (parser.positionalArguments().contains("discover")){
            discover *lbd = new discover(this);
            connect(lbd, &discover::discoverCompleted, this,
                [this] (const QMap<QString, discover::lbinfo>& DiscoverMap, const discover::discoverError error, const QString errorStr){
                if (error == discover::NoError){
                    foreach (const auto i, DiscoverMap)
                        qDebug()<<i;
                    emit lbQuit();
                }else{
                    qDebug().noquote()<<errorStr;
                    emit lbQuit();
                }
            }
                    );
            connect(lbd, &discover::icmpResponseReceived, this,
                    [this] (const QString& ipv6, const int& delay, const int& ifindex){
                        qDebug() << "Ответ от:" << ipv6 << "RTT:" << delay << "мкс" << ifindex;
                    }
                    );
            lbd->execute(parser.value(lbInterfaceOption).toInt());
        }else if (parser.positionalArguments().contains("ifconfig")){
            discover *lbd = new discover(app);
            foreach (QNetworkInterface i, lbd->getlbIfDiscover()) {
                qDebug()<<"Interface name :"<<i.humanReadableName();
                qDebug()<<"Interface index:"<<i.index();
                foreach (QNetworkAddressEntry j, i.addressEntries()) {
                    qDebug()<<j.ip().toString();
                }
                qDebug()<<"===========================================";
            }
            delete lbd;
            emit lbQuit();
        }else if(parser.positionalArguments().contains("mactoip")){
            if (parser.isSet(lbMacOption))
                qDebug().noquote()<<lbyaml::MacToIPv6(parser.value(lbMacOption));
            else
                qDebug().noquote()<<"MAC option is not selected";
            emit lbQuit();
        }else if (parser.positionalArguments().contains("scan") || parser.positionalArguments().contains("autoota") ||
                   parser.positionalArguments().contains("restartall")){
            qDebug().noquote()<<"scan";
            LBclient *albc = new LBclient(this);
            setlbAddr(parser, albc, lbhostOption, lbYamlConfOption, lbMacOption, hostOption, lbInterfaceOption);
            connect(albc, &LBclient::lbDisconnect, this, &lbconsole::printDisconnect);
            lbprocess *lbproc = new lbprocess(this, albc);
            connect(lbproc, &lbprocess::outMessage, this, &lbconsole::printMessage);
            connect(lbproc, &lbprocess::scanCompleted, this,
                    [](const QMap<qsizetype, lbprocess::scaninfo>& scan){
                auto i = scan.begin();
                for (auto i = scan.begin(); i != scan.end(); ++i) {
                    qDebug()<<i.key()<<i.value();
                }
            }
            );
            if (parser.positionalArguments().contains("autoota")){
                // if (parser.isSet(lbFileNameOption)){
                    lbproc->setOtaPath(parser.value(lbFileNameOption));
                    connect(lbproc, &lbprocess::outOta, this, &lbconsole::printOta);
                    lbproc->run(lbprocess::autoota);
                // }else{
                //     qDebug().noquote()<<"File option is not selected";
                //     emit lbQuit();
                // }
            }else if (parser.positionalArguments().contains("scan"))
                lbproc->run(lbprocess::scan, parser.positionalArguments().value(1, "sys.version"));
            else if (parser.positionalArguments().contains("restartall"))
                lbproc->run(lbprocess::restartall);   
        }
        else if (parser.positionalArguments().contains("gethosts") || parser.positionalArguments().contains("getvar")){
            lbyaml *y = new lbyaml(parser.value(lbYamlConfOption));
            if (parser.positionalArguments().contains("gethosts")){
                printlbHostMmap(y->getallhost());
            }else if (parser.positionalArguments().contains("getvar")){
                y->setlbhost(parser.value(lbhostOption));
                lbyaml::lbvarstat statstr = y->getVarStat();
                if (parser.isSet(lbYamlExtVar))
                    printlbVarMap(y->getLbVarMap());
                printStatstr(statstr, *y);
            }
            delete y;
            emit lbQuit();
        }
        else if (parser.positionalArguments().contains("test")){
            qDebug()<<"testing";
            lbyaml *y = new lbyaml(parser.value(lbYamlConfOption));
            QMultiMap<QString, QString> HostMmap;
            // y->getallhost(HostMmap);
            for (auto it = HostMmap.begin(); it != HostMmap.end(); ++it) {
                std::cout << std::setw(12) <<std::left <<
                    it.key().toStdString() <<
                    it.value().toStdString()<<std::endl;
            }
            delete y;
            emit lbQuit();
        }
        else{
            LBclient *lbc = new LBclient(this, parser.positionalArguments());
            qDebug().noquote()<<lbc->getlbKey(); //print LogicBox Key
            if (lbc->getlbKey()==LBclient::KeyOta || lbc->getlbKey()==LBclient::KeyFboot){
                // qDebug()<<"LBclient::KeyOta || LBclient::KeyFboot";
                connect(lbc, &LBclient::ExecuteCompleted, this, &lbconsole::printOta);
            }else if (lbc->getlbKey()==LBclient::KeyGetconf){
                connect(lbc, &LBclient::ExecuteCompletedJson, this,
                        [](const QString& lbhost, const QJsonObject& Qjo, const QString& message, const QModbusDevice::Error error){
                            if(error==QModbusDevice::NoError){
                                qDebug()<<"# BEGIN YAML";
                                lbyaml::printlbconf(Qjo);
                                qDebug()<<"# END YAML";
                            }
                            else
                                qDebug().noquote()<<message;
                        }
                        );
            }else{
                connect(lbc, &LBclient::ExecuteCompletedStr, this,&lbconsole::printMessage);
            }
            connect(lbc, &LBclient::lbDisconnect, this, &lbconsole::printDisconnect);
            setlbAddr(parser, lbc, lbhostOption, lbYamlConfOption, lbMacOption, hostOption, lbInterfaceOption);
            if (parser.isSet(timeoutOption))
                lbc->setTimeOut(parser.value(timeoutOption).toFloat()*1000);
            if (parser.isSet(lbFileNameOption))
                lbc->setOtaFilename(parser.value(lbFileNameOption));
            if (parser.isSet(lbTest)){
                qDebug()<<"testing";
                lbc->setQueryString({{"get","sys.devtype","sys.version"},{"getconf"}});
            }
            lbc->Execute();
        }
    }else{
        MBclient *mbc = new MBclient(app);
        QUrl url = QUrl::fromUserInput(parser.value(hostOption));
        if (url.port()==-1)
            url.setPort((QUrl::fromUserInput(DefaultHost)).port());
        mbc->setTCPaddr(url);
        if (!parser.isSet(WriteOption)){
            mbc->setTimeOut(parser.value(timeoutOption).toFloat()*1000);
            mbc->Execute(parser.value(functionOption).toInt(), parser.value(RegOption).toInt(), parser.value(QuantOption).toInt(), parser.value(MBaddrOption).toInt());
        }else{
            mbc->WriteExecute(parser.value(functionOption).toInt(), parser.value(WriteOption).toInt(), parser.value(RegOption).toInt(), parser.value(MBaddrOption).toInt());
        }
    }
}

// void lbconsole::stop()
// {
//     emit lbQuit();
//     // app->quit();
//     // app->deleteLater();
// }

void lbconsole::setlbAddr(const QCommandLineParser &parser, LBclient *lbc, const QCommandLineOption &lbhostOption, const QCommandLineOption &lbYamlConfOption, const QCommandLineOption &lbMacOption, const QCommandLineOption &hostOption, const QCommandLineOption &lbInterfaceOption)
{
    QString host;
    if (parser.isSet(lbMacOption))
        host = lbyaml::MacToIPv6(parser.value(lbMacOption));
    else
        host = parser.value(hostOption);

    if(parser.isSet(lbhostOption)){
        if (parser.isSet(lbInterfaceOption))
            lbc->setlbHost(parser.value(lbhostOption), parser.value(lbYamlConfOption),(parser.value(lbInterfaceOption)));
        lbc->setlbHost(parser.value(lbhostOption), parser.value(lbYamlConfOption));
        }
    else if (parser.isSet(lbInterfaceOption)){
        lbc->setTCPaddr(host, 502, parser.value(lbInterfaceOption));
    }
    else{
        QUrl url = QUrl::fromUserInput((parser.isSet(lbMacOption))?
                                           lbyaml::MacToIPv6(parser.value(lbMacOption)):
                                           parser.value(hostOption));
        url.setPort((QUrl::fromUserInput(DefaultHost)).port());
        lbc->setTCPaddr(host, QUrl::fromUserInput(DefaultHost).port());
    }
}

void lbconsole::printStatstr(const lbyaml::lbvarstat &statstr, lbyaml &yaml)
{
    qDebug().noquote()<<"Number of variables :"<<statstr.quantity;
    qDebug().noquote()<<"Must Multisource :" << (statstr.mustMultisource.empty() ? "none" : statstr.mustMultisource.join(", "));
    qDebug().noquote()<<"Handling Var :" << (statstr.handlingVar.empty() ? "none" : statstr.handlingVar.join(", "));
    qDebug().noquote()<<"not added Forte :" << (statstr.noaddedForte.empty() ? "none" : statstr.noaddedForte.join(", "));
    qDebug().noquote()<<"Error :" << yaml.getErr();
}

void lbconsole::printlbHostMmap(const QMultiMap<QString, QString> &lbHostMmap)
{
    // Настраиваем отступ для колонок (например, 20 символов)
    int colWidth = 20;

    // Формируем заголовок таблицы
    qDebug().nospace().noquote() << "\n"
                            << QString("Host").leftJustified(colWidth)
                            << "| " << QString("MAC").leftJustified(colWidth) << "\n"
                            << QString("-").repeated(colWidth * 2 + 3);

    // Итерируемся по всем элементам
    auto it = lbHostMmap.constBegin();
    while (it != lbHostMmap.constEnd()) {
        qDebug().nospace().noquote() << it.key().leftJustified(colWidth)
        << "| " << it.value().leftJustified(colWidth);
        ++it;
    }
}

void lbconsole::printlbVarMap(const QMap<QString, lbyaml::lbvar> &lbVarMap)
{
    // 1. Задаем фиксированную ширину колонок
    const int wN = 16, wV = 20, wO = 20, wM = 5, wR = 5, wI = 6;

    // Формат строки (без лишних пробелов в начале/конце)
    auto fmt = [=](const QString& a, const QString& b, const QString& c,
                   const QString& d, const QString& e, const QString& f) {
        return QString("%1 | %2 | %3 | %4 | %5 | %6")
        .arg(a, -wN).arg(b, -wV).arg(c, -wO).arg(d, -wM).arg(e, -wR).arg(f, -wI);
    };

    QString table;
    QString header = fmt("VarName", "Var", "Var_out", "Multi", "Retain", "Init");
    table += header + "\n" + QString(header.length(), '-') + "\n";

    for (auto i = lbVarMap.begin(); i != lbVarMap.end(); ++i) {
        const auto &v = i.value();

        // Разбиваем длинные списки на части
        QStringList names = wrapText(i.key(), wN);
        QStringList vars  = wrapText(formatList(v.var), wV);
        QStringList outs  = wrapText(formatList(v.var_out), wO);

        // Определяем, сколько строк займет эта запись
        int maxRows = qMax(names.size(), qMax(vars.size(), outs.size()));

        for (int r = 0; r < maxRows; ++r) {
            table += fmt(
                         r < names.size() ? names[r] : "",
                         r < vars.size()  ? vars[r]  : "",
                         r < outs.size()  ? outs[r]  : "",
                         (r == 0) ? (v.multisource ? "true" : "false") : "",
                         (r == 0) ? (v.retain ? "true" : "false") : "",
                         (r == 0) ? (v.init.isEmpty() ? "-" : v.init) : ""
                         ) + "\n";
        }
        // Небольшой разделитель между разными переменными для читаемости (опционально)
        // table += QString(header.length(), '.') + "\n";
    }

    qDebug().noquote() << (QString("\n") + table);
}

QStringList lbconsole::wrapText(QString text, int width)
{
    if (text.isEmpty()) return {"-"};
    QStringList result;
    for (int i = 0; i < text.length(); i += width) {
        result << text.mid(i, width).trimmed();
    }
    return result;
}

QString lbconsole::formatList(const QList<QStringList> &list)
{
    if (list.isEmpty()) return "-";
    QStringList groups;
    for (const auto &subList : list) {
        groups << "(" + subList.join(",") + ")";
    }
    return groups.join(" ");
}


void lbconsole::printOta(const QString &lbhost, const QStringList &result, const QString &message, const QModbusDevice::Error error)
{
    if(error==QModbusDevice::NoError){
    int width = 50;
#ifdef Q_OS_LINUX
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width = std::max(10, w.ws_col - 61);
#endif
        int prc = (int)result.at(1).toFloat();
        // float cbt = result.at(0).toFloat()/1000;
        int pos = (result.at(1).toFloat() * width) / 100;
        if (prc!=prc_old || message!=""){
            std::cout << "\r[";
            for (int j = 0; j < width; ++j)
                std::cout << (j < pos ? "=" : (j == pos ? ">" : " "));
            std::cout << "] " << std::setw(3) << prc <<"% "<<
                result.at(0).toStdString()<<" "<<result.at(2).toStdString()<<" "<<std::setw(42)<<message.toStdString();
            std::cout<<std::flush;
            if (prc==100)
                std::cout<<std::endl;
        }
        prc_old = prc;
    }
    else
        qDebug().noquote()<<" "<<message;
}

void lbconsole::printMessage(const QString &lbstr, const QString &message, const QModbusDevice::Error error)
{
    if(error==QModbusDevice::NoError)
        qDebug().noquote()<<lbstr;
    else
        qDebug().noquote()<<message;
}

void lbconsole::printDisconnect(const QString &lbhost, const QString &message, const QModbusDevice::Error error)
{
    qDebug().noquote()<<message;
    emit lbQuit();
}
