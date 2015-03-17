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

#include <QThread>
#include <QDebug>

#include "clibusb.h"
#include "io.h"
#include "io_p.h"
#include "handle.h"

namespace QUSB
{

IO::IO(DeviceHandle *handle,  QObject *parent) :
    QIODevice(parent), d_ptr(new IOPrivate(this, handle))
{
    Q_D(IO);

}

IO::IO(IOPrivate *d, QObject *parent) :
    QIODevice(parent), d_ptr(d)
{
}

IO::~IO()
{
    Q_D(IO);
    if(isOpen()){
        this->close();
    }
    delete d_ptr;
}

DeviceHandle *IO::getDeviceHandle()
{
    Q_D(IO);

    return d->handle;
}

bool IO::open(QIODevice::OpenMode openMode)
{
    Q_D(IO);
    //    Q_UNUSED(openMode);

    //    if(d->handle->claimInterface(d->interfaceNumber) != 0){
    //        return false;
    //    }

    // TODO: Implement open mode. This is a read-only implementation.
    d->readTransfer = d->alloc();
    d->fill(d->readTransfer, LIBUSB_ENDPOINT_IN|d->endpointIn,
            reinterpret_cast<uchar *>(d->readBuffer.data()),
            d->readBuffer.length());
    if (!d->submit(d->readTransfer))
    {
        libusb_free_transfer(d->readTransfer);
        d->readTransfer = 0;
        return false;
    }

    QIODevice::open(openMode);

    return true;
}

void IO::close()
{
    Q_D(IO);

    if(d->readTransfer){
        if (!d->readTransfer->dev_handle)
        {
            // Nothing to transfer. We can clean up now safely.
            d->stopRead();
        }else{
            qDebug("IO::close---cancel tranfer");
            // Needs to cancel the transfer first. Cleanup happens in the
            // callback. (IOPrivate::transferCallback)
            libusb_cancel_transfer(d->readTransfer);
        }
    }

    if(d->writeTransfer){
        if (!d->writeTransfer->dev_handle)
        {
            // Nothing to transfer. We can clean up now safely.
            d->stopWrite();
        }else{
            libusb_cancel_transfer(d->writeTransfer);
        }

    }
    QIODevice::close();
}

qint64 IO::readData(char *data, qint64 maxlen)
{
    Q_D(IO);
    QMutexLocker mutexLocker(&d->readMutex);
    return d->readBytes.read(data,maxlen);

    //    return d->read(data,maxlen);
}

qint64 IO::writeData(const char *data, qint64 len)
{
    Q_D(IO);
    //    return d->write(data,len);

    qDebug()<<"IOPrivate::write --- "<<len;
    QMutexLocker mutexLocker(&d->writeMutex);
    if(d->writeBytes.atEnd()){
        qDebug()<<" IOPrivate::write ---write bytes end";
        d->writeBytes.close();
    }
    if(!d->writeBytes.isOpen()){
        qDebug()<<" oepn write buffer";
        d->writeBytes.open(QBuffer::ReadWrite|QBuffer::Truncate);
    }
    qint64 readPos = d->writeBytes.pos();
    qDebug()<<"IOPrivate::write---buffer read pos "<<readPos;
    d->writeBytes.seek(d->writeBytes.size());
    Q_ASSERT(d->writeBytes.atEnd());
    d->writeBytes.write(data,len);
    d->writeBytes.seek(readPos);

    if(!d->writeTransfer ){
        qDebug()<<"IOPrivate::write---write data";
        d->writeTransfer = d->alloc();
        d->currentWrite = d->writeBytes.read(d->maxSendingPacketSize);
        d->fill(d->writeTransfer,LIBUSB_ENDPOINT_OUT|d->endpointOut,
                reinterpret_cast<uchar *>(d->currentWrite.data()),
                d->currentWrite.length());

        if(!d->submit(d->writeTransfer)){
            d->writeBytes.close();
            libusb_free_transfer(d->writeTransfer);
            d->writeTransfer = 0;
            return -1;
        }
    }
    return len;

}

bool IO::isSequential() const
{
    return true;
}

qint64 IO::bytesToWrite() const
{
    Q_D(const IO);
    return  d->writeBytes.bytesAvailable();
}

qint64 IO::bytesAvailable() const
{
    Q_D(const IO);
    return d->readBytes.bytesAvailable();

}

}   // namespace QUSB
