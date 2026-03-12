#include "lbyaml.h"
#include "qtyaml.cpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <iostream>

const QString lbyaml::NoError = "No error";

lbyaml::lbyaml(QString filename)
{
    try {
        config = YAML::LoadFile(filename.toStdString());
        err = NoError;
    } catch(const YAML::ParserException& e) {
        err = e.what();
        qDebug() << "Error parsing YAML: " << err;
    }
}

QByteArray lbyaml::getlbJson()
{
    QJsonDocument doc(YamlToJson(config, host));
    return doc.toJson(QJsonDocument::Compact);
}

void lbyaml::printlbconf(const QJsonObject &kqbo)
{
    std::cout<<JsonToYaml(kqbo)<<std::endl;
}

QJsonObject lbyaml::YamlToJson(const YAML::Node &fnode, QString host)
{
    YAML::Node node(fnode[host]);
    QJsonObject qjo;
    for (YAML::const_iterator yi = node.begin(); yi!=node.end(); ++yi){
        QString key = QString::fromStdString(yi->first.Scalar());
        if (key == "slot-1")
            qjo.insert("hostname", host);
        int i = 0;
        QJsonArray qja;
        switch (node[key].Type()) {
        case YAML::NodeType::Scalar:
            qjo.insert(key, QString::fromStdString(node[key].Scalar()));
            break;
        case YAML::NodeType::Sequence:
            for (YAML::const_iterator yii = node[key].begin(); yii!=node[key].end(); ++yii){
                qja.append(QString::fromStdString(node[key][i].Scalar()));
                ++i;
            }
            qjo.insert(key, qja);
            break;
        case YAML::NodeType::Map:
            qjo.insert(key, YamlToJson(node, key));
            break;
        default:
            break;
        }
    }
    return qjo;
}

QString lbyaml::find(const YAML::Node &node, const QString &qkey)
{
    if (!node.IsDefined()) return QString();
    if (node.IsMap()) {
        std::string key = qkey.toStdString();
        if (node[key]) {
            if (node[key].IsScalar()) {
                return node[key].as<QString>();
            }
        }
        for (auto it = node.begin(); it != node.end(); ++it) {
            QString result = find(it->second, qkey);
            if (!result.isEmpty()) return result;
        }
    }
    return QString();
}

void lbyaml::getvar(QMap<QString, lbvar> &lbVarMap, const YAML::Node &node)
{
    if (node.IsDefined()){
        switch (node.Type()) {
        case YAML::NodeType::Scalar:
            std::cout<<node.Scalar()<<"is Scalar";
            break;
        case YAML::NodeType::Map:
            std::cout<<"is Map";
            foreach (auto i, node) {
                std::cout<<i.first.Scalar()<<" "<<i.second.IsMap()<<std::endl;
                if (i.first.Scalar() == "var" && i.second.IsMap()){
                    foreach (auto j, i.second) {
                        std::cout<<j.first.Scalar()<<j.second.Type()<<std::endl;
                        lbvar varstr;
                        if (j.second.IsMap()){
                            foreach (auto k, j.second) {
                                if (k.first.Scalar() == "multisource" && k.second.Scalar() == "y")
                                    varstr.multisource = true;
                                if (k.first.Scalar() == "retain" && k.second.Scalar() == "y")
                                    varstr.retain = true;
                                if (k.first.Scalar() == "init" && k.second.IsScalar())
                                    varstr.init = k.second.as<QString>();

                            }
                        }
                        lbVarMap.insert(j.first.as<QString>(), varstr);
                    }
                }
            }
            break;
        case YAML::NodeType::Sequence:
            std::cout<<"is Sequence";
            break;
        default:
            break;
        }
    }
}

QString lbyaml::getErr() const
{
    return err;
}



void lbyaml::getallhost(QMultiMap<QString, QString> &lbHostMmap)
{
    QString h;
    foreach (auto i, config) {
        h = QString::fromStdString(i.first.Scalar());
        lbHostMmap.insert(h, find(i.second, "macaddr"));
    }
}

QString lbyaml::getVarStat(QMap<QString, lbvar> &lbVarMap)
{
    std::cout<<"into getVarStat..."<<std::endl;
    getvar(lbVarMap, config[host]);

    return QString();
}

YAML::Node lbyaml::JsonToYaml(QJsonObject qjo, int level)
{
    YAML::Node lbconf;
    ++level;
    if (qjo.find("hostname")!=qjo.end()){
        QString host = qjo.find("hostname")->toString();
        //qDebug()<<host;
        qjo.remove("hostname");
        lbconf[host] = JsonToYaml(qjo, level);
        return lbconf;
    }
    QStringList keys = qjo.keys();
    for (QStringList::Iterator sli = keys.begin();sli!=keys.end();++sli){
        if (qjo[*sli].isObject()){
            lbconf[*sli] = JsonToYaml(qjo[*sli].toObject(), level);
            if (level>2)
                lbconf[*sli].SetStyle(YAML::EmitterStyle::Flow);
        }
        if (qjo[*sli].isString()){
            lbconf[*sli] = qjo[*sli].toString();
        }
        if (qjo[*sli].isArray()){
            for (qsizetype i=0; i<qjo[*sli].toArray().size();++i){
                if (qjo[*sli].toArray().at(i).type() == QJsonValue::String)
                    lbconf[*sli].push_back(qjo[*sli].toArray().at(i).toString());
                if (qjo[*sli].toArray().at(i).type() == QJsonValue::Double)
                    lbconf[*sli].push_back(QString::number(qjo[*sli].toArray().at(i).toDouble()));
            }
            lbconf[*sli].SetStyle(YAML::EmitterStyle::Flow);
        }

    }
    return lbconf;
}

QString lbyaml::MacToIPv6(QString mac)
{
    QList<QString> qmac;
    // todo check for format MAC
    mac.remove(":");
    for (int i=0;i<6;++i){
        qmac.append(mac.sliced(i*2,2));
    }
    qmac.insert(3,"ff");
    qmac.insert(4,"fe");
    bool *ok = nullptr;
    // QString t = qmac.at(0);
    // int a = qmac.at(0).toInt(ok, 16) ^ 0x2;
    // qDebug()<<a;
    // qmac[0] = QString::number(qmac.at(0).toInt(ok, 16) ^ 0x2, 16);
    qmac[0] = QString::number(qmac.at(0).toInt(ok, 16) ^ 0x2, 16);
    QStringList ipv6("fe80:");
    for (int i=0; i<4;++i){
        ipv6.append(qmac.at(2*i)+qmac.at(2*i+1));
    }
    return ipv6.join(":");
}

QString lbyaml::getlbmac(const QJsonObject &kqbo)
{
    auto ikqbo = kqbo.find("slot-1");
    if (ikqbo!=kqbo.end()){
        ikqbo = ikqbo.value().toObject().find("macaddr");
        if (ikqbo!=kqbo.end())
            return ikqbo.value().toString();
    }
    return "empty";
}

QString lbyaml::getIPv6fromYaml()
{
    return MacToIPv6(getlbmac(YamlToJson(config, host)));
}

void lbyaml::setlbhost(QString h)
{
    host = h;
}

QDebug operator<<(QDebug out, const lbyaml::lbvar& varstr){
    out<<varstr.var<<varstr.var_out<<varstr.var_dublicate<<varstr.multisource<<varstr.retain<<varstr.init;
    return out;
}
