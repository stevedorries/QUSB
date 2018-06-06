#ifndef SAMVPROTOCAL_H
#define SAMVPROTOCAL_H

#include <QByteArray>


enum SAMVProtocalType{
    SAMVProtocalType_UNDEFINE,
    SAMVProtocalType_A,
    SAMVProtocalType_B
};


enum LocalReadCMD{
    CMD_NONE = 0,
    CMD_READSAMVNUMBER = 0x12,
};


class SAMVProtocal
{
public:
    SAMVProtocal();
    ~SAMVProtocal();
    virtual int check(const QByteArray &indata);
    virtual bool decode(const QByteArray &indata);
    virtual QByteArray encode();

    static const int kPreReadLen = 7;

    int type;

    quint16 length;
    QByteArray data;
    quint8 check_sum;



};

class SAMVInputProtocal:public SAMVProtocal
{
public:
    SAMVInputProtocal();
    virtual bool decode(const QByteArray &indata);
    virtual QByteArray encode();
    quint8 cmd;
    quint8 para;

};


class SAMVOutputProtocal:public SAMVProtocal
{
public:
    SAMVOutputProtocal();
    virtual bool decode(const QByteArray &indata);
    virtual QByteArray encode();
    quint8 sw1;
    quint8 sw2;
    quint8 sw3;

};


#endif // SAMVPROTOCAL_H
