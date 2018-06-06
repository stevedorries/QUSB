#ifndef KAERHID_H
#define KAERHID_H

#ifdef Q_OS_WIN
#if defined(KAERHID_LIBRARY)
#  define KAERHIDSHARED_EXPORT __declspec(dllexport)
#else
#  define KAERHIDSHARED_EXPORT __declspec(dllimport)
#endif

#else
#  define KAERHIDSHARED_EXPORT

#endif
//class KAERHIDSHARED_EXPORT KaerHid
//{

//public:
//    KaerHid();
//};

extern "C" {
KAERHIDSHARED_EXPORT int test();
KAERHIDSHARED_EXPORT int getSamvId(char * samvBuf);

}

#endif // KAERHID_H
