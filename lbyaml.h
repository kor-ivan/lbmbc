#ifndef LBYAML_H
#define LBYAML_H

#include <QString>
#include <yaml-cpp/yaml.h>
#include <QJsonObject>
#include "lbmbc_global.h"

class LBMBC_EXPORT lbyaml : public QObject
{
    Q_OBJECT
public:
    enum YamlMode{
        file,
        data
    };
    enum outputFormat{
        raw,
        retainY
    };

    explicit lbyaml(const QString filename, YamlMode mode = file, QObject *parent = nullptr);
    QByteArray getlbJson();
    QJsonObject getlbJsonObject();
    static void printlbconf(const QJsonObject &kqbo);
    static QString getlbconf(const QJsonObject &kqbo, outputFormat f = raw);
    static QString MacToIPv6(QString mac);
    QString getlbmac(const QJsonObject &kqbo);
    QString getIPv6fromYaml();
    void setlbhost(QString h);
    QString getlbhost() const;
    static const QString NoError;
    QString getErr() const;


    QMultiMap<QString, QString> getallhost();
    struct lbvar {
        QList<QStringList> var;
        QList<QStringList> var_out;
        bool multisource = false;
        bool retain = false;
        QString init;
    };
    struct lbvarstat {
        int quantity = 0;
        QStringList mustMultisource;
        QStringList handlingVar;
        QStringList noaddedForte;
    };
    struct lbhost{
        QString mac;
        int line;
    };
    QMultiMap<QString, lbhost> getallhostline();

    lbvarstat getVarStat();

    QMap<QString, lbvar> getLbVarMap() const;
    static QStringList expandVar(const QString &key, bool *point = nullptr);
    static bool isValidMacAddress(const QString &mac);
    void implementLbVarMap(const QMap<QString, lbyaml::lbvar> &updatedMap);
    QString getFormattedYaml(outputFormat f = raw);

    void setConfig(const QString filename, YamlMode mode = file);

private:
    static YAML::Node JsonToYaml(QJsonObject qjo, int level=0);
    QJsonObject YamlToJson(const YAML::Node &fnode, QString host);
    YAML::Node config;
    QString err;
    QString host;

    QMap<QString, lbvar> lbVarMap;
    QMultiMap<QString, QString> lbHostMmap;

    QString find(const YAML::Node& node, const QString& qkey);
    void getvar(QMap<QString, lbvar> &lbVarMap, const YAML::Node& node, const QStringList level = QStringList());

    void addlbVar_isMap(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap);
    void addlbVar_isScalar(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar&, QStringList), int size = 1);
    void addlbVar_isSequence(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar&, QStringList));
    static void addVar(lbvar &lbvar, QStringList level);
    static void addVar_out(lbvar &lbvar, QStringList level);
    static void cleanRetain(QJsonObject &kqbo);
};



#endif // LBYAML_H
