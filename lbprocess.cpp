#include "lbprocess.h"
#include "discover.h"
#include <QDir>

lbprocess::lbprocess(QObject *parent, LBclient *lbc)
    : QObject{parent}, plbc(lbc)
{
    plbc->setLbConn(LBclient::MaintainTCP);
}

void lbprocess::run(processMode m, const QString var)
{
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
    // qDebug()<<"preota"<<*ri_lbota<<plbc->getlbDeviceError();
    if (lbscanMap.value(*ri_lbota).devtype!="unknown"){
        // qDebug()<<otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin";
        plbc->setOtaFilename(otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin");
        if (plbc->getlbDeviceError()==QModbusDevice::NoError){
            SendOneMessage("Now ota slot " + QString::number(*ri_lbota) + " from " + otaPath + lbscanMap.value(*ri_lbota).devtype + ".bin");
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
    }
    return false;
}

void lbprocess::setNumOfVarRetries(int newNumOfVarRetries)
{
    numOfVarRetries = newNumOfVarRetries;
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
            qsizetype index = lbstr.indexOf("bustab_dump1:");
            if (index!=-1)
                // inf.mac = discover::addColonsToMac(lbstr.sliced(index + 17, 12));
                inf.mac = lbstr.sliced(index + 17, 12);
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
        if (error != QModbusDevice::NoError && cRetries<numOfVarRetries){
            cRetries++;
            SendOneMessage("trying the slot " + QString::number(i_lbscanMap.key()) + " ... " + QString::number(cRetries));
            plbc->Execute();
        }else{
            if (error == QModbusDevice::NoError){
                inf = i_lbscanMap.value();
                inf.devtype = result.at(0);
                inf.version = result.at(1);
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
                    lbotaKeys = lbscanMap.keys();
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
                            qDebug()<<"preparationOtaCompl into localExeCompl";
                            preparationOtaCompl();
                        }
                    }
                    break;
                case restartall:
                    SendOneMessage("Start reboot all...");
                    phase = reboot;
                    lbotaKeys = lbscanMap.keys();
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
                QString s;
                plbc->setQueryString({"get", "sys.devtype", scanVar});
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
                if (preparationOta())
                    plbc->Execute();
            }else{
                qDebug()<<"preparationOtaCompl into localFinish";
                preparationOtaCompl();
            }
        default:
            break;
        }
}

QDebug operator<<(QDebug out, const lbprocess::scaninfo& inf){
    out.noquote()<<inf.devtype<<" "<<discover::addColonsToMac(inf.mac)<<" "<<inf.version<<" "<<((inf.master)?"BM":"");
    return out;
}

