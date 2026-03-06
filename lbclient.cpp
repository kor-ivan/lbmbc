#include "lbclient.h"
#ifdef Q_OS_LINUX
#include "discover.h"
#endif
#include <QHostAddress>

const QModbusDevice::Error LBclient::lbSHAError = (QModbusDevice::Error)0x12;
const QModbusDevice::Error LBclient::lbJsonParseError = (QModbusDevice::Error)0x13;
const QModbusDevice::Error LBclient::lbKeyNotfoundError = (QModbusDevice::Error)0x14;
const QModbusDevice::Error LBclient::lbConfError = (QModbusDevice::Error)0x15;
const QModbusDevice::Error LBclient::lbYamlParsingError = (QModbusDevice::Error)0x16;

const QString LBclient::KeyGet = "get";
const QString LBclient::KeySet = "set";
const QString LBclient::KeyForse = "force";
const QString LBclient::KeyUnforse = "unforce";
const QString LBclient::KeyRestart = "restart";
const QString LBclient::KeySettime = "settime";
const QString LBclient::KeyStats = "stats";
const QString LBclient::KeyGetconf = "getconf";
const QString LBclient::KeyConf = "conf";
const QString LBclient::KeyOta = "ota";
const QString LBclient::KeyFboot = "fboot";
const QString LBclient::KeyNofboot = "nofboot";
const QString LBclient::KeyLog = "log";
const QString LBclient::KeyFsformat = "fsformat";

const QString LBclient::lbSHAErrorStr = "Logic Box SHA256 control is not valid";
const QString LBclient::lbJsonParseErrorStr = "Logic Box JSON parse error: ";
const QString LBclient::lbKeyNotfoundErrorStr = "Logic Box key not found";

const QString LBclient::lbOtaWarningSlotStr = " Ota warning: waiting reply from slot";
const QString LBclient::lbOtaWarningStr = " Ota warning: waiting reply";
const QString LBclient::lbOtaCompletedStr = "Ota has completed successfully, please restart";
const QString LBclient::lbOtaUnitStr = "kb";
const QString LBclient::lbConfErrorTimeoutStr = "Set timeOut between requests out of range";
const QString LBclient::lbConfErrorFilenameStr = "File not found";
const QString LBclient::lbYamlParsingErrorStr = "Error parsing YAML: ";
const QString LBclient::lbFbootCompletedStr = "Fboot loaded correctly";
const QString LBclient::lbFbootUnitStr = "package";
const QString LBclient::lbCompletedStr = "Successfully";
const QString LBclient::lbFsformatCompletedStr = "Timed out but maybe because fsformat is slow";


LBclient::LBclient(QObject *parent) : QObject{parent}, lbDevice(new lbModbusClient(this))
{
    lbDevice->setNumberOfRetries(lbNumberOfRetries);
}

LBclient::LBclient(QObject *parent, const QStringList postArg, lbConnection conn) : LBclient(parent)
{
    lbConn = conn;
    setQueryString(postArg);
}

LBclient::LBclient(QObject *parent, const std::initializer_list<QStringList> postArg, lbConnection conn) : LBclient(parent)
{
    lbConn = conn;
    setQueryString(postArg);
}

LBclient::~LBclient()
{
    // qDebug()<<"into ~LBclient()... delete lbDevice, ton";
    delete lbDevice;
    delete ton;
    delete pyaml;
    delete otafile;
}


// void LBclient::setTCPaddr(const QUrl url)
// {
//     lbhost = url.host();
//     lbDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
//     lbDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter,
//                                      (QHostAddress(url.host()).protocol()==QAbstractSocket::IPv6Protocol)?
//                                          (url.host().prepend("[").append("]")):(url.host()));
// }


bool LBclient::setTCPaddr(const QString addr, const int port, const QString iface)
{
    lbhost = addr;
    lbDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
#ifdef Q_OS_LINUX
    QHostAddress qhaddr;
    if (qhaddr.setAddress(addr) && port>0 && port<65536){
        if (qhaddr.protocol()==QAbstractSocket::IPv6Protocol){
            if (qhaddr.scopeId().isEmpty() && !iface.isEmpty())
                qhaddr.setScopeId(iface);
            else
                qhaddr.setScopeId(discover::getlbIfDiscover().value(0).humanReadableName());
        }
        lbDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, qhaddr.toString());
        return true;
    }
#else
    if (QHostAddress(addr).protocol()==QAbstractSocket::IPv6Protocol){
        QString ipv6 = addr;
        lbDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipv6.prepend("[").append("]"));
    }else
        lbDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, addr);
#endif
    return false;
}

void LBclient::setlbHost(const QString host, const QString filename, const QString iface)
{
    pyaml = new lbyaml(filename);
    if (pyaml->getErr() == lbyaml::NoError){
        pyaml->setlbhost(host);
        setTCPaddr(pyaml->getIPv6fromYaml(), 502, iface);
        // QUrl url = QUrl::fromUserInput(pyaml->getIPv6fromYaml());
        // url.setPort(502);
        // setTCPaddr(url);
    }else{
        lbDevice->setDeviceError(lbYamlParsingErrorStr + pyaml->getErr(),lbYamlParsingError);
    }
}

