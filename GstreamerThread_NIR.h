#ifndef GSTREAMERTHREAD_NIR_H
#define GSTREAMERTHREAD_NIR_H

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

/*!
 * \brief The GstreamerThread_NIR class is exactly the same
 * with the GstreamerThread class. I implemented it this way
 * due to the static callback functions and variables. If you
 * find a better way to use the same class please contact me:
 *  ctsiaous@gmail.com
 */
class GstreamerThread_NIR : public QThread
{
    Q_OBJECT
    GstElement *pipeline = nullptr;
    bool mContinue;
    bool mDestroy = false;
    QMutex playPauseMtx;
    QTimer fpsTimer;
    int m_port;
public:
    explicit GstreamerThread_NIR(int port, QObject *parent = nullptr);
    ~GstreamerThread_NIR();
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
