#include "lbnetworkresolver.h"

#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <algorithm>

namespace {
QMutex g_mutex;
// A raw link-local IPv6 address is not globally unique. Keep every interface
// on which it was actually observed instead of silently overwriting one IF
// with another (the old LAST-write-wins behaviour).
QHash<QString, QSet<int>> g_interfacesByAddress;

QString addressKey(const QHostAddress &address)
{
    QHostAddress raw(address);
    raw.setScopeId(QString());
    return raw.toString().toLower();
}

bool hasIpv6LinkLocal(const QNetworkInterface &iface)
{
    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
        const QHostAddress ip = entry.ip();
        if (ip.protocol() == QAbstractSocket::IPv6Protocol && ip.isLinkLocal())
            return true;
    }
    return false;
}

bool isUsable(const QNetworkInterface &iface)
{
    if (!iface.isValid())
        return false;

    const auto flags = iface.flags();
    if (!(flags & QNetworkInterface::IsUp))
        return false;
    if (flags & QNetworkInterface::IsLoopBack)
        return false;

    return hasIpv6LinkLocal(iface);
}

QString scopeIdForInterface(const QNetworkInterface &iface)
{
#ifdef Q_OS_WIN
    return QString::number(iface.index());
#else
    return iface.name();
#endif
}

QNetworkInterface interfaceFromHint(const QString &hint)
{
    if (hint.isEmpty())
        return {};

    bool ok = false;
    const int index = hint.toInt(&ok);
    if (ok) {
        const QNetworkInterface iface = QNetworkInterface::interfaceFromIndex(index);
        if (iface.isValid())
            return iface;
    }

    QNetworkInterface iface = QNetworkInterface::interfaceFromName(hint);
    if (iface.isValid())
        return iface;

    for (const QNetworkInterface &candidate : QNetworkInterface::allInterfaces()) {
        if (candidate.humanReadableName() == hint)
            return candidate;
    }

    return {};
}

QSet<int> cachedInterfaces(const QString &key)
{
    QMutexLocker locker(&g_mutex);
    return g_interfacesByAddress.value(key);
}

QSet<int> usableCachedInterfaces(const QString &key)
{
    QSet<int> result;
    const QSet<int> indexes = cachedInterfaces(key);
    for (int index : indexes) {
        if (isUsable(QNetworkInterface::interfaceFromIndex(index)))
            result.insert(index);
    }
    return result;
}

void rememberKeyInterface(const QString &key, int interfaceIndex)
{
    if (key.isEmpty() || interfaceIndex <= 0)
        return;

    QMutexLocker locker(&g_mutex);
    g_interfacesByAddress[key].insert(interfaceIndex);
}

QNetworkInterface uniqueCachedInterface(const QString &key)
{
    const QSet<int> indexes = usableCachedInterfaces(key);
    if (indexes.size() != 1)
        return {};

    return QNetworkInterface::interfaceFromIndex(*indexes.constBegin());
}

QString indexesToString(const QSet<int> &indexes)
{
    QList<int> ordered = indexes.values();
    std::sort(ordered.begin(), ordered.end());
    QStringList values;
    for (int index : ordered)
        values << QString::number(index);
    return values.join(',');
}
} // namespace

namespace lbnetwork {

QList<QNetworkInterface> discoverIpv6Interfaces()
{
    QList<QNetworkInterface> result;
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (isUsable(iface))
            result.append(iface);
    }
    return result;
}

QList<QNetworkInterface> activeIpv6Interfaces()
{
    QList<QNetworkInterface> normal;
    QList<QNetworkInterface> pointToPoint;

    for (const QNetworkInterface &iface : discoverIpv6Interfaces()) {
        if (iface.flags() & QNetworkInterface::IsPointToPoint)
            pointToPoint.append(iface);
        else
            normal.append(iface);
    }

    // Avoid VPN/TUN adapters when at least one ordinary Ethernet/Wi-Fi style
    // interface is available. If only P2P exists, keep it as a last resort.
    return normal.isEmpty() ? pointToPoint : normal;
}

