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

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include "clibusb.h"
#include "device.h"
#include "eventhandler.h"
#include "handle.h"


namespace QUSB
{

class DevicePrivate
{
    Q_DECLARE_PUBLIC(Device)
    Device *q_ptr;

public:
    DevicePrivate(Device *q, libusb_device *rawdevice);
    virtual ~DevicePrivate();

    libusb_device *rawdevice;
};

DevicePrivate::DevicePrivate(Device *q, libusb_device *rawdevice) :
    q_ptr(q), rawdevice(rawdevice)
{
    libusb_ref_device(rawdevice);
}

DevicePrivate::~DevicePrivate()
{
    libusb_unref_device(rawdevice);
}


Device::Device(libusb_device *rd, QObject *parent) :
   QObject(parent), d_ptr(new DevicePrivate(this, rd))
{
}

libusb_device *Device::rawdevice() const
{
    return d_func()->rawdevice;
}

libusb_context *Device::rawcontext()
{
    static QMutex mutex;
    static libusb_context *context = 0;
    static EventHandler *handler = 0;
    static Destroyer *destroyer = 0;
    if (!context)
    {
        mutex.lock();
        if (!context)
        {
            int r = libusb_init(&context);
            if (r)
                qWarning("Failed to initiaize LibUSB");
            if (context)
            {
                handler = new EventHandler(context);
                QThread *thread = new QThread();
                handler->moveToThread(thread);
                thread->start(QThread::TimeCriticalPriority);

                destroyer = new Destroyer(thread, handler);
                QObject::connect(
                    qApp, SIGNAL(aboutToQuit()),
                    destroyer, SLOT(deleteLater())
                );
            }
        }
        mutex.unlock();
    }
    return context;
}

Device::Device(const Device &d, QObject *parent) :
   QObject(parent), d_ptr(new DevicePrivate(this, d.rawdevice()))
{
}

Device::~Device()
{
    delete d_ptr;
}


quint8 Device::bus() const
{
    return libusb_get_bus_number(d_ptr->rawdevice);
}

quint8 Device::address() const
{
    return libusb_get_device_address(d_ptr->rawdevice);
}

Device::Speed Device::speed() const
{
    int rsp = libusb_get_device_speed(d_ptr->rawdevice);
    Speed sp = SpeedUnknown;
    switch (rsp)
    {
    case LIBUSB_SPEED_LOW:
        sp = SpeedLow;
        break;
    case LIBUSB_SPEED_FULL:
        sp = SpeedLow;
        break;
    case LIBUSB_SPEED_HIGH:
        sp = SpeedLow;
        break;
    case LIBUSB_SPEED_SUPER:
        sp = SpeedLow;
        break;
    default:
        break;
    }
    return sp;
}

int Device::maximumPacketSize(uchar endpoint) const
{
    return libusb_get_max_packet_size(d_ptr->rawdevice, endpoint);
}

int Device::maximumIsoPacketSize(uchar endpoint) const
{
    return libusb_get_max_iso_packet_size(d_ptr->rawdevice, endpoint);
}


qint32 Device::vendorId() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.idVendor;
}

qint32 Device::productId() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.idProduct;
}

qint16 Device::product() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.iProduct;
}

qint16 Device::manufacturer() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.iManufacturer;
}

qint16 Device::serialNumber() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.iSerialNumber;
}

qint16 Device::deviceClass() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.bDeviceClass;
}

qint16 Device::deviceSubClass() const
{
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d_ptr->rawdevice, &desc);
    if (r)
        return -1;
    return desc.bDeviceSubClass;
}

void Device::DeviceDescription()
{
    Q_D(Device);
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(d->rawdevice, &desc);
    if (r)
        return ;
    qDebug("dev config num %d",desc.bNumConfigurations);
    struct libusb_config_descriptor *config;
    r = libusb_get_active_config_descriptor(d->rawdevice,&config);
    if(r){
        return ;
    }
    qDebug("dev config interface number %d",config->bNumInterfaces);
    for(int i = 0;i<config->bNumInterfaces;i++){
        const struct libusb_interface * inter = &config->interface[i];
        qDebug("number of alternate settings : %d",inter->num_altsetting);
        for(int j = 0;j<inter->num_altsetting;j++){
            const struct libusb_interface_descriptor * interdesc = &inter->altsetting[j];
            qDebug("interface number %d;endpoint number %d;interface class %d",
                   interdesc->bInterfaceNumber,interdesc->bNumEndpoints,interdesc->bInterfaceClass);
            for(int k=0;k<interdesc->bNumEndpoints;k++){
                const struct libusb_endpoint_descriptor * epdesc = &interdesc->endpoint[k];
                qDebug("descriptor type :%d;ep addr %d;attributes %d;max packet size %d",
                       epdesc->bDescriptorType,epdesc->bEndpointAddress,
                       epdesc->bmAttributes,epdesc->wMaxPacketSize);
            }
        }
    }
    libusb_free_config_descriptor(config);
}

Device &Device::operator=(const Device &d)
{
    this->d_ptr->rawdevice = d.d_ptr->rawdevice;
    return *this;
}

bool Device::operator ==(const Device &d)
{
    return     (this->rawdevice() == d.rawdevice());

}

QList<Device> Device::availableDevices()
{
    libusb_context *context = Device::rawcontext();
    libusb_device **deviceArray = 0;

    ssize_t deviceCount = libusb_get_device_list(context, &deviceArray);
    QList<Device> devices;
    for (ssize_t i = 0; i < deviceCount; i++)
        devices.append(Device(deviceArray[i]));
    libusb_free_device_list(
        deviceArray,
        1   // Deref the device instances because DevicePrivate holds it.
    );

    return devices;
}

void setDebugLevel(int level)
{
    libusb_set_debug(Device::rawcontext(), level);
}

}   // namespace QUSB
