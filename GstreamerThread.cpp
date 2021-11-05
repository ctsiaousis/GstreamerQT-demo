#include "GstreamerThread.h"


GstreamerThread::GstreamerThread(int port, QObject *parent) : QThread(parent), m_port(port)
{
    mContinue = false;
    this->atomicFrame.exchange(nullptr);
    this->framecount.exchange(0);

    fpsTimer.setSingleShot(false);
    connect(&fpsTimer, &QTimer::timeout, this, [=](){
//        qDebug() << "GstreamerThread::FPS" << this->framecount;
        emit signalFPS(this->framecount);
        this->framecount.exchange(0);
    });
}

GstreamerThread::~GstreamerThread()
{
    mDestroy = true;
    this->wait();
}

void GstreamerThread::run()
{
    int argc = 0;
    char **argv;
    gst_init(&argc, &argv);
    qDebug() << "GstreamerThread::run - 0";
    QString pipe;
#ifdef USE_GPU_ACCEL
    pipe =  "udpsrc port="+QString::number(m_port)+" "
            "! application/x-rtp, payload=96 ! rtph264depay ! parsebin ! d3d11h264dec "
            "! videoconvert ! video/x-raw,format=(string)BGR "
            "! appsink name=sink emit-signals=true sync=false max-buffers=1 drop=true";
#else
    pipe =  "udpsrc port="+QString::number(m_port)+" "
            "! application/x-rtp, payload=96 ! rtph264depay ! h264parse ! parsebin ! avdec_h264 "
            "! decodebin ! videoconvert ! video/x-raw,format=(string)BGR "
            "! appsink name=sink emit-signals=true sync=false max-buffers=1 drop=true";
#endif
    gchar *descr = g_strdup(pipe.toStdString().c_str());

    qDebug() << "GstreamerThread::run - 1";
    // Check pipeline
    GError *error = nullptr;
    pipeline = gst_parse_launch(descr, &error);

    if(error) {
        gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
        g_print("could not construct pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }
    qDebug() << "GstreamerThread::run - 1.1";

    // Get sink
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    /**
     * @brief Get sink signals and check for a preroll
     *  If preroll exists, we do have a new frame
     */
    gst_app_sink_set_emit_signals((GstAppSink*)sink, true);
    gst_app_sink_set_drop((GstAppSink*)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink*)sink, 1);
    GstAppSinkCallbacks callbacks = { nullptr, GstreamerThread::new_preroll, GstreamerThread::new_sample };
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, this, nullptr); //this is the gpointer on callbacks

    qDebug() << "GstreamerThread::run - 2";
    // Declare bus
    GstBus *bus;
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, GstreamerThread::my_bus_callback, this); //this is the gpointer on callbacks
    gst_object_unref(bus);

    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

    ///////////////////////////////////// Main loop //////////////////////////////
    QImage* frame = nullptr;
    while(!mDestroy) {
        playPauseMtx.lock();
        bool b = mContinue;
        playPauseMtx.unlock();
        g_main_iteration(false);
        if(b){
            this->mtxFrame.lock();
            this->frameReadyCond.wait(&this->mtxFrame);
            frame = this->atomicFrame.load();
            if(frame != nullptr) {
                emit signalNewFrame(frame->copy());
                delete frame;
            }
            frame = nullptr;
            this->mtxFrame.unlock();
        }else{
            QThread::currentThread()->msleep(600);
        }
    }
    //////////////////////////////////////////////////////////////////////////////
    qDebug() << "GstreamerThread::run - end";
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    this->exit();
}

/**
 * @brief Check preroll to get a new frame using callback
 *  https://gstreamer.freedesktop.org/documentation/design/preroll.html
 * @return GstFlowReturn
 */
GstFlowReturn GstreamerThread::new_preroll(GstAppSink* /*appsink*/, gpointer /*data*/)
{
    return GST_FLOW_OK;
}

/**
 * @brief This is a callback that get a new frame when a preroll exist
 *
 * @param appsink
 * @return GstFlowReturn
 */
GstFlowReturn GstreamerThread::new_sample(GstAppSink *appsink, gpointer data)
{
    // Get caps and frame
    GstreamerThread *obj = static_cast<GstreamerThread*>(data);
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const int width = g_value_get_int(gst_structure_get_value(structure, "width"));
    const int height = g_value_get_int(gst_structure_get_value(structure, "height"));

    // Show caps on first frame
//    if(!GstreamerThread::framecount) {
//        g_print("caps: %s\n", gst_caps_to_string(caps));
//    }
    obj->framecount++;

    // Get frame data
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    if(map.data){
//        QImage* prevFrame = obj->atomicFrame.load();
//        if(prevFrame) {
//            delete prevFrame;
//        }
        obj->mtxFrame.lock();
        obj->atomicFrame.store(new QImage((uchar*)map.data, width, height, QImage::Format_BGR888));
        obj->mtxFrame.unlock();
        obj->frameReadyCond.wakeAll();
    }


    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

/**
 * @brief Bus callback
 *  Print important messages
 *
 * @param bus
 * @param message
 * @param data
 * @return gboolean
 */
gboolean GstreamerThread::my_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
    // Debug message
    g_print("GstreamerThread_1::bus_callback - %u - Got %s message\n", data, GST_MESSAGE_TYPE_NAME(message));
    switch(GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;

            gst_message_parse_error(message, &err, &debug);
            g_print("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            g_print("end-of-stream");
            break;
    case GST_MESSAGE_STREAM_START:
        g_print("GstreamerThread::Ready to stream\n");
        //we have available stream no need to watch buss again
        break;
        default:
            /* unhandled message */
            break;
    }
    /* we want to be notified again the next time there is a message
     * on the bus, so returning TRUE (FALSE means we want to stop watching
     * for messages on the bus and our callback should not be called again)
     */
    return true;
}







void GstreamerThread::play()
{
    QMutexLocker varname(&playPauseMtx);
    if(!mContinue){
        this->framecount.exchange(0);
        fpsTimer.start(1000);
        this->frameReadyCond.wakeAll();
        mContinue = true;
    }
}

void GstreamerThread::pause()
{
    QMutexLocker varname(&playPauseMtx);
    if(mContinue){
        mContinue = false;
        fpsTimer.stop();
    }
}


