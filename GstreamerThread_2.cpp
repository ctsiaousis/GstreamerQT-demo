#include "GstreamerThread_2.h"

std::atomic<QImage*> GstreamerThread_2::atomicFrame;
std::atomic<int> GstreamerThread_2::framecount;
QMutex GstreamerThread_2::mtxFrame;
QWaitCondition GstreamerThread_2::frameReadyCond;


GstreamerThread_2::GstreamerThread_2(int port, QObject *parent) : QThread(parent), m_port(port)
{
    mContinue = false;
    GstreamerThread_2::atomicFrame.exchange(nullptr);
    GstreamerThread_2::framecount.exchange(0);

    fpsTimer.setSingleShot(false);
    connect(&fpsTimer, &QTimer::timeout, this, [=](){
//        qDebug() << "GstreamerThread_2::FPS" << GstreamerThread_2::framecount;
        emit signalFPS(GstreamerThread_2::framecount);
        GstreamerThread_2::framecount.exchange(0);
    });
}

GstreamerThread_2::~GstreamerThread_2()
{
    mDestroy = true;
    this->wait();
}

void GstreamerThread_2::run()
{
    int argc = 0;
    char **argv;
    gst_init(&argc, &argv);
    qDebug() << "GstreamerThread_2::run - 0";
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

    qDebug() << "GstreamerThread_2::run - 1";
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
    qDebug() << "GstreamerThread_2::run - 1.1";

    // Get sink
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");

    /**
     * @brief Get sink signals and check for a preroll
     *  If preroll exists, we do have a new frame
     */
    gst_app_sink_set_emit_signals((GstAppSink*)sink, true);
    gst_app_sink_set_drop((GstAppSink*)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink*)sink, 1);
    GstAppSinkCallbacks callbacks = { nullptr, GstreamerThread_2::new_preroll, GstreamerThread_2::new_sample };
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, nullptr, nullptr);

    qDebug() << "GstreamerThread_2::run - 2";
    // Declare bus
    GstBus *bus;
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, GstreamerThread_2::my_bus_callback, nullptr);
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
            GstreamerThread_2::mtxFrame.lock();
            GstreamerThread_2::frameReadyCond.wait(&GstreamerThread_2::mtxFrame);
            frame = GstreamerThread_2::atomicFrame.load();
            if(frame != nullptr) {
                emit signalNewFrame(frame->copy());
            }
            frame = nullptr;
            GstreamerThread_2::mtxFrame.unlock();
        }else{
            QThread::currentThread()->msleep(600);
        }
    }
    //////////////////////////////////////////////////////////////////////////////
    qDebug() << "GstreamerThread_2::run - end";
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    this->exit();
}

/**
 * @brief Check preroll to get a new frame using callback
 *  https://gstreamer.freedesktop.org/documentation/design/preroll.html
 * @return GstFlowReturn
 */
GstFlowReturn GstreamerThread_2::new_preroll(GstAppSink* /*appsink*/, gpointer /*data*/)
{
    return GST_FLOW_OK;
}

/**
 * @brief This is a callback that get a new frame when a preroll exist
 *
 * @param appsink
 * @return GstFlowReturn
 */
GstFlowReturn GstreamerThread_2::new_sample(GstAppSink *appsink, gpointer /*data*/)
{
    // Get caps and frame
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const int width = g_value_get_int(gst_structure_get_value(structure, "width"));
    const int height = g_value_get_int(gst_structure_get_value(structure, "height"));

    // Show caps on first frame
//    if(!GstreamerThread_2::framecount) {
//        g_print("caps: %s\n", gst_caps_to_string(caps));
//    }
    GstreamerThread_2::framecount++;

    // Get frame data
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    if(map.data){
        QImage* prevFrame = GstreamerThread_2::atomicFrame.load();
        if(prevFrame) {
            delete prevFrame;
        }
        GstreamerThread_2::mtxFrame.lock();
        GstreamerThread_2::atomicFrame.store(new QImage((uchar*)map.data, width, height, QImage::Format_BGR888));
        GstreamerThread_2::mtxFrame.unlock();
        GstreamerThread_2::frameReadyCond.wakeAll();
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
gboolean GstreamerThread_2::my_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
    // Debug message
    g_print("GstreamerThread_1::bus_callback Got %s message\n", GST_MESSAGE_TYPE_NAME(message));
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
        g_print("GstreamerThread_2::Ready to stream\n");
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







void GstreamerThread_2::play()
{
    QMutexLocker varname(&playPauseMtx);
    if(!mContinue){
        GstreamerThread_2::framecount.exchange(0);
        fpsTimer.start(1000);
        GstreamerThread_2::frameReadyCond.wakeAll();
        mContinue = true;
    }
}

void GstreamerThread_2::pause()
{
    QMutexLocker varname(&playPauseMtx);
    if(mContinue){
        mContinue = false;
        fpsTimer.stop();
    }
}