void LBclient::Execute()
{
    if (!lbVectorRequest.isEmpty()){
        lbResponseBuffer.clear();
        lbVectorRequest.clear();
        lbRequestIterator = lbVectorRequest.begin();
        byteCount = initByteCount;
        lenCount = 0;
        i = 0;
    }
    createlbRequest(getJsonStr(queryString));
    if (lbDevice->error() == QModbusDevice::NoError){
        if (lbDevice->state() == QModbusDevice::ConnectedState){
            // lbDevice->setDeviceError("", QModbusDevice::NoError);
            Run();
        }else{
            connect(lbDevice,SIGNAL(stateChanged(QModbusDevice::State)),
                    this, SLOT(isConnected(QModbusDevice::State)));
            lbDevice->connectDevice();
        }
    }else{
        Stoping();
    }
}

void LBclient::setQueryString(const QStringList qstr)
{
    if (!queryString.isEmpty())
        queryString.clear();
    queryString.append(qstr);
    setlbBase(queryString.at(0)[0]);
}

void LBclient::setQueryString(const std::initializer_list<QStringList> qstr_list)
{
    if (!queryString.isEmpty())
        queryString.clear();
    for (auto qstr : qstr_list) {
        queryString.append(qstr);
    }
    setlbBase(queryString.at(0)[0]);
}

void LBclient::setTimeOut(const int t)
{
    if (t>=100 && t<=10000){
        ton = new QTimer(this);
        ton->setSingleShot(true);
        ton->setInterval(t);
        connect(ton, SIGNAL(timeout()),this, SLOT(isTimeout()));
    }else{
        lbDevice->setDeviceError(lbConfErrorTimeoutStr, lbConfError);
    }
}

void LBclient::setOtaFilename(const QString path)
{
    otafile = new QFile(path);
    if (!otafile->open(QIODevice::ReadOnly)){
        lbDevice->setDeviceError(lbConfErrorFilenameStr, lbConfError);
    }
}

void LBclient::setSlot(const QStringList slot)
{
    QList<qsizetype> initList;
    foreach (auto i, slot) {
        bool isNum;
        qsizetype s = i.toInt(&isNum);
        if (isNum)
            initList<<s;
    }
    if (!initList.isEmpty() && initList.at(0)==0){
        lbType.slot.clear();
        // lbDevice->setTimeout((lbKey == KeyOta)?lbOtaSlotTimeout:lbDefaultTimeout);
    }else{
        lbType.slot = initList;
        // lbDevice->setTimeout(lbOtaSlotTimeout);
    }
}

void LBclient::setSlot(const qsizetype slot)
{
    lbType.slot.clear();
    if (slot!=0)
    // {
        lbType.slot<<slot;
        // lbDevice->setTimeout(lbOtaSlotTimeout);
    // }else
    //     lbDevice->setTimeout((lbKey == KeyOta)?lbOtaSlotTimeout:lbDefaultTimeout);
}

QModbusDevice::Error LBclient::getlbDeviceError()
{
    return lbDevice->error();
}

QString LBclient::getlbDeviceMessage()
{
    return lbDevice->errorString();
}

QString LBclient::getlbKey()
{
    return lbKey;
}

void LBclient::isConnected(QModbusDevice::State)
{
    switch (lbDevice->state()) {
    case QModbusDevice::ConnectedState:
        Run();
        break;
    case QModbusDevice::UnconnectedState:
        Stoping();
        break;
    default:
        break;
    }
}

void LBclient::SendRequest()
{
    if (lbDevice->state() == QModbusDevice::ConnectedState){
        // printRequestDiag(*lbRequestIterator);
        lbReply = lbDevice->sendRawRequest(*lbRequestIterator,lbAddr);
        ++lbRequestIterator;
        connect(lbReply,SIGNAL(finished()), this, SLOT(isRequestFinish()));
    }
}

void LBclient::Stoping()
{
    emit lbDisconnect(lbhost, lbDevice->errorString(), lbDevice->error());
}

void LBclient::Run()
{
    // createlbRequest(getJsonStr(queryString.at(0)));
    switch (lbType.alg) {
    case lbota:
        lbDevice->setTimeout((!lbType.slot.isEmpty())?lbOtaSlotTimeout:lbOtaTimeout);
        break;
    case lblog:
        lbDevice->setTimeout((!lbType.slot.isEmpty())?lbLogSlotTimeout:lbLogTimeout);
        break;
    case lbcmd:
        if (lbKey==KeyFboot  || lbKey==KeyFsformat)
            lbDevice->setTimeout(lbFbootTimeout);
        else
            lbDevice->setTimeout((!lbType.slot.isEmpty())?lbOtaSlotTimeout:lbDefaultTimeout);
        break;
    default:
        lbDevice->setTimeout(lbDefaultTimeout);
        break;
    }
    lbRequestIterator = lbVectorRequest.begin();
    SendRequest();
}

QByteArray LBclient::getlbSHA(const QModbusResponse &response)
{
    QByteArray respSHA(response.data().remove(0, 3));
    respSHA.resize(lbModbusClient::lbLenSHA);
    return respSHA;
}

