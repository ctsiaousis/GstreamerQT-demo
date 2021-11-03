#ifndef GSTREAMERTHREAD_H
#define GSTREAMERTHREAD_H

#include <atomic>
#include <QThread>
#include <QImage>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QMutexLocker>
#include <QWaitCondition>
#include <gst/gst.h>
#include <gst/app/app.h>

class GstreamerThread : public QThread
{
    Q_OBJECT
    GstElement *pipeline = nullptr;
    bool mContinue;
    bool mDestroy = false;
    QMutex playPauseMtx;
    QTimer fpsTimer;
    int m_port;
public:
    explicit GstreamerThread(int port, QObject *parent = nullptr);
    ~GstreamerThread();
    static QImage* atomicFrame;
    static QMutex mtxFrame;
    static QWaitCondition frameReadyCond;
    static std::atomic<int> framecount;
    static gboolean my_bus_callback(GstBus *bus, GstMessage *message, gpointer data);
    static GstFlowReturn new_sample(GstAppSink *appsink, gpointer);
    static GstFlowReturn new_preroll(GstAppSink *, gpointer);

protected:
    void run() override;

public slots:
    void play();
    void pause();

signals:
    void signalNewFrame(QImage);
    void signalFPS(int);
};

#endif // GSTREAMERTHREAD_H
