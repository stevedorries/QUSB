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

#ifndef QUSB_HANDLE_H
#define QUSB_HANDLE_H

#include <QObject>
#include "global.h"
struct libusb_device_handle;

namespace QUSB
{

class Device;
class DeviceHandlePrivate;

class QUSB_SHARED_EXPORT DeviceHandle : public QObject
{
    Q_DECLARE_PRIVATE(DeviceHandle)
    DeviceHandlePrivate *d_ptr;

    Q_DISABLE_COPY(DeviceHandle)

    DeviceHandle(
        const Device &device, libusb_device_handle *rawhandle,
        QObject *parent = 0
    );
public:


    explicit DeviceHandle(const Device &device, QObject *parent = 0);
    ~DeviceHandle();
    Device * getDevice();

    int activeConfiguration() const;
    int setConfiguration(int config) const;
    int claimInterface(int num);
    int releaseInterface(int num);
    int setInterfaceAlternateSetting(int,int) const;


    bool InterfaceClaimed(int num);
    libusb_device_handle *rawhandle() const;

    static DeviceHandle *fromVendorIdProductId(quint16 vid, quint16 pid);

    QString stringDescriptor(quint32 index) const;
};

}   // namespace QUSB

#endif // QUSB_HANDLE_H