QJsonParseError LBclient::lbParseJson(const QByteArray &lbJsonStr, QJsonObject &paramJsonObj)
{
    QJsonParseError errJson;
    QJsonDocument docRead;
    docRead = docRead.fromJson(lbJsonStr, &errJson);
    if (errJson.error == QJsonParseError::NoError){
        QJsonObject jObj = docRead.object();
        QStringList keys = jObj.keys();
        if (!keys.empty() && keys.size()==1){
            MulpipleRequest = false;
            if (keys.at(0)==KeyGet || keys.at(0)==KeyStats || keys.at(0)==KeySet)
                paramJsonObj = jObj[keys.at(0)].toObject();
            else if (keys.at(0)==KeyGetconf){
                lbParseGetconf(docRead, errJson, jObj, paramJsonObj);
                if (errJson.error == QJsonParseError::NoError)
                    paramJsonObj = docRead.object();
            }
        }else if (!keys.empty() && keys.size()>1) {
            MulpipleRequest = true;
            for (auto k : keys) {
                if (k==KeyGetconf){
                    lbParseGetconf(docRead, errJson, jObj, paramJsonObj);
                    if (errJson.error == QJsonParseError::NoError)
                        paramJsonObj.insert(KeyGetconf, docRead.object());
                }else{
                    paramJsonObj.insert(k, jObj[k]);
                }
            }
        }else{
            paramJsonObj = jObj;
        }
    }
    return errJson;
}

void LBclient::lbParseGetconf(QJsonDocument &qjd, QJsonParseError &qjperr, const QJsonObject &jobj, QJsonObject &pjobj)
{
    if (jobj.value(KeyGetconf).isString()){
        QString confstr = jobj[KeyGetconf].toString();
        if (confstr!=""){
            QByteArray qbacs = confstr.toUtf8();
            qjd = qjd.fromJson(qbacs, &qjperr);
        }
    }
}

QJsonValue LBclient::QueryToJson(QStringList query, QJsonObject (*pf)(QStringList))
{
    // QJsonObject jObj;
    // QJsonObject parjObj;
    query.removeFirst();
    query.sort();
    return pf(query);
    // parjObj.insert(key,jObj);
    // return parjObj;
    // QJsonDocument jDoc(parjObj);
    // return jDoc.toJson(QJsonDocument::Compact);
}

QJsonValue LBclient::QueryToJson(QStringList query)
{
    // QJsonObject jObj;
    query.removeFirst();
    if (query.size()!=0)
        return QJsonValue(query[0]);
    else
        return QJsonValue("1");

    // QJsonDocument jDoc(jObj);
    // return jDoc.toJson(QJsonDocument::Compact);
}

QJsonValue LBclient::FbootToJson()
{
    // QJsonObject jObj;
    // jObj.insert(lbKey, QString::fromUtf8(otafile->readAll()));
    // QJsonDocument jDoc(jObj);
    nw = 0;
    percent = "";
    return QJsonValue(QString::fromUtf8(otafile->readAll()));
}

QJsonObject LBclient::methodGet(QStringList query)
{
    QJsonObject jObj;
    for (int i=0;i<query.size();i++){
        jObj.insert(query[i],"");
    }
    return jObj;
}

QJsonObject LBclient::methodSet(QStringList query)
{
    QJsonObject jObj;
    for (int i=0;i<query.size();i++){
        if (query[i].contains("="))
            jObj.insert(query[i].sliced(0,query[i].indexOf("=")),
                        query[i].sliced(query[i].indexOf("=")+1));
    }
    return jObj;
}


void LBclient::setlbBase(const QString str)
{
    QList<qsizetype> f;
    qsizetype i = 0;
    do {
        i = str.indexOf("/", i+1);
        if (i!=-1)
            f.append(i);
    } while (i!=-1);
    if (f.isEmpty())
        lbKey = str;
    else {
        auto j = f.begin();
        lbKey = str.first(*j);
        do {
            if (j+1==f.end())
                lbType.slot.append(str.sliced(*j+1).toInt());
            else
                lbType.slot.append(str.sliced(*j+1, *(j+1)-*j-1).toInt());
            j++;
        } while (j!=f.end());
    }
    if (lbKey == KeyConf){
        lbType.alg = lbconf;
        // lbDevice->setTimeout(lbDefaultTimeout);
    }
    else if (lbKey == KeyOta){
        // lbDevice->setTimeout((!lbType.slot.isEmpty())?lbOtaSlotTimeout:lbOtaTimeout);
        lbDevice->setEnablePassTimeOut(true);
        lbType.alg = lbota;
    }
    else if (lbKey == KeyLog){
        if (queryString.at(0).size()>1)
            lblogKey = queryString.at(0).at(1);
        if (queryString.at(0).size()>2)
            lblogKey.append(QChar::Null + queryString.at(0).at(2));
        // lbDevice->setTimeout((!lbType.slot.isEmpty())?lbLogSlotTimeout:lbLogTimeout);
        lbType.alg = lblog;
    }
    else if (lbKey == KeyFsformat){
        lbDevice->setEnablePassTimeOut(true);
        lbType.alg = lbcmd;
    }
    else{
        lbType.alg = lbcmd;
        // lbDevice->setTimeout((!lbType.slot.isEmpty())?lbOtaSlotTimeout:lbDefaultTimeout);
    }
}

int LBclient::NowWriteToInt(QString str)
{
    const QString nwa = "now write at ";
    const QString rwo = "received whole image";
    if (str.contains(nwa))
        return str.remove(nwa).toInt();
    if (str.contains(rwo))
        return otafile->size();
    else{
        return -1;
    }
    return 0;
}

QString LBclient::nwToFloatStr(int nw)
{
    return QString::number((float)nw/1000,'f',2);
}

