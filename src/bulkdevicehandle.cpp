#include "bulkdevicehandle.h"

#include <QDebug>

namespace QUSB
{

BulkDeviceHandle::BulkDeviceHandle(const Device &device,QObject *parent) :
    QIODevice(parent),readTransfer(0),readBuffer(64,0),
    endpointIn(130),endpointOut(1),maxSendingPacketSize(64),
    writeTransfer(0),device(device)
{
    int r = libusb_open(device.rawdevice(), &rawhandle);
    if (r){
        qWarning()<<"Unable to obtain device handle "<< r;
        throw r;
    }

}

BulkDeviceHandle::BulkDeviceHandle(const Device &device, libusb_device_handle *rawhandle, QObject *parent):
    QIODevice(parent),readTransfer(0),readBuffer(64,0),
    endpointIn(130),endpointOut(1),maxSendingPacketSize(64),
    writeTransfer(0),device(device),rawhandle(rawhandle)

{
}

BulkDeviceHandle::~BulkDeviceHandle()
{
    qDebug()<<"BulkDeviceHandle::~BulkDeviceHandle";
    if(this->isOpen()){
        this->close();
    }

    foreach (int num, claimedInterfaces)
        libusb_release_interface(rawhandle, num);
    libusb_close(rawhandle);

}

Device *BulkDeviceHandle::getDevice()
{
    return &device;
}

int BulkDeviceHandle::activeConfiguration() const
{
    int rc = 0;
    int config = 0;
    //This method returns a returncode and puts the data you actually
    //want in an int you pass by reference.
    rc = libusb_get_configuration(this->rawhandle,&config);
    switch(rc)
    {
    case 0:
        if(config != 0)
            return config;
        else
        {
            qWarning("The device is in an unconfigured state");
            return 0;
        }
    case LIBUSB_ERROR_NO_DEVICE:
        qWarning("The device has been disconnected");
        break;
    default:
        qWarning("An error has occured");
        break;
    }
    return rc;

}

int BulkDeviceHandle::setConfiguration(int config) const
{
    int rc = 0;
    rc = libusb_set_configuration(this->rawhandle, config);
    switch(rc)
    {
    case 0://success
        return 0;
    case LIBUSB_ERROR_NO_DEVICE:
        qWarning("The device has been disconnected");
        break;
    case LIBUSB_ERROR_BUSY:
        qWarning("The interface is already claimed");
        break;
    case LIBUSB_ERROR_NOT_FOUND:
        qWarning("The requested configuration does not exist");
        break;
    default:
        qWarning("An error has occured");
        break;
    }
    return rc;

}

int BulkDeviceHandle::claimInterface(int num)
{
    libusb_detach_kernel_driver(this->rawhandle,num);
    int r = libusb_claim_interface(this->rawhandle, num);
    if (r)
        qWarning("Failed to claim interface %d,error code %d", num,r);
    else
        this->claimedInterfaces.append(num);
    return r;

}

int BulkDeviceHandle::releaseInterface(int num)
{
    int r = libusb_release_interface(this->rawhandle, num);
    if (r)
        qWarning("Failed to release interface %d,ret %d", num,r);
    else{
        this->claimedInterfaces.removeOne(num);
        libusb_attach_kernel_driver(this->rawhandle,num);
    }
    return r;

}

int BulkDeviceHandle::setInterfaceAlternateSetting(int interfaceNumber, int alternateSetting) const
{
    int rc = libusb_set_interface_alt_setting(
                this->rawhandle, interfaceNumber, alternateSetting);
    switch(rc)
    {
    case 0://success
        return 0;
    case LIBUSB_ERROR_NOT_FOUND:
        qWarning("The interface is not claimed or the requested alternate setting does not exist");
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        qWarning("The device has been disconnected");
        break;
    default:
        qWarning("An error has occured");
    }
    return rc;

}

bool BulkDeviceHandle::isInterfaceClaimed(int num)
{
    foreach (int n, this->claimedInterfaces) {
        if(num == n) {
            return true;
        }
    }
    return false;

}

//libusb_device_handle *BulkDeviceHandle::rawhandle() const
//{
//    return this->rawhandle;
//}

BulkDeviceHandle *BulkDeviceHandle::fromVendorIdProductId(quint16 vid, quint16 pid)
{
    libusb_device_handle *rawhandle = libusb_open_device_with_vid_pid(
        Device::rawcontext(), vid, pid
    );
    if (!rawhandle){
        return 0;
    }
    libusb_device *rawdevice = libusb_get_device(rawhandle);
    Device device(rawdevice);
    return new BulkDeviceHandle(device, rawhandle);

}

QString BulkDeviceHandle::stringDescriptor(quint32 index) const
{
    const int bufferSize = 256;
    char buffer[bufferSize];
    int r = libusb_get_string_descriptor_ascii(
                this->rawhandle, index, reinterpret_cast<uchar *>(buffer),
                bufferSize);
    if (r < 0)  // TODO: Need to do something with the error code.
        qWarning("Error getting description");
    return QString::fromLatin1(buffer, bufferSize);

}

void BulkDeviceHandle::onTransferEvent(libusb_transfer *transfer)
{
    qDebug()<<"transfer -- status "<<transfer->status<<" ; endpoint "<<transfer->endpoint<<
              " ; type : "<<transfer->type<<" ; actual length  : "<<transfer->actual_length;

    if (transfer == this->readTransfer)
    {
        switch (transfer->status)
        {
        case LIBUSB_TRANSFER_ERROR:
        case LIBUSB_TRANSFER_NO_DEVICE:

        case LIBUSB_TRANSFER_CANCELLED:
            // Transfers are cancelled when they close. Do cleanup for it.
            // See also: IO::close()
            this->stopRead();
            break;
        case LIBUSB_TRANSFER_COMPLETED:{
            this->readMutex.lock();
            if(this->readBytes.atEnd())//everything has been read;the buffer is safe to reset
                this->readBytes.close();
            if(!this->readBytes.isOpen())
                this->readBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
            qint64 readPos = this->readBytes.pos();
            this->readBytes.seek(this->readBytes.size());
            Q_ASSERT(this->readBytes.atEnd());
            this->readBytes.write(reinterpret_cast<char *>(transfer->buffer),
                                 transfer->actual_length);
            this->readBytes.seek(readPos);
            this->readMutex.unlock();

            int r = libusb_submit_transfer(transfer);
            if (r) {
                this->stopRead();
            }else{//submit success
                emit this->readyRead();
            }

            break;
        }
        default:
            qWarning("IOPrivate::onTransferEvent---not implement read transfer status %d",transfer->status);
            // TODO: Implement the rest of the API
            break;
        }
    }else if(transfer == this->writeTransfer){
        switch (transfer->status)
        {
        case LIBUSB_TRANSFER_NO_DEVICE:

        case LIBUSB_TRANSFER_CANCELLED:
            // Transfers are cancelled when they close. Do cleanup for it.
            // See also: IO::close()
            this->stopWrite();
            break;
        case LIBUSB_TRANSFER_COMPLETED:{
            emit this->bytesWritten(transfer->actual_length);

            QMutexLocker mutexLocker(&this->writeMutex);
            if(this->writeBytes.atEnd()){
                qDebug()<<"write all bytes and stop write";
                this->stopWrite();
                return;
            }
            this->currentWrite = this->writeBytes.read(this->maxSendingPacketSize);

            libusb_fill_bulk_transfer(
                transfer,
                this->rawhandle,
                LIBUSB_ENDPOINT_OUT|this->endpointOut,
                reinterpret_cast<uchar *>(this->currentWrite.data()),
                this->currentWrite.length(),
                BulkDeviceHandle::transferCallback,
                this,
                0
            );

            int r = libusb_submit_transfer(transfer);
            if (r)   {
                qWarning()<<"IOPrivate::transferCallback---write submit error";
                this->stopWrite();
            }
            break;
        }
        default:
            // TODO: Implement the rest of the API
            qWarning("IOPrivate::onTransferEvent---not implement write transfer status %d",transfer->status);
            break;
        }

    }

}

bool BulkDeviceHandle::startRead()
{
    this->readTransfer = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(
        readTransfer,
        this->rawhandle,
        LIBUSB_ENDPOINT_IN|this->endpointIn,
        reinterpret_cast<uchar *>(this->readBuffer.data()),
        this->readBuffer.length(),
        BulkDeviceHandle::transferCallback,
        this,
        0
    );

    int r = libusb_submit_transfer(readTransfer);   // Continue reading.
    if (r)
    {
        qWarning("BulkDeviceHandle::open---failed %d",r);
        libusb_free_transfer(this->readTransfer);
        this->readTransfer = 0;
        return false;
    }
    return true;
}

bool BulkDeviceHandle::stopRead()
{
    qDebug()<<"BulkDeviceHandle::stopRead";
    libusb_free_transfer(this->readTransfer);
    this->readTransfer = 0 ;
    emit this->readChannelFinished();
    this->readBytes.close();
    return true;

}

bool BulkDeviceHandle::startWrite()
{

    this->writeTransfer = libusb_alloc_transfer(0);
    this->currentWrite = this->writeBytes.read(this->maxSendingPacketSize);

    while(this->currentWrite.size() < this->maxSendingPacketSize){
        this->currentWrite.append((char )0);
    }
    qDebug()<<"write data "<<currentWrite.toHex();

    libusb_fill_bulk_transfer(
        this->writeTransfer,
        this->rawhandle,
        LIBUSB_ENDPOINT_OUT|this->endpointOut,
        reinterpret_cast<uchar *>(this->currentWrite.data()),
        this->currentWrite.length(),
        BulkDeviceHandle::transferCallback,
        this,
        0
    );

    int r = libusb_submit_transfer(this->writeTransfer);
    if (r)
    {
        this->writeBytes.close();
        libusb_free_transfer(this->writeTransfer);
        this->writeTransfer = 0;
        return false;
    }
    return true;
}

bool BulkDeviceHandle::stopWrite()
{
    this->writeBytes.close();
    libusb_free_transfer(this->writeTransfer);
    this->writeTransfer = 0;
    this->writeBytes.close();

    return true;
}

void BulkDeviceHandle::transferCallback(libusb_transfer *transfer)
{
//    qDebug()<<"IOPrivate::transferCallback ---transfer " <<transfer<<"status: "<<transfer->status<<
//              " ; flag: "<< transfer->flags<<" ; endpoint : "<<transfer->endpoint<<
//             " ; type : "<<transfer->type<<".";
    if (!transfer || !transfer->user_data){
        qWarning()<<"IOPrivate::transferCallback --- receive null data";
        return;
    }
    BulkDeviceHandle *obj = reinterpret_cast<BulkDeviceHandle *>(transfer->user_data);
    obj->onTransferEvent(transfer);

}

bool BulkDeviceHandle::isSequential() const
{
    return true;
}

bool BulkDeviceHandle::open(QIODevice::OpenMode mode)
{

    if(!this->startRead()){
        return false;
    }
    QIODevice::open(mode);

    return true;

}

void BulkDeviceHandle::close()
{
    qDebug()<<"close device io";
    if(this->readTransfer){
        if (!this->readTransfer->dev_handle)
        {
            // Nothing to transfer. We can clean up now safely.
            this->stopRead();
        }else{
            qDebug("BulkDeviceHandle::close---cancel tranfer");
            // Needs to cancel the transfer first. Cleanup happens in the
            // callback. (IOPrivate::transferCallback)
            libusb_cancel_transfer(this->readTransfer);
        }
    }

    if(this->writeTransfer){
        if (!this->writeTransfer->dev_handle)
        {
            // Nothing to transfer. We can clean up now safely.
            this->stopWrite();
        }else{
            libusb_cancel_transfer(this->writeTransfer);
        }
    }
    QIODevice::close();

}

qint64 BulkDeviceHandle::bytesAvailable() const
{
    return QIODevice::bytesAvailable()+this->readBytes.bytesAvailable();

}

qint64 BulkDeviceHandle::bytesToWrite() const
{
    return  this->writeBytes.bytesAvailable()+ QIODevice::bytesToWrite();

}

qint64 BulkDeviceHandle::readData(char *data, qint64 maxlen)
{
    QMutexLocker mutexLocker(&this->readMutex);
    return this->readBytes.read(data,maxlen);

}

qint64 BulkDeviceHandle::writeData(const char *data, qint64 len)
{
    QMutexLocker mutexLocker(&this->writeMutex);
    if(this->writeBytes.atEnd()){
        this->writeBytes.close();
    }
    if(!this->writeBytes.isOpen()){
        this->writeBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
    }
    qint64 readPos = this->writeBytes.pos();
    this->writeBytes.seek(this->writeBytes.size());
    Q_ASSERT(this->writeBytes.atEnd());
    this->writeBytes.write(data,len);
    this->writeBytes.seek(readPos);

    if(!this->writeTransfer   && !this->startWrite() ){
       return -1;
    }
    return len;

}

}   // namespace QUSB
