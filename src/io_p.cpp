/******************************************************************************
**
** Copyright (C) 2014 BIMEtek Co. Ltd.
**
** This file is part of QUSB.
**
** QUSB is free software: you can redistribute it and/or modify it under the
** terms of the GNU Lesser General Public License as published by the Free
** Software Foundation, either version 3 of the License, or (at your option)
** any later version.
**
** QUSB is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this file. If not, see <http://www.gnu.org/licenses/>.
**
******************************************************************************/

#include "io.h"
#include "io_p.h"
#include <QDebug>
#include "handle.h"
namespace QUSB
{

IOPrivate::IOPrivate(IO *q, DeviceHandle *handle) :
    q_ptr(q), handle(handle), readTransfer(0),readBuffer(64,0),
    endpointIn(130),endpointOut(1),maxSendingPacketSize(64),
    writeTransfer(0),interfaceNumber(0)
{

}

IOPrivate::~IOPrivate()
{
//    if (this->readTransfer && this->readTransfer->buffer)
    //        delete [] this->readTransfer->buffer;
}


qint64 IOPrivate::read(char *data, qint64 maxlen)
{
    QMutexLocker mutexLocker(&this->readMutex);
    return this->readBytes.read(data,maxlen);
}

qint64 IOPrivate::write(const char *data, qint64 len)
{
    qDebug()<<"IOPrivate::write --- "<<len;
    QMutexLocker mutexLocker(&this->writeMutex);
    if(this->writeBytes.atEnd()){
        qDebug()<<" IOPrivate::write ---write bytes end";
        this->writeBytes.close();
    }
    if(!this->writeBytes.isOpen()){
        qDebug()<<" oepn write buffer";
        this->writeBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
    }
    qint64 readPos = this->writeBytes.pos();
    qDebug()<<"IOPrivate::write---buffer read pos "<<readPos;
    this->writeBytes.seek(this->writeBytes.size());
    Q_ASSERT(this->writeBytes.atEnd());
    this->writeBytes.write(data,len);
    this->writeBytes.seek(readPos);

    if(!this->writeTransfer ){
        qDebug()<<"IOPrivate::write---write data";
        this->writeTransfer = this->alloc();
        this->currentWrite = this->writeBytes.read(this->maxSendingPacketSize);
        this->fill(writeTransfer,LIBUSB_ENDPOINT_OUT|endpointOut,
                   reinterpret_cast<uchar *>(currentWrite.data()),
                   currentWrite.length());

        if(!this->submit(writeTransfer)){
            this->writeBytes.close();
            libusb_free_transfer(this->writeTransfer);
            this->writeTransfer = 0;
            return -1;
        }
    }
    return len;
}

bool IOPrivate::startRead()
{
    this->readTransfer = this->alloc();
    this->fill(this->readTransfer, LIBUSB_ENDPOINT_IN|this->endpointIn,
               reinterpret_cast<uchar *>(this->readBuffer.data()),
               this->readBuffer.length());
    bool ok = this->submit(this->readTransfer);
    if (!ok)
    {
        libusb_free_transfer(this->readTransfer);
        this->readTransfer = 0;
        return false;
    }
    return true;

}

bool IOPrivate::stopRead()
{
    Q_Q(IO);
    qDebug("IOPrivate::stopRead---stop read");
    emit q->readChannelFinished();
    libusb_free_transfer(this->readTransfer);
    this->readTransfer = 0 ;
    return true;
}

bool IOPrivate::startWrite()
{
    return true;
}

bool IOPrivate::stopWrite()
{
    this->writeBytes.close();
    libusb_free_transfer(this->writeTransfer);
    this->writeTransfer = 0;
    return true;
}

void IOPrivate::onTransferEvent(libusb_transfer *transfer)
{
    Q_Q(IO);
    qDebug()<<"IOPrivate::onTransferEvent --- ";

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

            if (!this->submit(transfer))  {
                this->stopRead();
            }else{//submit success
                emit q->readyRead();
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
            emit q->bytesWritten(transfer->actual_length);

            QMutexLocker mutexLocker(&this->writeMutex);
            if(this->writeBytes.atEnd()){
                qDebug()<<"write all bytes and stop write";
                this->stopWrite();
                return;
            }
            this->currentWrite = this->writeBytes.read(this->maxSendingPacketSize);
            this->fill(transfer,LIBUSB_ENDPOINT_OUT|this->endpointOut,
                      reinterpret_cast<uchar *>(this->currentWrite.data()),
                      this->currentWrite.length());

            if (!this->submit(transfer))   {  // Continue write
                qWarning()<<"IOPrivate::transferCallback---write submit error";
                this->stopWrite();
            }

        }
            break;
        default:
            // TODO: Implement the rest of the API
            qWarning("IOPrivate::onTransferEvent---not implement write transfer status %d",transfer->status);

            break;
        }

    }

}

void IOPrivate::transferCallback(libusb_transfer *transfer)
{
    qDebug()<<"IOPrivate::transferCallback ---transfer " <<transfer<<"status: "<<transfer->status<<
              " ; flag: "<< transfer->flags<<" ; endpoint : "<<transfer->endpoint<<
             " ; type : "<<transfer->type<<".";
    if (!transfer || !transfer->user_data){
        qWarning()<<"IOPrivate::transferCallback --- receive null data";
        return;
    }
    IOPrivate *obj = reinterpret_cast<IOPrivate *>(transfer->user_data);
    obj->onTransferEvent(transfer);
}

libusb_transfer *IOPrivate::alloc()
{
    return libusb_alloc_transfer(0);
}

void IOPrivate::fill(libusb_transfer *tran, int flag, uchar *buf, int len)
{

}


bool IOPrivate::submit(libusb_transfer *transfer)
{
    qDebug()<<"IOPrivate::submit---submit transfer "<<transfer->length;
    int r = libusb_submit_transfer(transfer);   // Continue reading.
    if (r)
    {
        qWarning("IOPrivate::submit---failed %d",r);
        //        if (transfer && transfer->buffer)
        //            delete [] transfer->buffer;
//        libusb_free_transfer(transfer);
    }
    return (r == 0);
}

}   // namespace QUSB