QString LBclient::nwToPercentStr(int nw, qint64 size)
{
    return QString::number(100.0*(float)nw/(float)size,'g',4);
}


void LBclient::insertQuint32(QByteArray &qba, qsizetype pos, qsizetype data)
{
    QByteArray pkt;
    QDataStream pktStream(&(pkt),QIODeviceBase::ReadWrite);
    pktStream<<static_cast<quint32>(data);
    qba.replace(pos,4,pkt);
}


void LBclient::isRequestFinish()
{
    if (lbDevice->state() == QModbusDevice::ConnectedState){
        if (lbReply->error()==QModbusDevice::NoError){
            // printResponseDiag(lbReply->rawResult());
            ++i;
            if (lbVectorRequest.end() != lbRequestIterator){
                if (lbKey==KeyFboot){
                    nw=lbRequestIterator-lbVectorRequest.begin();
                    //percent = QString::number(100.0*(float)nw/(float)(lbVectorRequest.size()-1),'g',4);
                    percent = nwToPercentStr(nw, lbVectorRequest.size()-1);
                    emit ExecuteCompletedStr(QString::number(nw),lbDevice->errorString(), lbDevice->error());
                    emit ExecuteCompleted(lbhost, QStringList{QString::number(nw), percent, lbFbootUnitStr}, lbDevice->errorString(), lbDevice->error());
                    // qDebug()<<lbVectorRequest.size()<<" "<<lbRequestIterator-lbVectorRequest.begin();
                }
                SendRequest();
            }else if (!islbRespondMore(lbReply)){
                switch (lbType.alg) {
                case lbcmd:
                    if (lberr==lbModbusClient::TimeOutError){
                        if (lbConn == NotMaintainTCP)
                            lbDevice->disconnectDevice();
                    }
                    else if (lbResponseSHA != QCryptographicHash::hash(lbResponseBuffer,QCryptographicHash::Sha256)){
                        lbDevice->setDeviceError(lbSHAErrorStr, lbSHAError);
                        lbDevice->disconnectDevice();
                    }else{
                        QJsonParseError errJson = lbParseJson(lbResponseBuffer , lbRequestQjo);
                        if (errJson.error != QJsonParseError::NoError){
                            lbDevice->setDeviceError(lbJsonParseErrorStr + errJson.errorString(), lbJsonParseError);
                            lbDevice->disconnectDevice();
                        }else{
                            if (lbKey==KeyFboot)
                                lbDevice->setDeviceError(lbFbootCompletedStr, QModbusDevice::NoError);
                            else if (lbKey==KeyNofboot)
                                lbDevice->setDeviceError(lbCompletedStr, QModbusDevice::NoError);
                            else{
                                emit ExecuteCompleted(lbhost, getResults(lbRequestQjo), lbDevice->errorString(), lbDevice->error());
                                emit ExecuteCompletedJson(lbhost, lbRequestQjo, lbDevice->errorString(), lbDevice->error());
                            }
                            // printResultsDiag();
                            if (ton!=nullptr)
                                ton->start();
                            else if (lbConn == NotMaintainTCP)
                                lbDevice->disconnectDevice();
                        }
                    }
                    break;
                case lblog:
                case lbota:
                case lbconf:
                    if (lbConn == NotMaintainTCP)
                        lbDevice->disconnectDevice();
                    else{
                        // qDebug()<<"OK";
                        lbDevice->setDeviceError("", QModbusDevice::NoError);
                        emit ExecuteFinished(lbDevice->errorString(), lbDevice->error());
                    }
                    break;
                default:
                    break;
                }
            }
        }
        else if (lbReply->error()!=QModbusDevice::NoError) {
            // qDebug()<<"FAIL >>>>>> "<<lbhost<<lbReply->error()<<lbReply->errorString();
            if (lbConn != NotMaintainTCP){
                lbDevice->setDeviceError("", QModbusDevice::NoError);
                emit ExecuteFinished(lbReply->errorString(), lbReply->error());
            }
            emit ExecuteCompletedStr("",lbReply->errorString(), lbReply->error());
            emit ExecuteCompletedJson(lbhost, lbRequestQjo, lbReply->errorString(), lbReply->error());
            emit ExecuteCompleted(lbhost, QStringList{}, lbReply->errorString(), lbReply->error());
            if (lbConn == NotMaintainTCP)
                lbDevice->disconnectDevice();
        }
    }
}

void LBclient::isTimeout()
{
    lbRequestIterator = lbVectorRequest.begin();
    byteCount = initByteCount;
    lenCount = 0;
    i = 0;
    lbResponseBuffer.clear();
    SendRequest();
}

bool LBclient::getMulpipleRequest() const
{
    return MulpipleRequest;
}

void LBclient::setLbConn(lbConnection newLbConn)
{
    lbConn = newLbConn;
}

