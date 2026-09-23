#include "lbprocess.h"
#include "discover.h"
#include <QDir>

lbprocess::lbprocess(QObject *parent, LBclient *lbc, Strategy strat)
    : QObject{parent}, plbc(lbc), m_strategy(strat)
{
    plbc->setLbConn(LBclient::MaintainTCP);
}

void lbprocess::run(processMode m, const QStringList var)
{
    if (PreOtaSlotNotValid){
        emit outMessage("STOP preOtaSlot array not valid", "", QModbusDevice::NoError);
        plbc->deleteLater();
        return;
    }
    mode = m;
    scanVar = var;
    plbc->setQueryString({"set", "sys.bustab=1"});
    connect(plbc, &LBclient::ExecuteCompleted, this, &lbprocess::localExeCompl);
    phase = setBustab;
    plbc->Execute();
}

void lbprocess::setOtaPath(const QString &newOtaPath)
{
    if (!newOtaPath.isEmpty())
        otaPath = newOtaPath + QDir::separator();
}

void lbprocess::SendOneMessage(const QString &mess)
{
    connect(plbc, &LBclient::ExecuteCompletedStr, this, &lbprocess::outMessage);
    emit outMessage(mess, "", QModbusDevice::NoError);
    disconnect(plbc, &LBclient::ExecuteCompletedStr, this, &lbprocess::outMessage);

}

bool lbprocess::preparationOta()
{
    if (lbscanMap.value(*ri_lbota).devtype!="unknown"){
        // qDebug()<<otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin";
        QString binPath = otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin";
        QString xzPath  = otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin.xz";
        QString finalFile;

        if (m_strategy == onlyBin) {
            finalFile = binPath;
        } else if (m_strategy == onlyXZ) {
            finalFile = xzPath;
        } else if (m_strategy == firstBin) {
            finalFile = QFile::exists(binPath) ? binPath : xzPath;
        } else if (m_strategy == firstXZ) {
            finalFile = QFile::exists(xzPath) ? xzPath : binPath;
        }
        if (!finalFile.isEmpty()) {
            plbc->setOtaFilename(finalFile);
        }

        if (plbc->getlbDeviceError()==QModbusDevice::NoError){
            SendOneMessage("Now ota slot " + QString::number(*ri_lbota) + " from " + finalFile);
            plbc->setQueryString({"ota"}); //todo here setEnablePassTimeout
            if (*ri_lbota!=-1)
                plbc->setSlot(*ri_lbota);
            else
                plbc->setSlot(0);
            return true;
        }
        return false;
    }else{
        SendOneMessage("Ota slot " + QString::number(*ri_lbota) + " fail");
    }
    return false;
}

void lbprocess::preparationRestart()
{
    if (*ri_lbota!=-1)
        plbc->setSlot(*ri_lbota);
    else
        plbc->setSlot(0);
    SendOneMessage("Now reboot slot " + QString::number(*ri_lbota));
}

void lbprocess::preparationOtaCompl()
{
    plbc->setOtaFilename(QString());
    SendOneMessage("End autoota and start reboot");
    phase = reboot;
    disconnect(plbc, &LBclient::ExecuteCompleted, this, &lbprocess::processOta);
    disconnect(plbc, &LBclient::ExecuteFinished, this, &lbprocess::localFinish);
    ri_lbota = lbotaKeys.rbegin();
    plbc->setQueryString({"restart"});
    plbc->setSlot(*ri_lbota);
    SendOneMessage("Now reboot slot " + QString::number(*ri_lbota));
    plbc->Execute();
}

bool lbprocess::nextOtaSlot()
{
    ri_lbota++;
    if (ri_lbota!=lbotaKeys.rend()){
        if (preparationOta())
            return true;
        else
            return nextOtaSlot();
    }
    return false;
}

