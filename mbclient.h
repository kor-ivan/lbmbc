#ifndef MBCLIENT_H
#define MBCLIENT_H

#include <QObject>
#include <QCoreApplication>
#include <QtSerialBus/QModbusTcpClient>
#include <QtSerialBus/QModbusRtuSerialClient>
#include <QUrl>
#include <QVariant>
#include <QTimer>
#include "lbclient.h"


class MBclient : public QObject
{
    Q_OBJECT
public:
    MBclient(QCoreApplication *a);
    ~MBclient();
    void Execute();
    void Execute(int f, int reg, int q);
    void WriteExecute(int f, int value, int reg);
    void setTimeOut(int);
    void setTCPaddr(QUrl);
    void setSerialAddr(const QString &portName,
                       int addr,
                       int baudRate = 9600,
                       int dataBits = 8,
                       int parity = 0,
                       int stopBits = 1);
    void setAddr(int addr);

    void setDebugMask(LBclient::DebugMaskBit newDebugMask);

public slots:
    void isConnected(QModbusDevice::State);
    void isReadFinish();
    void isWriteFinish();
    void isTimeout();
private:
    QModbusClient *modbusDevice = nullptr;
    QModbusDataUnit readRequest() const;
    QModbusDataUnit writeRequest() const;
    QModbusReply *reply = nullptr;
    QTimer *ton = nullptr;
    QCoreApplication *app = nullptr;
    void ReadHolding();
    void WriteRegisters();
    void clearReply();

    int function = 3;
    int serverAddress = 1;
    int MBreg = 0;
    int quant = 10;
    int WriteValue = 0;
    bool WriteFlag = false;
    const int DefaultRtuTimeout = 4000;


    int packetIndex = 0;
    LBclient::DebugMaskBit debugMask = LBclient::none;

};

#endif // MBCLIENT_H
