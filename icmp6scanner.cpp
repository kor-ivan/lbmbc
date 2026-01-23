#include "icmp6scanner.h"
#include <QtConcurrent>
#ifdef Q_OS_WIN
#include <iphlpapi.h>
#include <ws2tcpip.h>


Icmp6Scanner::Icmp6Scanner(QObject *parent)
    : QObject{parent}
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}


void Icmp6Scanner::sendMulticastRequest(const QNetworkInterface &iface, const QHostAddress &target)
{
    QFuture<void> future = QtConcurrent::run([this, iface, target]() {
        SOCKET sock = WSASocketW(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6, nullptr, 0, 0);
        if (sock == INVALID_SOCKET) {
            qCritical() << "Ошибка создания сокета:" << WSAGetLastError();
            emit scanFinished(iface.humanReadableName());
            return;
        }

        sockaddr_in6 bindAddr = {};
        bindAddr.sin6_family = AF_INET6;
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv6Protocol &&
                entry.ip().isGlobal() == false) {
                QHostAddress addr = entry.ip();
                Q_IPV6ADDR raw = addr.toIPv6Address();
                memcpy(&bindAddr.sin6_addr, &raw, sizeof(raw));
                break;
            }
        }
        bindAddr.sin6_scope_id = 0;  // Не указываем scope_id при bind

        if (bind(sock, reinterpret_cast<SOCKADDR*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            qCritical() << "Ошибка bind:" << err;
            closesocket(sock);
            emit scanFinished(iface.humanReadableName());
            return;
        }

        sockaddr_in6 destAddr = {};
        destAddr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, target.toString().toUtf8().constData(), &destAddr.sin6_addr);
        destAddr.sin6_scope_id = iface.index();

        uint8_t buffer[64] = {};
        Icmp6Header* hdr = (Icmp6Header*)buffer;
        hdr->type = ICMP6_ECHO_REQUEST;
        hdr->code = 0;
        hdr->identifier = htons(1234);
        hdr->sequence = htons(1);
        strcpy_s(hdr->SendData, sizeof(SendBuff), SendBuff);

        int sent = sendto(sock, (const char*)buffer, sizeof(Icmp6Header), 0,
                          reinterpret_cast<SOCKADDR*>(&destAddr), sizeof(destAddr));
        if (sent == SOCKET_ERROR) {
            qCritical() << "Ошибка отправки:" << WSAGetLastError();
            closesocket(sock);
            emit scanFinished(iface.humanReadableName());
            return;
        }
        receiveReplies(sock, iface);
        closesocket(sock);
        emit scanFinished(iface.humanReadableName());
    });
}

Icmp6Scanner::~Icmp6Scanner() {
    WSACleanup();
    // qDebug()<<"Delete Icmp6Scanner";
}


void Icmp6Scanner::receiveReplies(SOCKET sock, const QNetworkInterface &iface)
{
    QElapsedTimer timer;
    timer.start();

    char buffer[64] = {};
    sockaddr_in6 recvAddr = {};
    int recvLen = sizeof(recvAddr);

    DWORD timeout = timeoutMs;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    u_long nonblocking = 1;
    ioctlsocket(sock, FIONBIO, &nonblocking);

    while (timer.elapsed() < timeoutMs) {
        int len = recvfrom(sock, buffer, sizeof(buffer), 0,
                           reinterpret_cast<SOCKADDR*>(&recvAddr), &recvLen);
        if (len == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) {
                //QThread::usleep(1);  // <--- разгрузка процессора
                continue;
            }
            qWarning() << "recvfrom error:" << err;
            break;
        }

        if (recvAddr.sin6_scope_id != iface.index())
            continue;

        uint8_t type = buffer[0];
        if (type != 129)
            continue;

        char ipStr[INET6_ADDRSTRLEN] = {};
        inet_ntop(AF_INET6, &recvAddr.sin6_addr, ipStr, sizeof(ipStr));
        //QHostAddress from(QString::fromUtf8(ipStr));
        emit replyReceived(QString::fromUtf8(ipStr), timer.nsecsElapsed()/1000, iface.index());
    }

}
#endif

Icmp6Scanner::Icmp6Scanner(QObject *parent)
{

}

Icmp6Scanner::~Icmp6Scanner()
{

}
void Icmp6Scanner::setTimeoutMs(int timeout)
{
    timeoutMs = timeout;
}


int Icmp6Scanner::getTimeoutMs()
{
    return timeoutMs;
}

void Icmp6Scanner::sendMulticastRequest(const QNetworkInterface &iface, const QHostAddress &target)
{

}
