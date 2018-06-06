#include "samvprotocal.h"

#include <QtEndian>
#include <QDebug>



/***
数据输入传输帧格式
Preamble(5 Bytes)	Len1(1 Byte)	Len2(1 Byte)	CMD(1 Byte)	Para(1 Byte)	Data(n Bytes)	CHK_SUM(1 Byte)

数据输出传输帧格式
Preamble(5 Bytes)	Len1(1 Byte)	Len2(1 Byte)	SW1(1 Byte)	SW2(1 Byte)	SW3(1 Byte)	Data(n Bytes)	CHK_SUM(1 Byte)

*/


const int kPreambleLen = 5;
static const char PreambleA[6] = {0xAA,0xAA,0xAA,0x96,0x69,0x00};
static const char PreambleB[6] = {0xAA,0xAA,0xAA,0x5A,0xA5,0x00};


quint8 calculateCheckSum(const QByteArray & data){
    quint8  check = data[0];
    for(int i = 1;i < data.size();i++){
        check ^= data[i];
    }
    return check;
}

SAMVProtocal::SAMVProtocal():
    type(SAMVProtocalType_UNDEFINE),length(0),check_sum(0)
{
}

SAMVProtocal::~SAMVProtocal()
{
}

int SAMVProtocal::check(const QByteArray &indata)
{
    if (indata.length() < kPreReadLen) {
        return kPreReadLen - indata.length();
    }
    if(indata.startsWith(PreambleA)){
        this->type = SAMVProtocalType_A;
    }else if(indata.startsWith(PreambleB)){
        this->type = SAMVProtocalType_B;
    }else{
//        qDebug("preamble error");
        return -1;
    }

    const uchar * lensrc =(const uchar *) (indata.data()+kPreambleLen);
    length = qFromBigEndian<quint16>(lensrc);
    if( length >= (indata.length() - kPreReadLen) ) {
        return ( length - (indata.length() - kPreReadLen) );
    }
    return 0;
}

bool SAMVProtocal::decode(const QByteArray &indata)
{
    return false;
}

QByteArray SAMVProtocal::encode()
{
    return QByteArray();
}





SAMVInputProtocal::SAMVInputProtocal():cmd(0),para(0)
{
    this->type = SAMVProtocalType_A;
}

bool SAMVInputProtocal::decode(const QByteArray &indata)
{
    const uchar * lensrc =(const uchar *) (indata.data()+kPreambleLen);
    length = qFromBigEndian<quint16>(lensrc);
    this->cmd = indata[kPreReadLen];
    this->para = indata[kPreReadLen + 1];
    this->data = indata.mid(kPreReadLen+2,length-3);
    this->check_sum = indata.at(kPreReadLen+length-1);

    //判断校验和是否正确
    quint8 tmpCheckSum = calculateCheckSum(
                indata.mid(kPreambleLen,length-1+2));
    if(this->check_sum != tmpCheckSum){
        qWarning("check sum error!!");
        return false;
    }

    return true;

}

QByteArray SAMVInputProtocal::encode()
{
    QByteArray tmpData;
    if(this->type != SAMVProtocalType_B ){
        tmpData.append(PreambleA,kPreambleLen);
    }else {
        tmpData.append(PreambleB,kPreambleLen);
    }
    length = 3 + data.length();

    quint16 big = qToBigEndian<quint16>(length);
    tmpData.append((char *)&big,sizeof(quint16));
    tmpData.append(cmd);
    tmpData.append(para);
    tmpData.append(data);

    this->check_sum = calculateCheckSum(tmpData.mid(kPreambleLen));

    tmpData.append(check_sum);
    return tmpData;

}


SAMVOutputProtocal::SAMVOutputProtocal():
    sw1(0),sw2(0),sw3(0)
{

}

bool SAMVOutputProtocal::decode(const QByteArray &indata)
{
    if(indata.startsWith(PreambleA)){
        this->type = SAMVProtocalType_A;
    }else if(indata.startsWith(PreambleB)){
        this->type = SAMVProtocalType_B;
    }else{
        qWarning("preamble error");
        return false;
    }

    const uchar * lensrc = (const uchar *)(indata.data()+kPreambleLen);
    length = qFromBigEndian<quint16>(lensrc);
    this->sw1 = indata[kPreReadLen];
    this->sw2 = indata[kPreReadLen + 1];
    this->sw3 = indata[kPreReadLen + 2];
    this->data = indata.mid(kPreReadLen+3,length-4);
    this->check_sum = indata.at(kPreReadLen+length-1);

    quint8 tmpCheckSum = calculateCheckSum(
                indata.mid(kPreambleLen,length-1+2));
    if(this->check_sum != tmpCheckSum){
        qWarning("check sum error!!");
        return false;
    }


    return true;

}

QByteArray SAMVOutputProtocal::encode()
{
    QByteArray tmpData;
    tmpData.append(PreambleA,kPreambleLen);
    length = 4 + data.length();

    quint16 big = qToBigEndian<quint16>(length);

    tmpData.append((char *)&big,sizeof(quint16));

    tmpData.append(sw1);
    tmpData.append(sw2);
    tmpData.append(sw3);
    tmpData.append(data);

    this->check_sum = calculateCheckSum(tmpData.mid(kPreambleLen));

    tmpData.append(check_sum);
    return tmpData;

}
