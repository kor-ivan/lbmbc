#ifndef LBPROCESS_H
#define LBPROCESS_H

#include <QObject>
#include "lbclient.h"

class lbprocess : public QObject
{
    Q_OBJECT
public:
    enum processMode {
        scan,
        autoota,
        restartall
    };
    explicit lbprocess(QObject *parent = nullptr, LBclient *lbc = nullptr);
    void run(processMode m = scan, const QString var = "sys.version");
    struct scaninfo{
        QString devtype = "unknown";
        QString mac = "unknown";
        QString version = "unknown";
        bool master = false;
    };

    enum stage {
        setBustab,
        logBustab,
        getSysvar,
        ota,
        reboot
    };

    friend QDebug operator<<(QDebug out, const lbprocess::scaninfo& inf);

    void setOtaPath(const QString &newOtaPath);

    void setNumOfVarRetries(int newNumOfVarRetries);

signals:
    void outOta(const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void outMessage (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void scanCompleted (const QMap<qsizetype, lbprocess::scaninfo>& scan);
private:
    LBclient *plbc = nullptr;
    QMap<qsizetype, scaninfo> lbscanMap;
    QMap<qsizetype, scaninfo>::Iterator i_lbscanMap;
    QList<qsizetype> lbotaKeys;
    QList<qsizetype>::reverse_iterator ri_lbota;
    stage phase;
    processMode mode;
    QString otaPath;
    inline void SendOneMessage(const QString& mess);
    inline bool preparationOta();
    inline void preparationRestart();
    inline void preparationOtaCompl();
    inline bool nextOtaSlot();
    int numOfVarRetries = 3;
    int cRetries = 0;
    QString scanVar;



private slots:
    void processOta(const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void processMessage (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void localMessage (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void localExeCompl (const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void localFinish (const QString& message, const QModbusDevice::Error error);
};

#endif // LBPROCESS_H