QList<qsizetype> lbprocess::checkOtaKeys(const QList<qsizetype> &otaKeys, const QList<qsizetype> &OtherOtaKeys)
{
    if (!OtherOtaKeys.isEmpty()){
        QSet<qsizetype> otaKeysSet(otaKeys.begin(), otaKeys.end());
        bool containsAll = true;
        for(qsizetype key : OtherOtaKeys){
            if (!otaKeysSet.contains(key)){
                containsAll = false;
                break;
            }
        }
        if (containsAll){
            return OtherOtaKeys;
        }else{
            SendOneMessage("END preOtaSlot array not valid");
            plbc->deleteLater();
            return QList<qsizetype>();
        }
    }
    return otaKeys;
}

void lbprocess::setNumOfVarRetries(int newNumOfVarRetries)
{
    numOfVarRetries = newNumOfVarRetries;
}

void lbprocess::setPreOtaSlot(const QStringList &otaslots)
{
    if(!preOtaKeys.isEmpty()) preOtaKeys.clear();
    preOtaKeys.reserve(otaslots.size());
    for (const QString &str : otaslots) {
        bool ok;
        qsizetype value = static_cast<qsizetype>(str.toLongLong(&ok));
        if (ok) {
            preOtaKeys.append(value);
        } else {
            PreOtaSlotNotValid = true;
            emit outMessage("preOtaSlot array not valid", "", QModbusDevice::NoError);
            return;
        }
    }
    PreOtaSlotNotValid = false;
}

void lbprocess::processOta(const QString &lbhost, const QStringList &result, const QString &message, const QModbusDevice::Error error)
{
    emit outOta(lbhost, result, message, error);
}

void lbprocess::processMessage(const QString &lbstr, const QString &message, const QModbusDevice::Error error)
{
    emit outMessage(lbstr, message, error);
}

void lbprocess::localMessage(const QString &lbstr, const QString &message, const QModbusDevice::Error error)
{
    if(error==QModbusDevice::NoError){
        // qDebug()<<lbstr;
        scaninfo inf;
        switch (phase) {
        case logBustab:
        {
            qsizetype index = lbstr.indexOf(":", lbstr.indexOf("bustab_dump1:")+13);
            if (index!=-1)
                inf.mac = lbstr.sliced(index + 2, 12);
            index = lbstr.indexOf("rank=");
            if (index!=-1){
                QString s = lbstr.sliced(index + 5);
                index = s.indexOf(" ");
                if (index!=-1){
                    if (s.contains("BM"))
                        inf.master = true;
                    s = s.first(index);
                }
                bool ok;
                int rank = s.toInt(&ok);
                if (ok && !lbscanMap.contains(rank))
                    lbscanMap.insert(rank, inf);
            }
        }
            break;
        default:
            break;
        }
    }else
        SendOneMessage(message);
}

