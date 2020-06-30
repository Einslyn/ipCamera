
#ifndef _SOCKSERVER_H_
#define _SOCKSERVER_H_

#include <QThread>

class SockServer:public QThread
{
    Q_OBJECT
    public:
    explicit SockServer(QObject *parent  = 0);

    protected:
    run();

    signals:
    void socketSignal(int);
}


#endif
