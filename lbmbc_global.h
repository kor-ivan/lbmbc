#ifndef LBMBC_GLOBAL_H
#define LBMBC_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LBMBC_LIBRARY)
#  define LBMBC_EXPORT Q_DECL_EXPORT  // Для сборки библиотеки
#else
#  define LBMBC_EXPORT Q_DECL_IMPORT  // Для использования библиотекой в приложении
#endif

#endif // LBMBC_GLOBAL_H
