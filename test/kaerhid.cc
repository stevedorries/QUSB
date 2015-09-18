#include "kaerhid.h"


//KaerHid::KaerHid()
//{
//}
extern "C" {
#include <libusb-1.0/libusb.h>
}

#include "../../SAMVServer/samvprotocal.h"

#include <QByteArray>
#include <QDebug>
#include <QtEndian>

const int endpointIn = 130;
const int endpointOut = 1;
const int packetSize = 64;
const int vid = 0x0483;
const int pid = 0x5750;


int readSamvId(libusb_device_handle *rawhandle,char * buffer){
    SAMVInputProtocal protocal;
    protocal.cmd = CMD_READSAMVNUMBER;
    protocal.para = 0xFF;

    QByteArray data = protocal.encode();
    while(data.size() < packetSize){
        data.append((char )0);
    }

    int transfered;
    int r = libusb_bulk_transfer(rawhandle,endpointOut,(unsigned char *)data.data(),packetSize,&transfered,1000);
    if(r != 0){
        qDebug()<<"write fails "<<r;
        return r;
    }


    r = libusb_bulk_transfer(rawhandle,LIBUSB_ENDPOINT_IN|endpointIn,(unsigned char *)data.data(),packetSize,&transfered,1000);
    if(r !=  0){
        qDebug()<<"read fails "<<r;
        return r;
    }


    qWarning()<<"local read "<<data.toHex();
    SAMVOutputProtocal recvProtocal;
    if(!recvProtocal.decode(data)){
        qWarning()<<"decode remote command error!";
        return r;
    }
//    quint32  samvNum;
    if(recvProtocal.type == SAMVProtocalType_A){
        if(recvProtocal.sw3 == 0x90){
            qDebug()<<"size = "<<recvProtocal.data.size();
            qDebug()<<"data = "<<recvProtocal.data;
            memcpy(buffer,recvProtocal.data,recvProtocal.data.size());
            //*(buffer + recvProtocal.data.size()) = 0;
//            return QByteArray(recvProtocal.data);
//            QByteArray numData =  recvProtocal.data.right(4);
//            const uchar * src = (const uchar *) numData.data();
//            samvNum  = qFromLittleEndian<quint32>(src);
        }
    }

    return r;

}

int testRead(libusb_device_handle *rawhandle,char * buffer){

    SAMVInputProtocal protocal;
    protocal.type = SAMVProtocalType_B;
    protocal.cmd = 0x05;
    protocal.para = 0x00;
    protocal.data = QByteArray(1,'\0');

    QByteArray data = protocal.encode();
    while(data.size() < packetSize){
        data.append((char )0);
    }
    qDebug()<<" write date "<<data.toHex();
    int transfered;
    int r = libusb_bulk_transfer(rawhandle,endpointOut,(unsigned char *)data.data(),packetSize,&transfered,1000);
    if(r != 0){
        qDebug()<<"write fails "<<r;
        return r;
    }


    r = libusb_bulk_transfer(rawhandle,LIBUSB_ENDPOINT_IN|endpointIn,(unsigned char *)data.data(),packetSize,&transfered,1000);
    if(r !=  0){
        qDebug()<<"read fails "<<r;
        return r;
    }


    qWarning()<<"local read "<<data.toHex();
    SAMVOutputProtocal recvProtocal;
    if(!recvProtocal.decode(data)){
        qWarning()<<"decode remote command error!";
        return r;
    }
//    if(recvProtocal.type == SAMVProtocalType_A){
//        if(recvProtocal.sw3 == 0x90){
//            qDebug()<<"size = "<<recvProtocal.data.size();
//            qDebug()<<"data = "<<recvProtocal.data;
//            if(buffer != NULL){
//                memcpy(buffer,recvProtocal.data,recvProtocal.data.size());
//            }
//        }
//    }

    return r;


}

int getSamvId(char * samvBuf)
{
    int  r = libusb_init(NULL);
    if (r < 0){
        qCritical()<<"init error";
        return r;
    }
    qDebug()<<"getSamvId";
    libusb_device_handle *rawhandle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (!rawhandle){
        qCritical()<<"open device error";
        libusb_exit(NULL);
        return r;
    }

    libusb_set_auto_detach_kernel_driver(rawhandle,1);
    r = libusb_claim_interface(rawhandle, 0);
    if (r){
        qCritical("Failed to claim interface %d,error code %d", 0,r);
        libusb_exit(NULL);
        return r;
    }


    r = readSamvId(rawhandle,samvBuf);
    libusb_release_interface(rawhandle, 0);
    libusb_close(rawhandle);
    libusb_exit(NULL);

    return r;
}


int test()
{
    int  r = libusb_init(NULL);
    if (r < 0){
        qCritical()<<"init error";
        return r;
    }
    qDebug()<<"open";
    libusb_device_handle *rawhandle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (!rawhandle){
        qCritical()<<"open device error";
        libusb_exit(NULL);
        return -1;
    }

    libusb_set_auto_detach_kernel_driver(rawhandle,1);
    r = libusb_claim_interface(rawhandle, 0);
    if (r){
        qCritical("Failed to claim interface %d,error code %d", 0,r);
        libusb_exit(NULL);
        return r;
    }


    r = testRead(rawhandle,NULL);
    libusb_release_interface(rawhandle, 0);
    libusb_close(rawhandle);
    libusb_exit(NULL);

    return r;

}
