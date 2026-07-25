#ifndef LBCLIENT_H
#define LBCLIENT_H

#include <QObject>
#include <QUrl>
#include <QtSerialBus/QModbusTcpClient>
//#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>
#include "lbmodbusclient.h"
#include <QTimer>
#include "lbyaml.h"
#include <QFile>
#include "lbmbc_global.h"



class LBMBC_EXPORT LBclient : public QObject
{

    Q_OBJECT
public:
    enum lbConnection{
        NotMaintainTCP,
        MaintainTCP
    };
    explicit LBclient(QObject *parent);
    explicit LBclient(QObject *parent, const QStringList postArg, lbConnection conn = NotMaintainTCP);
    explicit LBclient(QObject *parent, const std::initializer_list<QStringList> postArg, lbConnection conn = NotMaintainTCP);
    virtual ~LBclient();
    // void setTCPaddr(const QUrl);
    bool setTCPaddr(const QString addr, const int port, const QString iface="");
    void setlbHost(const QString host, const QString filename, const QString iface="");
    void Execute();
    void extracted(const QStringList &qstr);
    void setQueryString(const QStringList qstr);
    void setQueryString(const std::initializer_list<QStringList> qstr_list);
    void setTimeOut(const int t);
    void setOtaFilename(const QString path);
    void setSlot(const QStringList slot);
    void setSlot(const qsizetype slot);
    QModbusDevice::Error getlbDeviceError();
    QString getlbDeviceMessage();
    QString getlbKey();

    static const QString KeyGet;
    static const QString KeySet;
    static const QString KeyForse;
    static const QString KeyUnforse;
    static const QString KeyRestart;
    static const QString KeySettime;
    static const QString KeyStats;
    static const QString KeyGetconf;
    static const QString KeyConf;
    static const QString KeyOta;
    static const QString KeyFboot;
    static const QString KeyNofboot;
    static const QString KeyLog;
    static const QString KeyFsformat;

    void setLbConn(lbConnection newLbConn);

    bool getMulpipleRequest() const;

    void setlbiface(const QString &newLbiface);

signals:
    void ExecuteCompleted (const QString& lbhost, const QStringList& result, const QString& message, const QModbusDevice::Error error);
    void ExecuteCompletedJson (const QString& lbhost, const QJsonObject& Qjo, const QString& message, const QModbusDevice::Error error);
    void ExecuteCompletedStr (const QString& lbstr, const QString& message, const QModbusDevice::Error error);
    void lbDisconnect (const QString& lbhost, const QString& message, const QModbusDevice::Error error);
    void ExecuteFinished (const QString& message, const QModbusDevice::Error error);

private slots:
    void isConnected(QModbusDevice::State);
    void isRequestFinish();
    void isTimeout();

private:
    enum Tag {
        start = 0x0,
        update = 0x1,
        conf = 0x4,
        confstat = 0x5,
        cmds = 0x8,
        more = 0x0a,
        slot = 0x0b,
        ota_begin = 0x0c,
        ota_write = 0x0d,
        ota_end = 0x0e,
        loggrep = 0x0f,
        logmore = 0x11,
        getplace = 0x20,
        plcprog = 0x17,
    };
    enum lbalg {
        lbcmd, lbconf, lbota, lblog
    };

    struct lbbase {
        QList<qsizetype> slot;
        lbalg alg;
    };


    const qsizetype MaxLenCmds = 218;
    const qsizetype MaxLenStart = 246;
    const qsizetype MaxLenUpdate = 250;
    const qsizetype MaxLenSlot = 216;
    const qsizetype MaxLenLog = 0xF7;
    // const qsizetype MaxLenOtaSlot = 249;
    // const qsizetype MaxLenOtaWriteSlot = 244;
    static const QModbusDevice::Error lbSHAError;
    static const QString lbSHAErrorStr;
    static const QModbusDevice::Error lbJsonParseError;
    static const QString lbJsonParseErrorStr;
    static const QModbusDevice::Error lbKeyNotfoundError;
    static const QString lbKeyNotfoundErrorStr;
    static const QModbusDevice::Error lbConfError;
    static const QString lbConfErrorTimeoutStr;
    static const QString lbConfErrorFilenameStr;
    static const QModbusDevice::Error lbYamlParsingError;
    static const QString lbYamlParsingErrorStr;
    static const QString lbOtaWarningSlotStr;
    static const QString lbOtaWarningStr;
    static const QString lbOtaCompletedStr;
    static const QString lbOtaUnitStr;
    static const QString lbFbootCompletedStr;
    static const QString lbFbootUnitStr;
    static const QString lbCompletedStr;
    static const QString lbFsformatCompletedStr;

