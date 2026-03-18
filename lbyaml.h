#ifndef LBYAML_H
#define LBYAML_H

#include <QString>
#include <yaml-cpp/yaml.h>
#include <QJsonObject>

class lbyaml
{
public:
    explicit lbyaml(QString filename);
    QByteArray getlbJson();
    static void printlbconf(const QJsonObject &kqbo);
    static QString MacToIPv6(QString mac);
    QString getlbmac(const QJsonObject &kqbo);
    QString getIPv6fromYaml();
    void setlbhost(QString h);
    static const QString NoError;
    QString getErr() const;


    void getallhost(QMultiMap<QString, QString> &lbHostMmap);
    struct lbvar {
        QList<QStringList> var;
        QList<QStringList> var_out;
        bool multisource = false;
        bool retain = false;
        QString init;
    };
    QString getVarStat(QMap<QString, lbvar> &lbVarMap);
    friend QDebug operator<<(QDebug out, const lbyaml::lbvar& varstr);

private:
    static YAML::Node JsonToYaml(QJsonObject qjo, int level=0);
    QJsonObject YamlToJson(const YAML::Node &fnode, QString host);
    YAML::Node config;
    QString err;
    QString host;

    QString find(const YAML::Node& node, const QString& qkey);
    void getvar(QMap<QString, lbvar> &lbVarMap, const YAML::Node& node, const QStringList level = QStringList());
    QStringList expandVar(const QString &key, bool *point = nullptr);
    void addlbVar_isMap(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap);
    void addlbVar_isScalar(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar&, QStringList));
    void addlbVar_isSequence(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar&, QStringList));
    static void addVar(lbvar &lbvar, QStringList level);
    static void addVar_out(lbvar &lbvar, QStringList level);
};

#endif // LBYAML_H