void lbprocess::localExeCompl(const QString &lbhost, const QStringList &result, const QString &message, const QModbusDevice::Error error)
{
    // qDebug()<<message<<error;
    scaninfo inf;
    switch (phase) {
    case setBustab:
        if (error == QModbusDevice::NoError){
            disconnect(plbc, &LBclient::ExecuteCompleted, this, &lbprocess::localExeCompl);
            plbc->setQueryString({"log", "100r", "bustab_dump"});
            connect(plbc, &LBclient::ExecuteCompletedStr, this, &lbprocess::localMessage);
            connect(plbc, &LBclient::ExecuteFinished, this, &lbprocess::localFinish);
            phase = logBustab;
            plbc->Execute();
        }else
            plbc->deleteLater();
        break;
    case getSysvar:
        if (error != QModbusDevice::NoError && cRetries < numOfVarRetries){
            cRetries++;
            SendOneMessage("trying the slot " + QString::number(i_lbscanMap.key()) + " ... " + QString::number(cRetries));
            plbc->Execute();
        }else{
            if (error == QModbusDevice::NoError){
                inf = i_lbscanMap.value();
                inf.devtype = result.at(0);
                inf.version = result.at(1);
                for (int i=2;i<scanVar.size()+2;++i)
                    inf.data << result.at(i);
                lbscanMap.insert(i_lbscanMap.key(), inf);
            }
            i_lbscanMap++;
            if (i_lbscanMap!=lbscanMap.end()){
                plbc->setSlot(i_lbscanMap.key());
                plbc->Execute();
            }else{
                emit scanCompleted(lbscanMap);
                switch (mode) {
                case scan:
                    plbc->deleteLater();
                    break;
                case autoota:
                    lbotaKeys = checkOtaKeys(lbscanMap.keys(), preOtaKeys);
                    if (lbotaKeys.isEmpty()) break;
                    ri_lbota = lbotaKeys.rbegin();
                    phase = ota;
                    connect(plbc, &LBclient::ExecuteCompleted, this, &lbprocess::processOta);
                    connect(plbc, &LBclient::ExecuteFinished, this, &lbprocess::localFinish);
                    if (preparationOta()){
                        plbc->Execute();
                    }else{
                        if (nextOtaSlot()){
                            plbc->Execute();
                        }else{
                            preparationOtaCompl();
                        }
                    }
                    break;
                case restartall:
                    SendOneMessage("Start reboot all...");
                    phase = reboot;
                    lbotaKeys = checkOtaKeys(lbscanMap.keys(), preOtaKeys);
                    if (lbotaKeys.isEmpty()) break;
                    ri_lbota = lbotaKeys.rbegin();
                    plbc->setQueryString({"restart"});
                    preparationRestart();
                    plbc->Execute();
                    break;
                default:
                    break;
                }
            }
        }
        break;
    case reboot:
        ri_lbota++;
        if (ri_lbota!=lbotaKeys.rend()){
            preparationRestart();
            plbc->Execute();
        }else{
            SendOneMessage("END");
            plbc->deleteLater();
        }
        break;
    default:
        break;
    }
}


void lbprocess::localFinish(const QString &message, const QModbusDevice::Error error)
{
        // qDebug()<<lbscanMap;
        switch (phase) {
        case logBustab:
            if (error == QModbusDevice::NoError){
                i_lbscanMap = lbscanMap.begin();
                QStringList ss = {"get", "sys.devtype", "sys.version"};
                if (!scanVar.empty()){
                    for (const auto &i : scanVar)
                        ss<<i;
                }
                plbc->setQueryString(ss);
                if (i_lbscanMap.key()!=-1)
                    plbc->setSlot(i_lbscanMap.key());
                phase = getSysvar;
                disconnect(plbc, &LBclient::ExecuteCompletedStr, this, &lbprocess::localMessage);
                disconnect(plbc, &LBclient::ExecuteFinished, this, &lbprocess::localFinish);
                connect(plbc, &LBclient::ExecuteCompleted, this, &lbprocess::localExeCompl);
                plbc->Execute();
            }
            break;
        case ota:
            if (error != QModbusDevice::NoError){
                SendOneMessage("Ota slot " + QString::number(*ri_lbota) + " fail");
            }
            ri_lbota++;
            if (ri_lbota!=lbotaKeys.rend()){
                // if (preparationOta())
                //     plbc->Execute();
                if (preparationOta()){
                    plbc->Execute();
                }else{
                    if (nextOtaSlot()){
                        plbc->Execute();
                    }else{
                        preparationOtaCompl();
                    }
                }
            }else{
                // qDebug()<<"preparationOtaCompl into localFinish";
                preparationOtaCompl();
            }
        default:
            break;
        }
}

QDebug operator<<(QDebug out, const lbprocess::scaninfo& inf){
    out.noquote()<<inf.devtype<<" "<<discover::addColonsToMac(inf.mac)<<" "<<inf.version;
    for (const auto &i : inf.data) {
        out.noquote()<<i;
    }
    out.noquote()<<" "<<((inf.master)?"BM":"");
    return out;
}

