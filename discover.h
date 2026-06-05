#ifndef DISCOVER_H
#define DISCOVER_H

#include <QObject>
#include <QNetworkInterface>
// #include <QCoreApplication>
#include "icmp6scanner.h"
#include "lbclient.h"
#include "lbmbc_global.h"

class LBMBC_EXPORT discover : public QObject
{
    Q_OBJECT
public:
    explicit discover(QObject *parent = nullptr);
    // explicit discover(QCoreApplication *a, QObject *parent = nullptr);
    virtual ~discover();
    void execute(int ifindex = 0);
    static QList<QNetworkInterface> getlbIfDiscover();
    struct lbinfo {
        QString name = "noname";
        QString type = "unknown";
        QString ipv4 = "unknown";
        QString mac = "unknown";
        QList<float> delay;
        QList<int> ifindex;
        bool btn = false;
        QString LBMBC_EXPORT toString() const;
    };

    friend LBMBC_EXPORT QDebug operator<<(QDebug out, const discover::lbinfo& inf);

    int getWaitingTime() const;
    void setWaitingTime(int newWaitingTime);
    enum discoverError{
        NoError,
        SomeError
    };
    static QString addColonsToMac (const QString &mac);

signals:
    void discoverCompleted (const QMap<QString, discover::lbinfo>& DiscoverMap, const discover::discoverError error, const QString errorStr);
    void icmpResponseReceived (const QString& ipv6, const int& delay, const int& ifindex);
private:
    QMap<QString, lbinfo> getlbDiscoverMap();
    const QString lbMulticastAddr = "ff02::4c6f:6769:6342:6f78";
    const QString NoErrorStr = "No error";
    int waitingTime = 1000;
    QMap<QString, lbinfo> lbDiscoverMap;

    // QCoreApplication *app = nullptr;
    QMap<QString, LBclient*> lbcMap;
    QMap<QString, bool> lbFinishMap;
    QList<Icmp6Scanner*> scanner;
    int ScannCount = 0;
    int lbcCount = 0;
    void getResults();
    float delaytofloat(int d);


private slots:
    void isIfscanIsFinish(const QString& name);
    void isResponseReceived(const QString& from, int rttmcs, int ifindex);
    void isExecuteCompleted (const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void isExecuteCompletedJson (const QString& lbhost, const QJsonObject& Qjo, const QString& message, const QModbusDevice::Error error);
    void islbHostDisconnect (const QString& lbhost, const QString& message, const QModbusDevice::Error error);
};

#endif // DISCOVER_H
