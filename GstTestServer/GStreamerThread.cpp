#include "GStreamerThread.h"

GStreamerThread::GStreamerThread(int port, QObject *parent)
    : QThread(parent)
    , m_port(port)
{
    frame = QImage(m_width, m_height, QImage::Format_RGB32);
    frame.fill(Qt::gray);
}

GStreamerThread::~GStreamerThread()
{
    g_main_loop_quit (loop);
    this->exit();
    this->wait();
}


void GStreamerThread::cb_need_data (GstAppSrc *appsrc, guint unused_size ,gpointer user_data)
{
    GStreamerThread *obj = static_cast<GStreamerThread*>(user_data);
    g_print("GStreamerThread::%s - %d\n", __func__, obj->m_port);
    GstBuffer *buffer;
    GstFlowReturn ret;

    obj->frame.fill(obj->white?Qt::white:Qt::red);

    obj->white = !obj->white;

    buffer = gst_buffer_new_allocate (NULL, obj->frame.sizeInBytes(), NULL);
    gst_buffer_fill(buffer, 0, obj->frame.bits(), obj->frame.sizeInBytes());


    GST_BUFFER_PTS (buffer) = obj->timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, 1/FRAMERATE, 2);

    obj->timestamp += GST_BUFFER_DURATION (buffer);

    //g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    ret = gst_app_src_push_buffer(appsrc, buffer);

    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        g_main_loop_quit (obj->loop);
    }
    g_print("GStreamerThread::%s - end %d\n", __func__, obj->white);
}

void GStreamerThread::cb_enough_data(GstAppSrc *src, gpointer user_data)
{
    g_print("GStreamerThread::%s\n", __func__);
}

gboolean GStreamerThread::cb_seek_data(GstAppSrc *src, guint64 offset, gpointer user_data)
{
    g_print("GStreamerThread::%s\n", __func__);
    return TRUE;
}

void GStreamerThread::run()
{

    /* init GStreamer */
    gst_init (NULL, NULL);
    loop = g_main_loop_new (NULL, FALSE);

    /* setup pipeline */
    pipeline = gst_pipeline_new ("pipeline");
    appsrc = gst_element_factory_make ("appsrc", "source");
    conv = gst_element_factory_make ("videoconvert", "conv");
    videoenc = gst_element_factory_make("x264enc", "ffenc_mpeg4");
    payloader = gst_element_factory_make("rtph264pay", "rtpmp4vpay");
    g_object_set(G_OBJECT(payloader),
            "config-interval", 1,
            NULL);
    udpsink = gst_element_factory_make("udpsink", "udpsink");
    g_object_set(G_OBJECT(udpsink),
            "host", "127.0.0.1",
            "port", m_port,
            NULL);

    /* setup */
    g_object_set (G_OBJECT (appsrc), "caps",
            gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "RGB8P",
                "width", G_TYPE_INT, m_width,
                "height", G_TYPE_INT, m_height,
                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                "framerate", GST_TYPE_FRACTION, FRAMERATE, 1,
                NULL), NULL);

    // WORKS!
    gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, videoenc, payloader, udpsink, NULL);
    gst_element_link_many (appsrc, conv, videoenc, payloader, udpsink, NULL);


    /* setup appsrc */
    g_object_set (G_OBJECT (appsrc),
            "stream-type", GST_APP_STREAM_TYPE_STREAM,
            "is-live", TRUE,
            "format", GST_FORMAT_TIME, NULL);
    GstAppSrcCallbacks cbs;
    cbs.need_data = cb_need_data;
    cbs.enough_data = cb_enough_data;
    cbs.seek_data = cb_seek_data;
    gst_app_src_set_callbacks(GST_APP_SRC_CAST(appsrc), &cbs, this, NULL);

    /* play */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);

    /* clean up */
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    g_main_loop_unref (loop);

    return;
}
