#include <QCoreApplication>
#include "GStreamerThread.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    GStreamerThread *t1 = new GStreamerThread(5000);
    t1->start();
    GStreamerThread *t2 = new GStreamerThread(5001);
    t2->start();
    return a.exec();
}
