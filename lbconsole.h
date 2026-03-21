#ifndef LBCONSOLE_H
#define LBCONSOLE_H

#include <QObject>
#include <QCoreApplication>
// #include <QModbusDevice>
#include <QCommandLineParser>
#include "lbclient.h"

class lbconsole : public QObject
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


private slots:
    void printOta(const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void printMessage (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void printDisconnect (const QString& lbhost, const QString& message, const QModbusDevice::Error error);
};

#endif // LBCONSOLE_H
