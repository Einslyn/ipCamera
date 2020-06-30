<<<<<<< HEAD

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
=======

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
>>>>>>> a6d2214bda1c223f2b5a1caf2eccb7f94d6903ae