void LBclient::createlbRequest(QByteArray lbJsonStr)
{
    QByteArray lbArr(252,(char)0x00);
    qsizetype pos = 0;
    if (!lbType.slot.isEmpty())
        lbWrapSlot(pos, getLenlbArr(lbType.slot.size()*2+1, lbArr, lbJsonStr), lbArr);
    else
        lbArr.replace(pos + 1, 1, QByteArray(1, getLenlbArr(pos, lbArr, lbJsonStr)));
    switch (nextpack.first) {
    case Acmds:
        !lbType.slot.isEmpty()?lbJsonStr.remove(0, MaxLenSlot):lbJsonStr.remove(0, MaxLenCmds);
    case Aconf:
        lbVectorRequest.append(QModbusRequest(lbModbusClient::LogicBox, lbArr));
        lbRequestIterator = lbVectorRequest.begin();
        nextpack.second = start;
        if (neednext)
            createlbRequest(lbJsonStr);
        break;
    case Aupdate:
        lbRequestIterator = lbVectorRequest.insert(++static_cast<QVector<QModbusRequest>::const_iterator>(lbRequestIterator),
                                                   QModbusRequest(lbModbusClient::LogicBox, lbArr));
        nextpack.second = update;
        if (neednext){
            lbJsonStr.remove(0, MaxLenUpdate);
            createlbRequest(lbJsonStr);
        }
        break;
    case Astart:
        lbRequestIterator = lbVectorRequest.insert(static_cast<QVector<QModbusRequest>::const_iterator>(lbRequestIterator),
                                                   QModbusRequest(lbModbusClient::LogicBox, lbArr));
        nextpack.second = update;
        if (neednext){
            lbJsonStr.remove(0, MaxLenStart);
            createlbRequest(lbJsonStr);
        }
        break;
    default:
        lbVectorRequest.append(QModbusRequest(lbModbusClient::LogicBox, lbArr));
        break;
    }
}



qsizetype LBclient::lbWrapSlot(qsizetype pos, qsizetype len, QByteArray &arr, const qsizetype lev)
{
    arr.replace(pos,1,QByteArray(1,slot));
    pos++;
    if (lev==0){
        arr.replace(pos,1,QByteArray(1,len));
        pos++;
    }
    arr.replace(pos,1,QByteArray(1,lbType.slot.at(lev)));
    pos++;
    qsizetype l = lev + 1;
    if (lbType.slot.size()>1 && l<lbType.slot.size())
        lbWrapSlot(pos, len, arr, l);
    return pos;
}

qsizetype LBclient::getLenlbArr(qsizetype pos, QByteArray &arr, const QByteArray& JsonStr)
{
    qsizetype len;
    switch (lbType.alg){
    case lbcmd:
        len = insertTaglbArr(arr, pos, (lbVectorRequest.isEmpty())?cmds:nextpack.second, JsonStr);
        return len;
        break;
    case lbconf:
        len = insertTaglbArr(arr, pos, (lbVectorRequest.isEmpty())?conf:nextpack.second, JsonStr);
        return len;
        break;
    case lbota:
        nw = 0;
        percent = "";
        insertTaglbArr(arr, pos, ota_begin);
        // return (!lbType.slot.isEmpty())?0x06:0x04;
        return lbType.slot.size()*2 + 4;
        break;
    case lblog:
        lblogfd = 0;
        insertTaglbArr(arr, pos, loggrep);
        return (!lbType.slot.isEmpty())?(lblogKey.size() + 3):(lblogKey.size() + 1);
        break;
    default:
        break;
    }
    return 0;
}

qsizetype LBclient::insertTaglbArr(QByteArray& arr, qsizetype pos, Tag tag, const QByteArray &JsonStr)
{
    arr.replace(pos,1,QByteArray(1,tag));
    (!lbType.slot.isEmpty())?pos++:pos+=2;
    switch (tag){
    case cmds:
        nextpack.first = Acmds;
        arr.replace(pos,lbModbusClient::lbLenSHA,QCryptographicHash::hash(JsonStr,QCryptographicHash::Sha256));
        if (JsonStr.size()<=MaxLenCmds){
            arr.replace(pos+32,JsonStr.size(),JsonStr);
            return (!lbType.slot.isEmpty())?JsonStr.size() + lbModbusClient::lbLenSHA + 2*lbType.slot.size():
                       JsonStr.size() + lbModbusClient::lbLenSHA;
        }else{
            neednext = true;
            arr.replace(pos+32,MaxLenCmds,JsonStr.sliced(0,MaxLenCmds));
            return MaxLenUpdate;
        }
        break;
    case start:
        nextpack.first = Astart;
        if (JsonStr.size()<=MaxLenStart){
            neednext = false;
            arr.replace(pos+3,1,QByteArray(1,MaxLenUpdate));
            arr.replace(pos+4,JsonStr.size(),JsonStr);
            return JsonStr.size() + ((lbType.alg==lbconf)?0:4);
        }else{
            neednext = true;
            insertQuint32(arr, pos, JsonStr.size() + ((lbType.alg==lbconf)?0:4));
            arr.replace(pos+4,MaxLenStart,JsonStr.first(MaxLenStart));
            return MaxLenUpdate;
        }
        break;
    case update:
        nextpack.first = Aupdate;
        if (JsonStr.size()<=MaxLenUpdate){
            neednext = false;
            arr.replace(pos,JsonStr.size(),JsonStr);
            return JsonStr.size();
        }else{
            arr.replace(pos,MaxLenUpdate,JsonStr.first(MaxLenUpdate));
            return MaxLenUpdate;
        }
        break;
    case conf:
        nextpack.first = Aconf;
        neednext = true;
        arr.replace(pos,lbModbusClient::lbLenSHA,QCryptographicHash::hash(JsonStr,QCryptographicHash::Sha256));
        return lbModbusClient::lbLenSHA;
        break;
    case confstat:
        break;
    case more:
        insertQuint32(arr, pos, byteCount += 0xF9);
        break;
    case ota_begin:
        insertQuint32(arr, pos, otafile->size());
        break;
    case ota_write:
        insertQuint32(arr, pos, otafile->pos());
        pos+=4;
        arr.replace(pos,byteRemained,otafile->read(byteRemained));
        break;
    case ota_end:
        break;
    case loggrep:
        arr.replace(pos,1, QByteArray(1,xlblogTimeout));
        pos++;
        arr.replace(pos,lblogKey.size(), lblogKey.toUtf8());
        break;
    case logmore:
        arr.replace(pos,1,QByteArray(1,lblogfd));
        break;
    default:
        break;
    }
    return 0;
}

