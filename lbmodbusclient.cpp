#include "lbmodbusclient.h"
#include <qdebug.h>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>

const QString lbModbusClient::lbNotValidResponseStr = "Logic Box Response is not valid";
const QString lbModbusClient::lbErrorStr = "Logic Box Error is ";

lbModbusClient::lbModbusClient(QObject *parent)
    : QModbusTcpClient{parent}
{
    QModbusResponse::registerDataSizeCalculator(LogicBox, [] (const QModbusResponse &pdu){
        return 252;
    });
}

void lbModbusClient::setDeviceError(QString errStr, Error err)
{
    setError(errStr, err);
}

bool lbModbusClient::processPrivateResponse(const QModbusResponse &response, QModbusDataUnit *data)
{
    if (response.functionCode() != LogicBox && response.isValid()){
        setError(lbNotValidResponseStr, lbNotValidResponse);
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
        setError(lbErrorStr + getlbError(response, len), lbError);
        return false;
    }
    return true;
}

void lbModbusClient::getlbTLE(const QModbusResponse &response, quint8 &t, quint8 &l, quint8 &e)
{
    response.decodeData(&t, &l, &e);
}

// QByteArray lbModbusClient::getlbSHA(const QModbusResponse &response)
// {
//     QByteArray respSHA(response.data().remove(0, 3));
//     respSHA.resize(lbLenSHA);
//     return respSHA;
// // }

// QByteArray lbModbusClient::getlbJsonStr(const QModbusResponse &response, const quint8 &len)
// {
//     QByteArray respJson(response.data().remove(0, 3 + lbLenSHA));
//     respJson.resize(len - lbLenSHA - 1);
//     return respJson;
// }

// QJsonParseError lbModbusClient::lbParseJson(const QByteArray &lbJsonStr, QStringList &keys, QJsonObject &paramJsonObj)
// {
//     QJsonParseError errJson;
//     QJsonDocument docRead;
//     docRead = docRead.fromJson(lbJsonStr, &errJson);
//     if (errJson.error == QJsonParseError::NoError){
//         QJsonObject jObj = docRead.object();
//         keys = jObj.keys();
//         if (!keys.empty()){
//             if (keys[0] == "get"){
//                 paramJsonObj = jObj["get"].toObject();
//             }
//         }
//         return errJson;
//     }else{
//         return errJson;
//     }
// }

// QList<quint16> lbModbusClient::getlbValue(const QJsonObject lbJsonObj)
// {
//     QList<quint16> val;
//     if (!lbJsonObj.empty()){
//         QStringList key = lbJsonObj.keys();
//         for (int i=0;i<lbJsonObj.size(); i++){
//             val.append(lbJsonObj[key[i]].toDouble());
//         }
//     }
//     return val;
// }

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

