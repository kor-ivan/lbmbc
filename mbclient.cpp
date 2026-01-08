#include "mbclient.h"

MBclient::MBclient(QCoreApplication *a) {
    //reply = new QModbusReply(QModbusReply::Common, 1, this);

    ton = new QTimer(this);
    ton->setSingleShot(true);
    connect(ton, SIGNAL(timeout()),this, SLOT(isTimeout()));
    app=a;
}
MBclient::~MBclient()
{
    if (modbusDevice){
        modbusDevice->disconnectDevice();
        qDebug()<<"...... disconnect";
    }
    qDebug()<<"...... delete modbusDevice, ton";
    delete modbusDevice;
    delete ton;
}

void MBclient::Execute(){
    modbusDevice->connectDevice();
}
void MBclient::Execute(int f, int reg, int q, int a){
    modbusDevice->connectDevice();
    function = f;
    MBreg = reg;
    quant = q;
    MBaddr = a;
}
void MBclient::WriteExecute(int f, int v, int reg, int a){
    WriteFlag = true;
    Execute();
    function = f;
    WriteValue = v;
    MBreg = reg;
    MBaddr = a;
}
void MBclient::isConnected(QModbusDevice::State){
    qDebug()<<modbusDevice->state()<<modbusDevice->error()<<modbusDevice->errorString();
    switch (modbusDevice->state()) {
    case QModbusDevice::ConnectedState:
        if (!WriteFlag){
            ReadHolding();
        }else{
            WriteRegisters();
        }
        break;
    case QModbusDevice::UnconnectedState:
        qDebug()<<"...... Stoping";
        //disconnect(reply,SIGNAL(finished()),this,SLOT(isReadFinish()));
        modbusDevice->disconnect();
        modbusDevice->disconnectDevice();
        app->quit();
        break;
    default:
        break;
    }
}
void MBclient::ReadHolding(){
    if (modbusDevice->state() == QModbusDevice::ConnectedState){
        reply = modbusDevice->sendReadRequest(readRequest(), MBaddr);
        connect(reply,SIGNAL(finished()),this,SLOT(isReadFinish()));
    }
}
void MBclient::WriteRegisters(){
    if (modbusDevice->state() == QModbusDevice::ConnectedState){
        reply = modbusDevice->sendWriteRequest(writeRequest(), MBaddr);
        connect(reply,SIGNAL(finished()),this,SLOT(isWriteFinish()));
    }
}
void MBclient::setTimeOut(const int t){
    if (t>=100 && t<=10000){
        ton->setInterval(t);
    }else{
        qDebug()<<"...... TimeOut out of range";
    }
}
void MBclient::setTCPaddr(const QUrl url){
    modbusDevice = new QModbusTcpClient(this);
    connect(modbusDevice, SIGNAL(stateChanged(QModbusDevice::State)),
            this, SLOT(isConnected(QModbusDevice::State)));
    qDebug()<<"IP ="<<url.host()<<"Port ="<<url.port();
    modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
    modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
}
QModbusDataUnit MBclient::readRequest() const
{
    switch (function) {
    case 1:
        return QModbusDataUnit(QModbusDataUnit::Coils,MBreg,quant);
        break;
    case 2:
        return QModbusDataUnit(QModbusDataUnit::DiscreteInputs,MBreg,quant);
        break;
    case 3:
        return QModbusDataUnit(QModbusDataUnit::HoldingRegisters,MBreg,quant);
        break;
    case 4:
        return QModbusDataUnit(QModbusDataUnit::InputRegisters,MBreg,quant);
        break;
    default:
        return QModbusDataUnit(QModbusDataUnit::HoldingRegisters,MBreg,quant);
    }
}
QModbusDataUnit MBclient::writeRequest() const
{
    QModbusDataUnit data;
    switch (function) {
    case 1:
        data = QModbusDataUnit(QModbusDataUnit::Coils,MBreg,1);
        data.setValue(0, WriteValue);
        return data;
        break;
    case 3:
        data = QModbusDataUnit(QModbusDataUnit::HoldingRegisters,MBreg,1);
        data.setValue(0, WriteValue);
        return data;
        break;
    default:
        data = QModbusDataUnit(QModbusDataUnit::HoldingRegisters,MBreg,1);
        data.setValue(0, WriteValue);
        return data;
    }
}
void MBclient::isReadFinish(){
    //qDebug()<<modbusDevice->state()<<reply->error()<<reply->isFinished();
    if (modbusDevice->state()==QModbusDevice::ConnectedState){
        if (reply->error()==QModbusDevice::NoError){
            qDebug()<<"OK >>>>>>"<<reply->result().values();
        }else{
            qDebug()<<"FAIL >>>>>> "<<reply->errorString();
        }
        ton->start();
    }
}
void MBclient::isWriteFinish(){
    //qDebug()<<modbusDevice->state()<<reply->error()<<reply->isFinished();
    if (modbusDevice->state()==QModbusDevice::ConnectedState){
        if (reply->error()==QModbusDevice::NoError){
            qDebug()<<"OK WRITE >>>>>>";
        }else{
            qDebug()<<"FAIL WRITE >>>>>>";
        }
    }
    app->quit();
}
void MBclient::isTimeout(){
    ReadHolding();
}