void LBclient::SendAfterRequest(Tag tag)
{
    QByteArray lbArr(252,(char)0x00);
    qsizetype pos = 0;
    if (!lbType.slot.isEmpty())
        lbWrapSlot(pos, getLenlbArrAfter(lbType.slot.size()*2+1, lbArr, tag), lbArr);
    else
        lbArr.replace(pos + 1, 1, QByteArray(1, getLenlbArrAfter(pos, lbArr, tag)));
    lbReply = lbDevice->sendRawRequest(QModbusRequest(lbModbusClient::LogicBox, lbArr),lbAddr);
    connect(lbReply,SIGNAL(finished()), this, SLOT(isRequestFinish()));
}

qsizetype LBclient::getLenlbArrAfter(qsizetype pos, QByteArray &arr, Tag tag)
{
    qsizetype len = 0;
    switch (lbType.alg){
    case lbcmd:
    case lbconf:
        len = 0x04;
        break;
    case lbota:
        if (tag==ota_end){
            len = (!lbType.slot.isEmpty())?2*lbType.slot.size():0x0;
            break;
        }
        if (!lbType.slot.isEmpty()){
            if (otafile->bytesAvailable()>MaxLenStart - 2*lbType.slot.size())
                // byteRemained = MaxLenOtaWriteSlot;
                byteRemained = MaxLenStart - 2*lbType.slot.size();
            else
                byteRemained = otafile->bytesAvailable();
            len = byteRemained + 0x4 + 2*lbType.slot.size();
        }else{
            if (otafile->bytesAvailable()>MaxLenStart)
                byteRemained = MaxLenStart;
            else
                byteRemained = otafile->bytesAvailable();
            len = byteRemained + 0x4;
        }
        break;
    case lblog:
        len = (!lbType.slot.isEmpty())?0x3:0x1;
        break;
    default:
        break;
    }
    insertTaglbArr(arr, pos, tag);
    return len;
}

QJsonValue LBclient::getJsonValue(QStringList query, const QString lbKey)
{
    if (lbKey==KeyGet && query.size()>1)
        return QueryToJson(query, methodGet);
    else if (lbKey==KeySet && query.size()>1)
        return QueryToJson(query, methodSet);
    else if (lbKey==KeyForse && query.size()>1)
        return QueryToJson(query, methodSet);
    else if (lbKey==KeyUnforse && query.size()>1)
        return QueryToJson(query, methodGet);
    else if (lbKey==KeyRestart)
        return QueryToJson(query);
    else if (lbKey==KeySettime && query.size()>1)
        return QueryToJson(query);
    else if (lbKey==KeyStats)
        return QueryToJson(query);
    else if (lbKey==KeyGetconf)
        return QueryToJson(query);
    else if (lbKey==KeyFboot)
        return FbootToJson();
    else if (lbKey==KeyNofboot)
        return QueryToJson(query);
    else if (lbKey==KeyFsformat)
        return QueryToJson(query);
    else{
        lbDevice->setDeviceError(lbKeyNotfoundErrorStr, lbKeyNotfoundError);
        // lbDevice->disconnectDevice();
    }
    return QJsonObject();
}

// QJsonObject LBclient::getJsonObj(QStringList query, const QString lbKey)
// {
//     QJsonObject jObj;
//     QJsonObject parjObj;
//     if (lbKey==KeyGet && query.size()>1)
//         return QueryToJson(query, KeyGet, methodGet);
//     else if (lbKey==KeySet && query.size()>1)
//         return QueryToJson(query, KeySet, methodSet);
//     else if (query[0]==KeyForse && query.size()>1)
//         return QueryToJson(query, KeyForse, methodSet);
//     else if (query[0]==KeyUnforse && query.size()>1)
//         return QueryToJson(query, KeyUnforse, methodGet);
//     else if (query[0].size()>=KeyRestart.size() && query[0].first(7)==KeyRestart && query.size()>0)
//         return QueryToJson(query, KeyRestart);
//     else if (query[0]==KeySettime && query.size()>1)
//         return QueryToJson(query, KeySettime);
//     else if (query[0]==KeyStats && query.size()>0)
//         return QueryToJson(query, KeyStats);
//     else if (query[0]==KeyGetconf && query.size()>0)
//         return QueryToJson(query, KeyGetconf);
//     else if (query[0]==KeyConf && query.size()>0)
//         return pyaml->getlbJson();
//     else if (lbKey==KeyFboot){
//         nw = 0;
//         percent = "";
//         return FbootToJson();
//     }
//     else if (lbKey==KeyNofboot)
//         return QueryToJson(query, KeyNofboot);
//     else if (lbKey==KeyOta || lbKey==KeyLog)
//         return QJsonObject();
//     else if (lbKey==KeyFsformat)
//         return QueryToJson(query, KeyFsformat);
//     else{
//         lbDevice->setDeviceError(lbKeyNotfoundErrorStr, lbKeyNotfoundError);
//         // lbDevice->disconnectDevice();
//     }
//     return QJsonObject();
// }

