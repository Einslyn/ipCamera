## install QT

### QT library

sudo apt-get install g++

sudo apt-get install g++-multilib libx11-dev libxext-dev libxtst-dev zlib1g-dev lib32ncurses5 lib32z1 libpng-dev autoconf automake libtool

./configure

make

sudo make install


### sudo gedit /etc/profile
 
export QTDIR=/usr/local/Trolltech/Qt-4.8.7
export PATH=$QTDIR/bin:$PATH
export MANPATH=$QTDIR/man:$MANPATH
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH


### How to make qt project?

- only need the qt lib, no qt creator.

* * in .pro directory, run

qmake sdkDemo.pro

make