void rememberInterface(const QString &address, int interfaceIndex)
{
    if (interfaceIndex <= 0)
        return;

    QHostAddress parsed;
    if (!parsed.setAddress(address) || parsed.protocol() != QAbstractSocket::IPv6Protocol || !parsed.isLinkLocal())
        return;

    const QNetworkInterface iface = QNetworkInterface::interfaceFromIndex(interfaceIndex);
    if (!isUsable(iface))
        return;

    const QString key = addressKey(parsed);
    rememberKeyInterface(key, interfaceIndex);

    const QSet<int> indexes = cachedInterfaces(key);
    qDebug().noquote()
        << "LBNetworkResolver: remember"
        << key
        << "IF =" << interfaceIndex
        << iface.humanReadableName()
        << "known IFs =" << indexesToString(indexes);
}

int rememberedInterfaceIndex(const QString &address)
{
    QHostAddress parsed;
    if (!parsed.setAddress(address) || parsed.protocol() != QAbstractSocket::IPv6Protocol)
        return 0;

    const QSet<int> indexes = usableCachedInterfaces(addressKey(parsed));
    return indexes.size() == 1 ? *indexes.constBegin() : 0;
}

QHostAddress scopedAddress(const QString &address, const QString &interfaceHint)
{
    QHostAddress parsed;
    if (!parsed.setAddress(address))
        return {};

    if (parsed.protocol() != QAbstractSocket::IPv6Protocol || !parsed.isLinkLocal())
        return parsed;

    const QString key = addressKey(parsed);

    // Existing scope is authoritative. It also teaches the cache, but it does
    // not erase any other observed endpoint for the same raw address.
    if (!parsed.scopeId().isEmpty()) {
        const QNetworkInterface scopedIface = interfaceFromHint(parsed.scopeId());
        if (isUsable(scopedIface))
            rememberKeyInterface(key, scopedIface.index());
        return parsed;
    }

    QNetworkInterface selected;
    QString reason;

    // 1. Explicit hint from the caller is authoritative for this operation.
    if (!interfaceHint.isEmpty()) {
        const QNetworkInterface hinted = interfaceFromHint(interfaceHint);
        if (isUsable(hinted)) {
            selected = hinted;
            reason = "explicit";
        }
    }

    // 2. Discover cache is safe only when the raw address was observed on one
    // interface. If it was observed on several, guessing FIRST or LAST can
    // target a different L2 endpoint, so keep the address unresolved unless
    // the caller supplies a scope/hint.
    if (!selected.isValid()) {
        const QSet<int> indexes = usableCachedInterfaces(key);
        if (indexes.size() == 1) {
            selected = uniqueCachedInterface(key);
            if (selected.isValid())
                reason = "discover-cache";
        } else if (indexes.size() > 1) {
            qWarning().noquote()
                << "LBNetworkResolver: ambiguous endpoint for"
                << key
                << "observed on IFs"
                << indexesToString(indexes)
                << "- explicit scope/interface is required";
            return parsed;
        }
    }

    const QList<QNetworkInterface> candidates = activeIpv6Interfaces();

    // 3. With one sensible active interface there is no ambiguity even before
    // Discover has populated the cache.
    if (!selected.isValid() && candidates.size() == 1) {
        selected = candidates.first();
        reason = "single-active";
    }

    // Do not guess from a process-wide "last successful" interface here.
    // That interface can belong to another PLC and can therefore route a raw
    // YAML/MAC-derived address to the wrong L2 segment. With several candidates
    // and no exact mapping the only safe result is "unresolved".

    if (!selected.isValid()) {
        QStringList names;
        for (const QNetworkInterface &candidate : candidates) {
            names << QString("%1[%2]")
                         .arg(candidate.humanReadableName())
                         .arg(candidate.index());
        }
        qWarning().noquote()
            << "LBNetworkResolver: cannot resolve scope for"
            << key
            << "active candidates:"
            << names.join(", ");
        return parsed;
    }

    parsed.setScopeId(scopeIdForInterface(selected));
    rememberKeyInterface(key, selected.index());

    qDebug().noquote()
        << "LBNetworkResolver:"
        << key
        << "->" << parsed.toString()
        << "IF =" << selected.index()
        << selected.humanReadableName()
        << "source =" << reason;

    return parsed;
}

QString scopedAddressString(const QString &address, const QString &interfaceHint)
{
    QHostAddress parsed;
    if (!parsed.setAddress(address))
        return address;

    const QHostAddress scoped = scopedAddress(address, interfaceHint);
    if (scoped.isNull())
        return address;
    return scoped.toString();
}

} // namespace lbnetwork
