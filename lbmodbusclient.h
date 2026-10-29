#ifndef LBMODBUSCLIENT_H
#define LBMODBUSCLIENT_H

// #include "lbclient.h"
#include <QModbusTcpClient>
#include <QModbusRtuSerialClient>
#include <QJsonParseError>


//#include <QCoreApplication>


class lbModbusClient
{
public:
    explicit lbModbusClient();
    virtual ~lbModbusClient() = default;

    static const QModbusPdu::FunctionCode LogicBox = (QModbusPdu::FunctionCode)0x6b;
    static const qsizetype lbLenSHA = 32;
    QString getlbError(const QModbusResponse &response, const quint8 &len);

    int OtaTimeOutCount = 0;
    int maxOTC = 3;
    static const quint16 TimeOutError = 0x74;
    static const quint16 LogEagain = 0x0b;

    void setEnablePassTimeOut(bool newEnablePassTimeOut);

    // Абстрактные прокси-методы, чтобы LBclient мог управлять сетью через этот же указатель
    virtual void setDeviceError(QString errStr, QModbusDevice::Error err) = 0;
    virtual QModbusDevice::State state() const = 0;
    virtual QModbusDevice::Error error() const = 0;
    virtual QString errorString() const = 0;
    virtual bool connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void setTimeout(int timeOut) = 0;
    virtual QModbusReply *sendRawRequest(const QModbusRequest &request, int serverAddress) = 0;
    virtual void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) = 0;
    virtual void setNumberOfRetries(int number) = 0;

protected:
    bool processLogic(const QModbusResponse &response, QModbusDataUnit *data);
    static const QModbusDevice::Error lbNotValidResponse;
    static const QModbusDevice::Error lbError;
    static const QString lbNotValidResponseStr;
    static const QString lbErrorStr;


    void getlbTLE(const QModbusResponse &response, quint8 &t, quint8 &l, quint8 &e);
    void initOtaWarning();
    bool WarningOta = false;
    bool enablePassTimeOut = false;
};

class lbModbusTcpClient : public QModbusTcpClient, public lbModbusClient
{
    Q_OBJECT
public:
    explicit lbModbusTcpClient(QObject *parent = nullptr);

    // Реализация прокси-методов управления Qt (просто перенаправляем в методы базового QModbusTcpClient)
    void setDeviceError(QString errStr, QModbusDevice::Error err) override { setError(errStr, err); }
    QModbusDevice::State state() const override { return QModbusTcpClient::state(); }
    QModbusDevice::Error error() const override { return QModbusTcpClient::error(); }
    QString errorString() const override { return QModbusTcpClient::errorString(); }
    bool connectDevice() override { return QModbusTcpClient::connectDevice(); }
    void disconnectDevice() override { QModbusTcpClient::disconnectDevice(); }
    void setTimeout(int timeOut) override { QModbusTcpClient::setTimeout(timeOut); }
    QModbusReply *sendRawRequest(const QModbusRequest &request, int serverAddress) override { return QModbusTcpClient::sendRawRequest(request, serverAddress); }
    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) override { QModbusTcpClient::setConnectionParameter(parameter, value); }
    void setNumberOfRetries(int number) override { QModbusTcpClient::setNumberOfRetries(number); }

protected:
    bool processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data) override;
};

class lbModbusRtuClient : public QModbusRtuSerialClient, public lbModbusClient
{
    Q_OBJECT
public:
    explicit lbModbusRtuClient(QObject *parent = nullptr);

    // Реализация прокси-методов управления Qt (просто перенаправляем в методы базового QModbusTcpClient)
    void setDeviceError(QString errStr, QModbusDevice::Error err) override { setError(errStr, err); }
    QModbusDevice::State state() const override { return QModbusRtuSerialClient::state(); }
    QModbusDevice::Error error() const override { return QModbusRtuSerialClient::error(); }
    QString errorString() const override { return QModbusRtuSerialClient::errorString(); }
    bool connectDevice() override { return QModbusRtuSerialClient::connectDevice(); }
    void disconnectDevice() override { QModbusRtuSerialClient::disconnectDevice(); }
    void setTimeout(int timeOut) override { QModbusRtuSerialClient::setTimeout(timeOut); }
    QModbusReply *sendRawRequest(const QModbusRequest &request, int serverAddress) override { return QModbusRtuSerialClient::sendRawRequest(request, serverAddress); }
    void setConnectionParameter(QModbusDevice::ConnectionParameter parameter, const QVariant &value) override { QModbusRtuSerialClient::setConnectionParameter(parameter, value); }
    void setNumberOfRetries(int number) override { QModbusRtuSerialClient::setNumberOfRetries(number); }

protected:
    bool processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data) override;
};

#endif // LBMODBUSCLIENT_H
