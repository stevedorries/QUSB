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

#include "clibusb.h"
#include "device.h"
#include "handle.h"
#include <QDebug>

namespace QUSB
{

class DeviceHandlePrivate
{
    Q_DECLARE_PUBLIC(DeviceHandle)
    DeviceHandle *q_ptr;

public:
    DeviceHandlePrivate(
            DeviceHandle *q, const Device &device, libusb_device_handle *rawhandle);
    virtual ~DeviceHandlePrivate();

    libusb_device_handle *rawhandle;
    Device device;
    QList<int> claimedInterfaces;
};

DeviceHandlePrivate::DeviceHandlePrivate(
        DeviceHandle *q, const Device &device, libusb_device_handle *rawhandle) :
    q_ptr(q), rawhandle(rawhandle), device(device) {


}

DeviceHandlePrivate::~DeviceHandlePrivate()
{
    foreach (int num, claimedInterfaces)
        libusb_release_interface(rawhandle, num);
    libusb_close(rawhandle);
}


DeviceHandle::DeviceHandle(const Device &device, libusb_device_handle *rawhandle,
        QObject *parent) :
    QObject(parent), d_ptr(new DeviceHandlePrivate(this, device, rawhandle))
{
}

libusb_device_handle *DeviceHandle::rawhandle() const
{
    return d_func()->rawhandle;
}

DeviceHandle::DeviceHandle(const Device &device, QObject *parent) :
    QObject(parent), d_ptr(new DeviceHandlePrivate(this, device, 0))
{
    Q_D(DeviceHandle);
    int r = libusb_open(device.rawdevice(), &d->rawhandle);
    if (r){
        delete d_ptr;
        qWarning()<<"Unable to obtain device handle "<<r;
        throw r;
    }

}

DeviceHandle::~DeviceHandle()
{
    qDebug("DeviceHandle::~DeviceHandle");
    delete d_ptr;
}

Device *DeviceHandle::getDevice()
{
    Q_D(DeviceHandle);
    return &d->device;
}

int DeviceHandle::activeConfiguration() const
{
    int rc = 0;
    int config = 0;
    //This method returns a returncode and puts the data you actually
    //want in an int you pass by reference.
    rc = libusb_get_configuration(d_ptr->rawhandle,&config);
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

int DeviceHandle::setConfiguration(int config) const
{
    int rc = 0;
    rc = libusb_set_configuration(d_ptr->rawhandle, config);
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

bool DeviceHandle::InterfaceClaimed(int num)
{
    Q_D(DeviceHandle);

    foreach (int n, d->claimedInterfaces) {
        if(num == n) {
            return true;
        }
    }
    return false;
}



int DeviceHandle::claimInterface(int num)
{
    Q_D(DeviceHandle);
    libusb_detach_kernel_driver(d->rawhandle,num);
    int r = libusb_claim_interface(d_ptr->rawhandle, num);
    if (r)
        qWarning("Failed to claim interface %d,error code %d", num,r);
    else
        d_ptr->claimedInterfaces.append(num);
    return r;
}

int DeviceHandle::releaseInterface(int num)
{
    Q_D(DeviceHandle);
    int r = libusb_release_interface(d->rawhandle, num);
    if (r)
        qWarning("Failed to release interface %d", num);
    else{
        d->claimedInterfaces.removeOne(num);

        libusb_attach_kernel_driver(d->rawhandle,num);
    }
    return r;
}

int DeviceHandle::setInterfaceAlternateSetting(
        int interfaceNumber, int alternateSetting) const
{
    int rc = libusb_set_interface_alt_setting(
                d_ptr->rawhandle, interfaceNumber, alternateSetting);
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

DeviceHandle *DeviceHandle::fromVendorIdProductId(quint16 vid, quint16 pid)
{
    libusb_device_handle *rawhandle = libusb_open_device_with_vid_pid(
        Device::rawcontext(), vid, pid
    );
    if (!rawhandle){
        return 0;
    }
    libusb_device *rawdevice = libusb_get_device(rawhandle);
    Device device(rawdevice);
    return new DeviceHandle(device, rawhandle);
}

QString DeviceHandle::stringDescriptor(quint32 index) const
{
    const int bufferSize = 256;
    char buffer[bufferSize];
    int r = libusb_get_string_descriptor_ascii(
                rawhandle(), index, reinterpret_cast<uchar *>(buffer),
                bufferSize);
    if (r < 0)  // TODO: Need to do something with the error code.
        qWarning("Error getting description");
    //return QString::fromLatin1(buffer, bufferSize);
    return QString::fromLatin1(buffer, r);
}

}   // namespace QUSB
