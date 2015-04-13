#ifndef KAERHID_GLOBAL_H
#define KAERHID_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(KAERHID_LIBRARY)
#  define KAERHIDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define KAERHIDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // KAERHID_GLOBAL_H
