#include "lbmodbusclient.h"
#include <qdebug.h>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>

const QModbusDevice::Error lbModbusClient::lbNotValidResponse = (QModbusDevice::Error)0x10;
const QModbusDevice::Error lbModbusClient::lbError = (QModbusDevice::Error)0x11;


const QString lbModbusClient::lbNotValidResponseStr = "Logic Box Response is not valid";
const QString lbModbusClient::lbErrorStr = "Logic Box Error is ";

lbModbusClient::lbModbusClient()
{}

// void lbModbusClient::setDeviceError(QString errStr, Error err)
// {
//     setError(errStr, err);
// }

bool lbModbusClient::processLogic(const QModbusResponse &response, QModbusDataUnit *data)
{
    if (response.functionCode() != LogicBox && response.isValid()){
        setDeviceError(lbNotValidResponseStr, lbNotValidResponse);
        return false;
    }
    quint8 tag=0, len=0, err=0;
    getlbTLE(response, tag, len, err);
    QList<quint16> val;
    val.append(tag);
    val.append(len);
    val.append(err);
    data->setValues(val);
    if (err == 0 || err == LogEagain){
        if (WarningOta)
            initOtaWarning();
        return true;
    }
    else if (err == TimeOutError && OtaTimeOutCount<maxOTC && enablePassTimeOut){
        WarningOta = true;
        OtaTimeOutCount++;
        return true;
        }
    else{
        setDeviceError(lbNotValidResponseStr, lbNotValidResponse);
        return false;
    }
    return true;
}

void lbModbusClient::getlbTLE(const QModbusResponse &response, quint8 &t, quint8 &l, quint8 &e)
{
    response.decodeData(&t, &l, &e);
}


void lbModbusClient::initOtaWarning()
{
    OtaTimeOutCount = 0;
    WarningOta = false;
}

void lbModbusClient::setEnablePassTimeOut(bool newEnablePassTimeOut)
{
    enablePassTimeOut = newEnablePassTimeOut;
}


QString lbModbusClient::getlbError(const QModbusResponse &response, const quint8 &len)
{
    QByteArray respErr(response.data().remove(0, 3));
    respErr.resize(len - 1);
    QString str(respErr);
    return str;
}


lbModbusTcpClient::lbModbusTcpClient(QObject *parent)
    : QModbusTcpClient(parent)
{
    QModbusResponse::registerDataSizeCalculator(LogicBox, [](const QModbusResponse &){ return 252; });
}

bool lbModbusTcpClient::processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data)
{
    return this->processLogic(response, data);
}


lbModbusRtuClient::lbModbusRtuClient(QObject *parent)
    : QModbusRtuSerialClient(parent)
{
    QModbusResponse::registerDataSizeCalculator(LogicBox, [](const QModbusResponse &){ return 252; });
}

bool lbModbusRtuClient::processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data)
{
    return this->processLogic(response, data);
}



