#ifndef LBSERIALBUSADDRESS_H
#define LBSERIALBUSADDRESS_H

#include <QHostAddress>
#include <QString>
#include <QUrl>

// Adapter between lbnetworkresolver and the stock Qt QModbusTcpClient.
//
// lbnetworkresolver deliberately returns a normal socket-style address:
//     fe80::1234%45
// This is exactly what QHostAddress/QTcpSocket want.
//
// Stock QtSerialBus, however, does not pass NetworkAddressParameter directly
// to QHostAddress. QModbusTcpClient::open() first concatenates
//     networkAddress + ':' + port
// and feeds the result to QUrl::fromUserInput(). For an IPv6 literal that
// intermediate representation must be URL-safe:
//     [fe80::1234%2545]:502
// where brackets delimit the IPv6 host and %25 is the RFC 6874 escaped '%'
// introducing the zone id. QUrl::host() then returns fe80::1234%45 to the
// socket layer.
//
// The adapter is Windows-only. On Linux the project may use the patched
// QtSerialBus path which parses NetworkAddressParameter directly as a
// QHostAddress; feeding that path brackets/%25 would be counterproductive.
namespace lbserialbus {

inline QString networkAddressParameter(const QString &resolvedAddress)
{
#ifdef Q_OS_WIN
    QHostAddress parsed;
    if (!parsed.setAddress(resolvedAddress))
        return resolvedAddress; // hostname or other non-IP input

    if (parsed.protocol() != QAbstractSocket::IPv6Protocol)
        return resolvedAddress; // IPv4 needs no URL adaptation

    const QString scopeId = parsed.scopeId();
    parsed.setScopeId(QString());

    QString urlHost = parsed.toString();
    if (!scopeId.isEmpty()) {
        // QUrl's IPv6 parser recognises the URL zone-id marker "%25".
        // Encode the scope itself as well, although on Windows our resolver
        // normally supplies the numeric interface index (for example "45").
        urlHost += QStringLiteral("%25")
                   + QString::fromLatin1(QUrl::toPercentEncoding(scopeId));
    }

    return QStringLiteral("[") + urlHost + QStringLiteral("]");
#else
    return resolvedAddress;
#endif
}

} // namespace lbserialbus

#endif // LBSERIALBUSADDRESS_H