QByteArray LBclient::getJsonStr(QVector<QStringList> query)
{
    if (!query.isEmpty()){
        if (lbType.alg==lbcmd){
            QJsonObject jObj;
            for (auto q=query.begin(); q!=query.end(); ++q) {
                if (q==query.begin())
                    jObj.insert(lbKey, getJsonValue(*q, lbKey));
                else if (lbKey!=KeyFsformat && (*q).at(0)!=KeyFsformat) //todo....
                    jObj.insert((*q).at(0), getJsonValue(*q, (*q).at(0)));
            }
            // qDebug()<<"getJsonStr>>"<<jObj;
            QJsonDocument jDoc(jObj);
            return jDoc.toJson(QJsonDocument::Compact);
        }
        else if (lbKey==KeyConf)
            return pyaml->getlbJson();
        else if (lbKey==KeyOta || lbKey==KeyLog)
            return 0;
    }
    return 0;
}

bool LBclient::islbRespondMore(const QModbusReply *reply)
{
    quint8 tag, len;
    tag = reply->result().values()[0];
    len = reply->result().values()[1];
    lberr = reply->result().values()[2];
    QString devmsg = lbDevice->getlbError(reply->rawResult(),len);
    lenCount += len;
    switch (tag) {
    case start:
    case update:
        break;
    case cmds:
        if (lbKey==KeyFsformat){
            lbDevice->setDeviceError(lbFsformatCompletedStr, QModbusDevice::NoError);
            lbDevice->setEnablePassTimeOut(false);
            return false;
        }
        lbResponseSHA = getlbSHA(lbReply->rawResult());
        if (len<=MaxLenUpdate){
            lbResponseBuffer = lbReply->rawResult().data().sliced(3 + lbModbusClient::lbLenSHA, len - lbModbusClient::lbLenSHA - 1);
            return false;
        }else{
            lbResponseBuffer += lbReply->rawResult().data().remove(0,3 + lbModbusClient::lbLenSHA);
            SendAfterRequest(more);
            return true;
        }
        break;
    case more:
        if (len<MaxLenUpdate){
            lbResponseBuffer += reply->rawResult().data().sliced(3,len-1);
            return false;
        }else{
            lbResponseBuffer += reply->rawResult().data().remove(0,3);
            SendAfterRequest(more);
            return true;
        }
        break;
    case conf:
        SendAfterRequest(confstat);
        return true;
    case confstat:
        emit ExecuteCompletedStr(devmsg,lbDevice->errorString(), lbDevice->error());
        emit ExecuteCompletedJson(lbhost, lbRequestQjo, lbDevice->errorString(), lbDevice->error());
        emit ExecuteCompleted(lbhost, QStringList{devmsg}, lbDevice->errorString(), lbDevice->error());
        if (devmsg == "pending"){
            SendAfterRequest(confstat);
            return true;
        }
        break;
    case ota_begin:
    {
        nw = NowWriteToInt(devmsg);
        if (nw!=-1){
            //percent = QString::number(100.0*(float)nw/(float)otafile->size(),'g',4);
            percent = nwToPercentStr(nw, otafile->size());
            emit ExecuteCompletedStr(nwToFloatStr(nw),lbDevice->errorString(), lbDevice->error());
            // emit ExecuteCompletedJson(lbhost, lbRequestQjo, lbDevice->errorString(), lbDevice->error());
            emit ExecuteCompleted(lbhost, QStringList{nwToFloatStr(nw), percent, lbOtaUnitStr}, lbDevice->errorString(), lbDevice->error());
            SendAfterRequest(ota_write);
            return true;
        }else
            return false;
    }
        break;
    case ota_write:
    {
        if (lberr==lbModbusClient::TimeOutError){
            emit ExecuteCompletedStr(nwToFloatStr(nw),lbOtaWarningStr+QString::number(lbDevice->OtaTimeOutCount), lbDevice->error());
            emit ExecuteCompleted(lbhost, QStringList{nwToFloatStr(nw), percent, lbOtaUnitStr},QString::number(lbDevice->OtaTimeOutCount)+lbOtaWarningStr, lbDevice->error());
            otafile->seek(nw);
            SendAfterRequest(ota_write);
            return true;
        }else{
        nw = NowWriteToInt(devmsg);
        if (nw!=-1){
            //percent = QString::number(100.0*(float)nw/(float)otafile->size(),'g',4);
            percent = nwToPercentStr(nw, otafile->size());
            emit ExecuteCompletedStr(nwToFloatStr(nw),lbDevice->errorString(), lbDevice->error());
            emit ExecuteCompleted(lbhost, QStringList{nwToFloatStr(nw), percent, lbOtaUnitStr}, lbDevice->errorString(), lbDevice->error());
            if (nw >= otafile->size())
                SendAfterRequest(ota_end);
            else{
                if (otafile->pos()!=nw)
                    otafile->seek(nw);
                SendAfterRequest(ota_write);
            }
            return true;
        }else
            return false;
        }
    }
        break;
    case ota_end:
        lbDevice->setDeviceError(lbOtaCompletedStr, QModbusDevice::NoError);
        lbDevice->setEnablePassTimeOut(false);
        break;
    case slot:
        if (lberr==lbModbusClient::TimeOutError && lbType.alg == lbota){
            emit ExecuteCompletedStr(nwToFloatStr(nw),QString::number(lbDevice->OtaTimeOutCount)+lbOtaWarningSlotStr, lbDevice->error());
            emit ExecuteCompleted(lbhost, QStringList{nwToFloatStr(nw), percent, lbOtaUnitStr},QString::number(lbDevice->OtaTimeOutCount)+lbOtaWarningSlotStr, lbDevice->error());
            otafile->seek(nw);
            SendAfterRequest(ota_write);
            return true;
        }
        break;
    case loggrep:
    case logmore:
        if (devmsg.size()==1 && devmsg.at(0)==char(lblogfd))
            return false;
        else if (lberr==lbModbusClient::LogEagain){
            SendAfterRequest(logmore);
            return true;
        }else{
            QString logstr = devmsg.removeFirst().removeLast().removeLast().trimmed();
            emit ExecuteCompletedStr(logstr,lbDevice->errorString(), lbDevice->error());
            emit ExecuteCompleted(lbhost, QStringList{logstr},lbDevice->errorString(), lbDevice->error());
            lblogfd = reply->rawResult().data().at(3);
            SendAfterRequest(logmore);
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void LBclient::printResponseDiag(const QModbusResponse localMbResponse)
{
    qDebug()<<lbDevice;
    qDebug()<<"<<<<<<<<<< "<<i<<" <<<<<<<<<<";
    qDebug().noquote()<<"localMbResponse.data().toHex() = "<<localMbResponse.data().toHex();
    qDebug()<<"lbDevice->error()<<lbDevice->errorString() = "<<lbDevice->error()<<lbDevice->errorString();
    //qDebug().noquote()<<"localMbResponse.functionCode() = "<<localMbResponse.functionCode();
    //qDebug().noquote()<<"localMbResponse.exceptionCode() = "<<localMbResponse.exceptionCode();
    //qDebug()<<"localMbResponse.ExceptionByte = "<<localMbResponse.ExceptionByte;
    //qDebug()<<"type = "<<lbReply->type();
    //qDebug()<<"rawResult().isException() = "<<lbReply->rawResult().isException();
    //qDebug()<<"rawResult().isValid() = "<<lbReply->rawResult().isValid();
}

void LBclient::printRequestDiag(const QModbusRequest localMbRequest)
{
    qDebug()<<">>>>>>>>> "<<i<<" >>>>>>>>>";
    qDebug().noquote()<<i<<"     lbRequest is : "<<localMbRequest.data();
    qDebug().noquote()<<i<<" HEX lbRequest is : "<<localMbRequest.data().toHex();
    //qDebug()<<i<<" sendRawRequest....";
}

void LBclient::printResultsDiag()
{
    //qDebug().noquote()<<"Response  SHA256 is: "<<lbResponseSHA.toHex();
    //qDebug().noquote()<<"Calculate SHA256 is: "<<
    //    QCryptographicHash::hash(lbResponseBuffer,QCryptographicHash::Sha256).toHex();
    //qDebug().noquote()<<"byteCount is: "<<byteCount;
    //qDebug().noquote()<<"lenCount is: "<<lenCount;
    // qDebug().noquote()<<lbResponseBuffer;
    //qDebug().noquote()<<lbResponseBuffer.toHex();
    //qDebug()<<"lbDevice->error()<<lbDevice->errorString() = "<<lbDevice->error()<<lbDevice->errorString();
    qDebug().noquote()<<"printResultsDiag<<"<<lbRequestQjo;
}

QStringList LBclient::getResults(const QJsonObject &Qjo)
{
    QStringList qdl;
    QString lbstr;
    // emit ExecuteCompletedJson(lbhost, Qjo, lbDevice->errorString(), lbDevice->error());
    if (!Qjo.isEmpty()){
        if (queryString.at(0).size()>1){
            for (int i=1;i<queryString.at(0).size();i++){
                QJsonValue qjv = Qjo.value(queryString.at(0).at(i));
                if (qjv.isDouble()){
                    qdl.append(QString::number(qjv.toDouble()));
                    lbstr.append(queryString.at(0).at(i) + "=" + QString::number(qjv.toDouble()) + " ");
                }else if (qjv.isString()){
                    qdl.append(qjv.toString());
                    lbstr.append(queryString.at(0).at(i) + "=" + qjv.toString() + " ");
                } 
            }
        }else if (queryString.at(0).at(0) == KeyGetconf){}
        else{
            QStringList keys = Qjo.keys();
            for (int i=0;i<Qjo.size();i++){
                QJsonValue qjv = Qjo.value(keys.at(i));
                if (qjv.isDouble()){
                    qdl.append(QString::number(qjv.toDouble()));
                    lbstr.append(keys.at(i) + "=" + QString::number(qjv.toDouble()) + " ");
                }else if (qjv.isString()){
                    qdl.append(qjv.toString());
                    lbstr.append(keys.at(i) + "=" + qjv.toString() + " ");
                }
            }
        }
        emit ExecuteCompletedStr(lbstr.trimmed(),lbDevice->errorString(), lbDevice->error());
    }
    return qdl;
}
