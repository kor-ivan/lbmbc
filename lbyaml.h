#ifndef LBYAML_H
#define LBYAML_H

#include <QString>
#include <yaml-cpp/yaml.h>
#include <QJsonObject>
#include "lbmbc_global.h"

class LBMBC_EXPORT lbyaml
{
public:
    enum YamlMode{
        file,
        data
    };
    explicit lbyaml(QString filename, YamlMode mode = file);
    QByteArray getlbJson();
    static void printlbconf(const QJsonObject &kqbo);
    static QString getlbconf(const QJsonObject &kqbo);
    static QString MacToIPv6(QString mac);
    QString getlbmac(const QJsonObject &kqbo);
    QString getIPv6fromYaml();
    void setlbhost(QString h);
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
};



#endif // LBYAML_H
