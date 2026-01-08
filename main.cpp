#include <QCoreApplication>
#include "lbconsole.h"



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    setlocale(LC_ALL, "");

    lbconsole console(&a);
    console.implement();

    return a.exec();
}
