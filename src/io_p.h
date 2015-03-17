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

#ifndef QUSB_IO_P_H
#define QUSB_IO_P_H

#include "clibusb.h"
#include <QtCore/QtGlobal>
#include <QEventLoop>
#include <QBuffer>
#include <QMutex>
struct libusb_transfer;
class QThread;

namespace QUSB
{

class Channel;
class IO;
class DeviceHandle;

class IOPrivate
{
    Q_DECLARE_PUBLIC(IO)

protected:
    IO *q_ptr;

public:
    IOPrivate(IO *q, DeviceHandle *handle);
    virtual ~IOPrivate();
    virtual qint64 read(char *data,qint64 maxlen);
    virtual qint64 write(const char * data,qint64 len);
    virtual bool startRead();
    virtual bool stopRead();
    virtual bool startWrite();
    virtual bool stopWrite();
    virtual void onTransferEvent(libusb_transfer *transfer);
    // Things to override.
    virtual libusb_transfer *alloc();
    virtual void fill(libusb_transfer *tran, int flag, uchar *buf, int len);

    bool submit(libusb_transfer *transfer);

    static void LIBUSB_CALL transferCallback(libusb_transfer *transfer);


    DeviceHandle *handle;
    libusb_transfer *readTransfer;
    libusb_transfer *writeTransfer;
    int endpointIn;
    int endpointOut;
    int maxSendingPacketSize;
    int maxReceivingPacketSize;

    QByteArray readBuffer;
    QBuffer readBytes;
    QMutex readMutex;

    QBuffer writeBytes;
    QByteArray currentWrite;
    QMutex writeMutex;

    int interfaceNumber;

};


}   // namespace QUSB

#endif // QUSB_IO_P_H
