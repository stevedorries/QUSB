#ifndef BULKDEVICEHANDLE_H
#define BULKDEVICEHANDLE_H

#include <QObject>
#include <QIODevice>
#include <QBuffer>
#include <QMutex>
#include <QSemaphore>
#include "global.h"


#include "device.h"
#include "clibusb.h"

namespace QUSB
{
class Device;
class QUSB_SHARED_EXPORT BulkDeviceHandle : public QIODevice
{
    Q_OBJECT

    BulkDeviceHandle(
        const Device &device, libusb_device_handle *rawhandle,
        QObject *parent = 0
    );

public:
    explicit BulkDeviceHandle(const Device &device,QObject *parent = 0);


    ~BulkDeviceHandle();
    Device * getDevice();

    int activeConfiguration() const;
    int setConfiguration(int config) const;
    int claimInterface(int num);
    int releaseInterface(int num);
    int setInterfaceAlternateSetting(int,int) const;


    bool isInterfaceClaimed(int num);

//    libusb_device_handle *rawhandle() const;

    static BulkDeviceHandle *fromVendorIdProductId(quint16 vid, quint16 pid);

    QString stringDescriptor(quint32 index) const;


    static void LIBUSB_CALL transferCallback(libusb_transfer *transfer);

signals:

public slots:
    void continueRead();
    // QIODevice interface
public:
    bool isSequential() const;
    bool open(OpenMode mode);
    void close();
    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

protected:
    virtual void onTransferEvent(libusb_transfer *transfer);
    virtual bool startRead();
    virtual bool stopRead();
    virtual bool startWrite();
    virtual bool stopWrite();


private:
    libusb_device_handle *rawhandle;
    Device device;
    QList<int> claimedInterfaces;

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

    QSemaphore transferSam;
//    static const int transferSamNum = 2;
};


}// namespace QUSB
#endif // BULKDEVICEHANDLE_H
