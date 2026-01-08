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
private:
    static YAML::Node JsonToYaml(QJsonObject qjo, int level=0);
    QJsonObject YamlToJson(const YAML::Node &fnode, QString host);
    YAML::Node config;
    QString err;
    QString host;
};

#endif // LBYAML_H
