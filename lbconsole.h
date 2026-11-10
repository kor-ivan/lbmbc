#ifndef LBCONSOLE_H
#define LBCONSOLE_H

#include <QObject>
#include <QCoreApplication>
// #include <QModbusDevice>
#include <QCommandLineParser>
#include "lbclient.h"
#include "lbmbc_global.h"

class LBMBC_EXPORT lbconsole : public QObject
{
    Q_OBJECT
public:
    explicit lbconsole(QCoreApplication *a = nullptr);
    virtual ~lbconsole();
    void implement();

signals:
    void lbQuit();

private:
    const QString DefaultHost = "127.0.0.1:502";
    const QString DefaultTimeout = "1";
    const QString DefaultFunction = "3";
    const QString DefaultMBaddr = "1";
    const QString DefaultReg = "0";
    const QString DefaultQuantity = "10";
    //const QString DefaultlbhostOption = "";
    const QString DefaultYamlConf = "conf.yml";
    const QString DefaultInterface = "0";
    const QString DefaultFileName = "";
    const QString DefaultBaund = "9600";

    QCoreApplication* app = nullptr;

    int prc_old;
    // void stop();
    void setlbAddr(const QCommandLineParser &parser, LBclient* lbc,
                   const QCommandLineOption &lbhostOption,
                   const QCommandLineOption &lbYamlConfOption,
                   const QCommandLineOption &lbMacOption,
                   const QCommandLineOption &hostOption,
                   const QCommandLineOption &lbInterfaceOption);

    void printStatstr(const lbyaml::lbvarstat &statstr, lbyaml &yaml);
    void printlbHostMmap(const QMultiMap<QString, QString> &lbHostMmap);
    void printlbVarMap(const QMap<QString, lbyaml::lbvar> &lbVarMap);
    QStringList wrapText(QString text, int width);
    QString formatList(const QList<QStringList> &list);
    bool parseSerialArgs(const QCommandLineParser &parser, QString &portName, int &addr, QStringList &parts);

    template <typename T>
    bool configureSerial(T *client, const QCommandLineParser &parser){
        QString portName;
        int addr = 0;
        QStringList parts;

        if (!parseSerialArgs(parser, portName, addr, parts)) {
            return false;
        }

        switch (parts.size()) {
        case 1: // Указан только addr
            client->setSerialAddr(portName, addr);
            break;
        case 2: // Указаны addr/baudRate
            client->setSerialAddr(portName, addr, parts.at(1).toInt());
            break;
        case 3: // Указаны addr/baudRate/dataBits
            client->setSerialAddr(portName, addr, parts.at(1).toInt(), parts.at(2).toInt());
            break;
        case 4: // Указаны addr/baudRate/dataBits/parity
            client->setSerialAddr(portName, addr, parts.at(1).toInt(), parts.at(2).toInt(), parts.at(3).toInt());
            break;
        default: // Указаны все 5 параметров или более (лишние игнорируются)
            client->setSerialAddr(portName, addr, parts.at(1).toInt(), parts.at(2).toInt(), parts.at(3).toInt(), parts.at(4).toInt());
            break;
        }

        return true;
    }

private slots:
    void printOta(const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void printMessage (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void printDisconnect (const QString& lbhost, const QString& message, const QModbusDevice::Error error);
};

#endif // LBCONSOLE_H