    const int lbAddr = 255;
    const int initByteCount = 1;
    const int lbNumberOfRetries = 0;
    const int lbOtaTimeout = 4000;
    const int lbOtaSlotTimeout = 6000;
    const int lbLogTimeout = 3000;
    const int lbLogSlotTimeout = 6000;
    const int lbDefaultTimeout = 1000;
    const int lbFbootTimeout = 8000;
    const qsizetype xlblogTimeout = 0x14;

    int i = 0;
    int byteCount = initByteCount;
    int lenCount = 0;
    QString lbhost;
    QString lbKey = "none";
    QString lblogKey = "a";
    int lblogfd = 0;
    quint8 lberr = 0;
    bool MulpipleRequest = false;

    lbbase lbType;
    lbConnection lbConn = NotMaintainTCP;

    lbModbusClient *lbDevice = nullptr;
    QModbusReply *lbReply = nullptr;
    QTimer *ton = nullptr;
    lbyaml *pyaml = nullptr;
    QFile *otafile = nullptr;
    QVector<QModbusRequest> lbVectorRequest;
    QVector<QModbusRequest>::iterator lbRequestIterator;
    QByteArray lbResponseBuffer;
    QByteArray lbResponseSHA;
    QJsonObject lbRequestQjo;

    void createlbRequest(QByteArray lbJsonStr);
    qsizetype lbWrapSlot(qsizetype pos, qsizetype len, QByteArray& arr, const qsizetype lev = 0);
    qsizetype lbArrReplace(qsizetype pos, QByteArray& arr);
    qsizetype getLenlbArr(qsizetype pos, QByteArray& arr, const QByteArray& JsonStr);
    inline qsizetype insertTaglbArr(QByteArray& arr, qsizetype pos, Tag tag, const QByteArray& JsonStr = 0);
    void SendAfterRequest(Tag tag);
    qsizetype getLenlbArrAfter(qsizetype pos, QByteArray& arr, Tag tag);
    qsizetype byteRemained;
    enum aTag {Acmds, Aconf, Astart, Aupdate};
    QPair<aTag,Tag> nextpack;
    bool neednext = false;

    QVector<QStringList> queryString;
    QJsonValue getJsonValue(QStringList query, const QString lbKey);
    QByteArray getJsonStr(QVector<QStringList> query);
    bool islbRespondMore(const QModbusReply *reply);
    void printResponseDiag(const QModbusResponse localMbResponse);
    void printRequestDiag(const QModbusRequest localMbRequest);
    void printResultsDiag();
    QStringList getResults(const QJsonObject &Qjo);
    void SendRequest();
    void Stoping();
    void Run();
    QByteArray getlbSHA(const QModbusResponse &response);
    QJsonParseError lbParseJson(const QByteArray &lbJsonStr, QJsonObject &paramJsonObj);
    void lbParseGetconf (QJsonDocument &qjd, QJsonParseError &qjperr, const QJsonObject &jobj, QJsonObject &pjobj);
    QJsonValue QueryToJson(QStringList query, QJsonObject (*pf)(QStringList));
    QJsonValue QueryToJson(QStringList query);
    QJsonValue FbootToJson();
    static QJsonObject methodGet(QStringList query);
    static QJsonObject methodSet(QStringList query);
    void setlbBase (const QString str);
    int NowWriteToInt (QString str);
    QString nwToFloatStr (int nw);
    QString nwToPercentStr (int nw, qint64 size);
    int nw = 0;
    QString percent = "";
    QString logstr;

    inline void insertQuint32 (QByteArray &qba, qsizetype pos, qsizetype data);
};

#endif // LBCLIENT_H
