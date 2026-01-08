#ifndef MBCLIENT_H
#define MBCLIENT_H

#include <QObject>
#include <QCoreApplication>
#include <QtSerialBus/QModbusTcpClient>
#include <QUrl>
#include <QVariant>
#include <QTimer>


class MBclient : public QObject
{
    Q_OBJECT
public:
    MBclient(QCoreApplication *a);
    ~MBclient();
    void Execute();
    void Execute(int f, int reg, int q, int a);
    void WriteExecute(int f, int value, int reg, int a);
    void setTimeOut(int);
    void setTCPaddr(QUrl);
    //void setMBaddr(int);
    //void setFunc(int);
    //void setReg(int);
    //void setQuant(int);
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
    int function = 3;
    int MBaddr = 1;
    int MBreg = 0;
    int quant = 10;
    int WriteValue = 0;
    bool WriteFlag = false;
};

#endif // MBCLIENT_H
