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

QMap<QString, lbyaml::lbvar> lbyaml::getLbVarMap() const
{
    return lbVarMap;
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

void lbyaml::getvar(QMap<QString, lbvar> &lbVarMap, const YAML::Node &node, const QStringList level)
{
    if (node.IsDefined() && node.IsMap()){
        foreach (auto i, node) {
            // if var:
            if (i.first.Scalar() == "var" && i.second.IsMap())
                addlbVar_isMap(i.second, lbVarMap);
            // if not var:
            else if (i.second.IsScalar()){
                if (i.first.Scalar() == "var")
                    addlbVar_isScalar(i.second, lbVarMap, level, addVar);
                if (i.first.Scalar() == "var_out")
                    addlbVar_isScalar(i.second, lbVarMap, level, addVar_out);
            }
            else if (i.second.IsSequence()){
                if (i.first.Scalar() == "var")
                    addlbVar_isSequence(i.second, lbVarMap, level, addVar);
                if (i.first.Scalar() == "var_out")
                    addlbVar_isSequence(i.second, lbVarMap, level, addVar_out);
            }
            else{
                QStringList qsl = level;
                getvar(lbVarMap, i.second, qsl << QString::fromStdString(i.first.Scalar()));
            }
        }
    }
}

QStringList lbyaml::expandVar(const QString &key, bool *point)
{
    QStringList result;

    // 1. Ищем диапазон: буквы + число + ровно две точки + число
    // Используем \.\. чтобы явно указать две точки
    // static const QRegularExpression rangeRe(R"(^([a-zA-Z]+)(\d+)\.\.(\d+)$)");
    static const QRegularExpression rangeRe(R"(^([\w_]+?)(\d+)\.\.(\d+)$)");

    // 2. Ищем "вариант с точкой": буквы + число + одиночная точка
    // [^.] перед точкой гарантирует, что мы не попали на вторую точку из ".."
    // static const QRegularExpression dotRe(R"(^([a-zA-Z]+)(\d+)\.[^.])");
    static const QRegularExpression dotRe(R"(^([\w_]+?)(\d+)\.)");

    // 1. Проверяем на диапазон di0..15
    QRegularExpressionMatch rangeMatch = rangeRe.match(key);
    if (rangeMatch.hasMatch()) {
        QString prefix = rangeMatch.captured(1);
        int start = rangeMatch.captured(2).toInt();
        int end = rangeMatch.captured(3).toInt();

        for (int i = start; i <= end; ++i) {
            result.append(prefix + QString::number(i));
        }

        if (point) *point = false;
        return result;
    }

    // 2. Проверяем на наличие точки mw0.0..15
    QRegularExpressionMatch dotMatch = dotRe.match(key);
    if (dotMatch.hasMatch()) {
        // Забираем только префикс и первое число (mw + 0)
        result.append(dotMatch.captured(1) + dotMatch.captured(2));
        if (point) *point = true;
        return result;
    }

    // 3. Если ничего не подошло, возвращаем как есть
    result.append(key);
    if (point) *point = false;
    return result;
}

void lbyaml::addlbVar_isMap(const YAML::Node &node, QMap<QString, lbvar> &lbVarMap)
{
    QStringList keylist;
    foreach (auto j, node) {
        keylist = expandVar(QString::fromStdString(j.first.Scalar()));
        foreach (auto key, keylist) {
            lbvar varstr;
            if (lbVarMap.contains(key))
                varstr = lbVarMap.value(key);
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
            lbVarMap.insert(key, varstr);
        }
    }
}

void lbyaml::addlbVar_isScalar(const YAML::Node& node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar&, QStringList))
{
    bool point = false;
    QStringList keylist = expandVar(QString::fromStdString(node.Scalar()), &point);
    QStringList levellist;
    if (!point)
        levellist = expandVar(level.last());
    else
        levellist << level.last();
    if (keylist.size()==levellist.size()){
        QString key;
        QStringList l = level;
        for (int k = 0; k < keylist.size(); ++k) {
            lbvar varstr;
            key = keylist.at(k);
            if (lbVarMap.contains(key))
                varstr = lbVarMap.value(key);
            l.last() = levellist.at(k);
            pf(varstr, l);
            lbVarMap.insert(key, varstr);
        }
    }else{
        // qDebug()<<keylist<<levellist;
        err = QString::fromStdString(node.Scalar()) + " " + level.last() + " the left and right ranges are not equal";
    }
}

void lbyaml::addlbVar_isSequence(const YAML::Node &node, QMap<QString, lbvar> &lbVarMap, const QStringList &level, void (*pf)(lbvar &, QStringList))
{

    foreach (auto j, node){
        if (j.IsScalar()){
            QStringList keylist = expandVar(QString::fromStdString(j.Scalar()));
            foreach (auto key, keylist) {
                lbvar varstr;
                if (lbVarMap.contains(key))
                    varstr = lbVarMap.value(key);
                pf(varstr, level);
                lbVarMap.insert(key, varstr);
            }
        }
    }
}

void lbyaml::addVar(lbvar &lbvar, QStringList level)
{
    lbvar.var.append(level);
}

void lbyaml::addVar_out(lbvar &lbvar, QStringList level)
{
    lbvar.var_out.append(level);
}


QString lbyaml::getErr() const
{
    return err;
}



QMultiMap<QString, QString> lbyaml::getallhost()
{
    QString h;
    foreach (auto i, config) {
        h = QString::fromStdString(i.first.Scalar());
        lbHostMmap.insert(h, find(i.second, "macaddr"));
    }
    return lbHostMmap;
}

lbyaml::lbvarstat lbyaml::getVarStat()
{
    getvar(lbVarMap, config[host]);
    lbvarstat statstr;
    statstr.quantity = lbVarMap.size();
    for (auto it = lbVarMap.begin(); it != lbVarMap.end(); ++it) {
        if (it.value().var.size()>1 && it.value().multisource == false)
            statstr.mustMultisource<<it.key();
        if (it.value().var.size() == 0 || it.value().var_out.size() == 0)
            statstr.handlingVar<<it.key();
        if (!it.value().var.contains(QStringList() << "forte") && !it.value().var_out.contains(QStringList() << "forte"))
            statstr.noaddedForte<<it.key();
    }

    return statstr;
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
