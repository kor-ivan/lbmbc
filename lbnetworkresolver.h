#ifndef LBNETWORKRESOLVER_H
#define LBNETWORKRESOLVER_H

#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>

namespace lbnetwork {

// Remember the interface that actually received a reply from a given
// link-local IPv6 address. This is the most reliable source of scope-id.
void rememberInterface(const QString &address, int interfaceIndex);

// Return every up, non-loopback interface that has a link-local IPv6 address.
// Discovery uses this broad set so a PLC behind a P2P/VPN-like adapter is not
// hidden merely because an ordinary Ethernet/Wi-Fi NIC is also up.
QList<QNetworkInterface> discoverIpv6Interfaces();

// Return interfaces suitable for an automatic resolver fallback.
// Point-to-point/tunnel interfaces are excluded when at least one normal
// interface is available, because fallback must be conservative.
QList<QNetworkInterface> activeIpv6Interfaces();

// Resolve a raw IPv6 address to a scoped address. Priority:
//   1) scope already present in address;
//   2) explicit interface hint;
//   3) interface remembered from Discover;
//   4) only unambiguous active interface.
// With multiple interfaces and no exact mapping the address is deliberately
// left unscoped instead of guessing another PLC's last successful interface.
// IPv4 and host names are returned unchanged by scopedAddressString().
QHostAddress scopedAddress(const QString &address,
                           const QString &interfaceHint = QString());
QString scopedAddressString(const QString &address,
                            const QString &interfaceHint = QString());

// For diagnostics/tests.
int rememberedInterfaceIndex(const QString &address);

} // namespace lbnetwork

#endif // LBNETWORKRESOLVER_H
