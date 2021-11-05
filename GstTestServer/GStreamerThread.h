#ifndef GSTREAMERTHREAD_H
#define GSTREAMERTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/gstbuffer.h>

#define FRAMERATE 10

class GStreamerThread : public QThread
{
    Q_OBJECT
    int m_port = 5000;
public:
    explicit GStreamerThread(int port, QObject *parent = nullptr);
    ~GStreamerThread();
    GMainLoop *loop;
    GstElement *pipeline, *appsrc, *conv, *payloader, *udpsink, *videoenc;
    GstClockTime timestamp = 0;
    gint m_width = 340, m_height = 248;
    QImage frame;
    bool white = false;
    static void cb_need_data(GstAppSrc *appsrc, guint unused_size, gpointer ctxObj);
    static void cb_enough_data(GstAppSrc *src, gpointer user_data);
    static gboolean cb_seek_data(GstAppSrc *src, guint64 offset, gpointer user_data);
protected:
    void run() override;
};

#endif // GSTREAMERTHREAD_H
