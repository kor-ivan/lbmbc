#ifndef ICMP6SCANNER_H
#define ICMP6SCANNER_H

#include <QObject>
#include <QNetworkInterface>
#include <QHostAddress>
#ifdef Q_OS_WIN
#include <winsock2.h>
#endif
class Icmp6Scanner : public QObject
{
    Q_OBJECT
public:
    explicit Icmp6Scanner(QObject *parent = nullptr);
    virtual ~ Icmp6Scanner();
    void setTimeoutMs(int timeout);
    int getTimeoutMs();
    void sendMulticastRequest(const QNetworkInterface& iface, const QHostAddress& target);

signals:
    void replyReceived(const QString& sender, int rttmcs, int ifindex);
    void scanFinished(const QString& ifaceName);

private:
    const uint8_t ICMPV6_ECHO_REQUEST = 128;
    const char SendBuff[32] = "Hello LogicBox made by elprivod";
    int timeoutMs = 1000;
    struct Icmp6Header {
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t identifier;
        uint16_t sequence;
        char SendData[32];
    };
#ifdef Q_OS_WIN
    void receiveReplies(SOCKET sock, const QNetworkInterface& iface);
#endif
};

#endif // ICMP6SCANNER_H
