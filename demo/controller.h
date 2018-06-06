#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include "handle.h"
#include "bulkio.h"

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject *parent = 0);
    virtual ~Controller();

public slots:
    void processBytes();
    void writtenBytes(qint64 bytes);
private:
    QUSB::DeviceHandle *handle;
    QUSB::BulkIO *io;
    int count;

    // QObject interface
protected:
    void timerEvent(QTimerEvent *);
};

#endif // CONTROLLER_H
