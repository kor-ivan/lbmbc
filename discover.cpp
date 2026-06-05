#include "discover.h"


discover::discover(QObject *parent)
    : QObject{parent}
{
}


discover::~discover()
{
    // foreach (auto i, scanner)
    //     delete i;
    // foreach (auto i, lbcMap) {
    //     delete i;
    // }
}


void discover::execute(int ifindex) {
    QList<QNetworkInterface> qnif;
    if (ifindex == 0){
        qnif = getlbIfDiscover();
        //printlbInterface();
    }else if (QNetworkInterface::interfaceFromIndex(ifindex).isValid()){
        qnif.append(QNetworkInterface::interfaceFromIndex(ifindex));
    }else{
        emit discoverCompleted(lbDiscoverMap, SomeError, "Interface is not Valid");
        this->deleteLater();
        return;
    }
    scanner = QList<Icmp6Scanner*>(qnif.size());
    for (int i=0;i<qnif.size();++i){
        scanner[i] = new Icmp6Scanner();
        scanner[i]->setTimeoutMs(waitingTime);
        connect(scanner.at(i), &Icmp6Scanner::replyReceived, this, &discover::isResponseReceived);
        connect(scanner.at(i), &Icmp6Scanner::scanFinished, this, &discover::isIfscanIsFinish);
        scanner.at(i)->sendMulticastRequest(qnif.at(i), QHostAddress(lbMulticastAddr));
        ScannCount++;
    }
}

QMap<QString, discover::lbinfo> discover::getlbDiscoverMap()
{
    return lbDiscoverMap;
}

int discover::getWaitingTime() const
{
    return waitingTime;
}

void discover::setWaitingTime(int newWaitingTime)
{
    waitingTime = newWaitingTime;
}

QList<QNetworkInterface> discover::getlbIfDiscover()
{
    QList<QNetworkInterface> qlnet = QNetworkInterface::allInterfaces();
    qlnet.removeIf([](QNetworkInterface qif){
        auto a {[](QNetworkInterface i){
            foreach (QNetworkAddressEntry h, i.addressEntries()) {
                if (h.ip().protocol()==QAbstractSocket::IPv6Protocol)
                    return false;
            }
            return true;
        }};
        return !(bool)(qif.flags()&QNetworkInterface::IsUp) || (bool)(qif.flags()&QNetworkInterface::IsLoopBack) || a(qif);
    });
    return qlnet;
}

void discover::getResults()
{
    if (ScannCount == 0 && lbcCount == 0){
        // qDebug()<<lbDiscoverMap;
        // foreach (const auto i, lbDiscoverMap) {
        //     qDebug()<<i;
        // }
        emit discoverCompleted(lbDiscoverMap, NoError, NoErrorStr);
        this->deleteLater();
    }
}

float discover::delaytofloat(int d)
{
    return ((float)d)/1000;
}

QString discover::addColonsToMac(const QString &mac)
{
    if (mac.length() != 12) {
        return mac;
    }
    QString result;
    for (int i = 0; i < mac.length(); ++i) {
        if (i > 0 && i % 2 == 0) {
            result += ':';
        }
        result += mac[i];
    }
    return result;
}

void discover::isIfscanIsFinish(const QString &name)
{
    // qDebug() << ScannCount << "Сканирование завершено на интерфейсе" << name;
    sender()->deleteLater();
    ScannCount--;
    // if (ScannCount == 0)
    //     qDebug()<<"Timeout icmpv6Scanner";
    getResults();
}

