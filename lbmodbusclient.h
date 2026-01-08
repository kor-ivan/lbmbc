#ifndef LBMODBUSCLIENT_H
#define LBMODBUSCLIENT_H

// #include "lbclient.h"
#include <QModbusTcpClient>
#include <QJsonParseError>


//#include <QCoreApplication>


class lbModbusClient : public QModbusTcpClient
{
public:
    explicit lbModbusClient(QObject *parent = nullptr);

    static const QModbusPdu::FunctionCode LogicBox = (QModbusPdu::FunctionCode)0x6b;
    static const qsizetype lbLenSHA = 32;
    void setDeviceError (QString errStr, QModbusDevice::Error err);
    QString getlbError(const QModbusResponse &response, const quint8 &len);

    int OtaTimeOutCount = 0;
    int maxOTC = 3;
    static const quint16 TimeOutError = 0x74;
    static const quint16 LogEagain = 0x0b;

    void setEnablePassTimeOut(bool newEnablePassTimeOut);

private:
    bool processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data) override;
    static const QModbusDevice::Error lbNotValidResponse = (QModbusDevice::Error)0x10;
    static const QModbusDevice::Error lbError = (QModbusDevice::Error)0x11;
    static const QString lbNotValidResponseStr;
    static const QString lbErrorStr;


    void getlbTLE(const QModbusResponse &response, quint8 &t, quint8 &l, quint8 &e);
    // QByteArray getlbSHA(const QModbusResponse &response);
    // QByteArray getlbJsonStr(const QModbusResponse &response, const quint8 &len);
    // QJsonParseError lbParseJson(const QByteArray &lbJsonStr, QStringList &keys, QJsonObject &paramJsonObj);
    // QList<quint16> getlbValue(const QJsonObject lbJsonObj);
    void initOtaWarning();
    bool WarningOta = false;
    bool enablePassTimeOut = false;
};

#endif // LBMODBUSCLIENT_H
