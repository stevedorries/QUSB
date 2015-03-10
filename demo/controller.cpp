#include <QDebug>
#include <QIODevice>
#include "controller.h"
#include "device.h"
#include <QTimer>
#include <QTimerEvent>

Controller::Controller(QObject *parent) :
    QObject(parent), handle(0), io(0), count(0)
{
    this->startTimer(10000);
    const quint16 vendor = 0x0483;
    const quint16 product = 0x5750;

    QUSB::setDebugLevel(3);
    QList<QUSB::Device> devices = QUSB::Device::availableDevices();
    foreach (const QUSB::Device &device, devices)
    {
        qDebug()<<"Got device at:"<<device.bus()<<":"<<device.address();
        qDebug()<<"Device info:";
        qDebug()<<"\t vid "<<device.vendorId()<<"\tProduct ID"<<device.productId();

        if(device.vendorId() != vendor || device.productId() != product){
                continue;
        }

//        handle = QUSB::DeviceHandle::fromVendorIdProductId(
//                    device.vendorId(), device.productId());
        handle = new QUSB::DeviceHandle(device,this);
        if (handle)
        {
            qDebug() << "\tProduct description" <<
                        handle->stringDescriptor(device.product());
            qDebug() << "\tManufacturer" <<
                        handle->stringDescriptor(device.manufacturer());
            handle->getDevice()->DeviceDescription();
            handle->setParent(this);
            if(handle->claimInterface(0) == 0){
                io = new QUSB::BulkIO(handle, 3, this);
            }
        }
        else
        {
            qDebug() << QString("Could not open device with ID: 0x%1, 0x%2")
                        .arg(vendor, 4, 16, QChar('0'))
                        .arg(product, 4, 16, QChar('0'));
        }

        if (io)
        {
            io->open(QIODevice::ReadWrite);
            connect(io, SIGNAL(readyRead()),
                    this, SLOT(processBytes()));
            connect(io, SIGNAL(bytesWritten(qint64)),
                    this, SLOT(writtenBytes(qint64)));

    //        QByteArray writeData(130,0);
    //        io->write(writeData);
        }

    }
    // Modify these to suit your needs.
//    handle = QUSB::DeviceHandle::fromVendorIdProductId(vendor, product);
//    if (handle)
//    {
//        handle->getDevice()->DeviceDescription();
//        handle->setParent(this);
//        if(handle->claimInterface(0) == 0){
//            io = new QUSB::BulkIO(handle, 3, this);
//        }
//    }
//    else
//    {
//        qDebug() << QString("Could not open device with ID: 0x%1, 0x%2")
//                    .arg(vendor, 4, 16, QChar('0'))
//                    .arg(product, 4, 16, QChar('0'));
//    }

//    if (io)
//    {
//        io->open(QIODevice::ReadWrite);
//        connect(io, SIGNAL(readyRead()),
//                this, SLOT(processBytes()));
//        connect(io, SIGNAL(bytesWritten(qint64)),
//                this, SLOT(writtenBytes(qint64)));

////        QByteArray writeData(130,0);
////        io->write(writeData);
//    }
}

Controller::~Controller()
{
    io->close();
}

void Controller::processBytes()
{
    qDebug()<<"receive signal form "<<this->sender();
    QByteArray bytes = io->readAll();
    qDebug()<<"read bytes "<<bytes.length();
    QDebug dbg = qDebug();
    for (int i = 0; i < bytes.length(); i++){
//        dbg << static_cast<int>(bytes[i])<<
        dbg<<QString::number(static_cast<uint>(bytes[i]),16);
    }
    count++;
    if (count >= 4)
        this->deleteLater();
}

void Controller::writtenBytes(qint64 bytes)
{
    qDebug()<<"Controller::writtenBytes---bytes written "<<bytes;
    io->write("1234567890");
    count++;
    if (count >= 4)
        this->deleteLater();

}

void Controller::timerEvent(QTimerEvent *event)
{
    qDebug() << "Timer ID:" << event->timerId();

}
