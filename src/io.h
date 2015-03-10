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

#ifndef QUSB_IO_H
#define QUSB_IO_H

#include <QIODevice>
#include "global.h"

namespace QUSB
{

class DeviceHandle;
class IOPrivate;

class QUSB_SHARED_EXPORT IO : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(IO)

protected:
    IOPrivate *d_ptr;
    IO(IOPrivate *d, QObject *parent = 0);

public:
    explicit IO(DeviceHandle *handle, QObject *parent = 0);
    virtual ~IO();

    virtual bool open(QIODevice::OpenMode openMode);
    virtual void close();

    // QIODevice interface
protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    bool isSequential() const;
    qint64 bytesToWrite() const;
    qint64 bytesAvailable() const;

};

}   // namespace QUSB

#endif // QUSB_IO_H