void discover::isResponseReceived(const QString &from, int rttmcs, int ifindex)
{
    emit icmpResponseReceived(from, rttmcs, ifindex);
    if (!lbDiscoverMap.contains(from)){
        lbinfo inf;
        inf.delay.append(delaytofloat(rttmcs));
        inf.ifindex.append(ifindex);
        lbDiscoverMap.insert(from, inf);
        auto lbMapIterator = lbcMap.insert(from,
            new LBclient(this, {{"get", "sys.ipaddr", "sys.devtype", "sys.macaddr"},{"getconf"}},LBclient::MaintainTCP));
        connect(lbMapIterator.value(), &LBclient::ExecuteCompletedJson, this, &discover::isExecuteCompletedJson);
        connect(lbMapIterator.value(), &LBclient::lbDisconnect, this, &discover::islbHostDisconnect);
        // QUrl url = QUrl::fromUserInput(from);
        // url.setPort(502);
        lbMapIterator.value()->setTCPaddr(from, 502);
        lbMapIterator.value()->Execute();
        lbcCount++;
        lbFinishMap.insert(from, false);
    }else{
        lbDiscoverMap[from].delay.append(delaytofloat(rttmcs));
        lbDiscoverMap[from].ifindex.append(ifindex);
    }
}

void discover::isExecuteCompleted(const QString &lbhost, const QStringList &result, const QString &message, const QModbusDevice::Error error)
{

    if (error == QModbusDevice::NoError){
        if (result.value(0).toDouble()!=-1 && result.value(0).toDouble()<60){
            lbinfo inf = lbDiscoverMap.value(lbhost);
            inf.btn = true;
            lbDiscoverMap.insert(lbhost, inf);
        }
    }
    // qDebug() << lbcCount << "Запросы Modbus (107) завершены по адресу" << lbhost;
    sender()->deleteLater();
    lbcCount--;
    lbFinishMap.insert(lbhost, true);
    getResults();
}

void discover::isExecuteCompletedJson(const QString &lbhost, const QJsonObject &Qjo, const QString &message, const QModbusDevice::Error error)
{
    lbinfo inf = lbDiscoverMap.value(lbhost);
    if (error == QModbusDevice::NoError){
        inf.ipv4 = Qjo.value("get").toObject().value("sys.ipaddr").toString();
        inf.type = Qjo.value("get").toObject().value("sys.devtype").toString();
        inf.mac = addColonsToMac(Qjo.value("get").toObject().value("sys.macaddr").toString());
        if (Qjo.value("getconf").toObject().find("hostname")->type()==QJsonValue::String)
            inf.name = Qjo.value("getconf").toObject().find("hostname")->toString();
        lbDiscoverMap.insert(lbhost, inf);
        // qDebug() << lbcCount << "Запросы Modbus (107) завершены по адресу" << lbhost;
    }
    // qDebug() << lbcCount << "Запросы Modbus JSON по адресу" << lbhost;
    sender()->disconnect(lbcMap.value(lbhost), &LBclient::ExecuteCompletedJson, this, &discover::isExecuteCompletedJson);
    connect(lbcMap.value(lbhost), &LBclient::ExecuteCompleted, this, &discover::isExecuteCompleted);
    lbcMap.value(lbhost)->setQueryString({"get", "sys.btnage"});
    lbcMap.value(lbhost)->Execute();
}


void discover::islbHostDisconnect(const QString &lbhost, const QString &message, const QModbusDevice::Error error)
{
    if (!lbFinishMap.value(lbhost)){
        lbcCount--;
        getResults();
        qDebug()<<"lbHostDisconnect is"<<lbhost;
    }
}

QDebug operator<<(QDebug out, const discover::lbinfo& inf){
    if (inf.btn)
        out.noquote()<<"*";
    out.noquote()<<inf.name<<" "<<inf.type<<" "<<inf.mac<<" "<<inf.ipv4;
    foreach (const auto i, inf.ifindex) {
        out<<"IF ="<<i;
    }
    foreach (const auto i, inf.delay) {
        out<<"delay ="<<i<<"ms";
    }
    return out;
}

QString discover::lbinfo::toString() const
{
    QString str;
    if (btn) str.append("*");
    str += QString("%1 %2 %3 %4 ").arg(name).arg(type).arg(mac).arg(ipv4);
    QStringList ifindexStr;
    for (int i : ifindex) ifindexStr << "IF =" << QString::number(i);
    str += ifindexStr.join(" ") + " ";
    QStringList delayStr;
    for (float d : delay) delayStr<<"delay ="<<QString::number(d)<<"ms";
    str += delayStr.join(" ");
    return str;
}
