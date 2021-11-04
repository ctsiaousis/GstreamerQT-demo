#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //// This actually makes it a bit slower and can be removed
#ifdef USE_GPU_ACCEL
    ui->graphicsView->setViewport(new QOpenGLWidget);
    ui->graphicsView_2->setViewport(new QOpenGLWidget);
#endif

    ui->graphicsView->setScene(new QGraphicsScene);
    ui->graphicsView->scene()->addItem(&imgItem_1);
    ui->graphicsView_2->setScene(new QGraphicsScene);
    ui->graphicsView_2->scene()->addItem(&imgItem_2);
    pipelineThrd_1 = new GstreamerThread_1(5000);
    pipelineThrd_2 = new GstreamerThread_2(5001);
    connect(pipelineThrd_1, &GstreamerThread_1::signalNewFrame, this ,&MainWindow::slotDisplayImage_1);
    connect(pipelineThrd_2, &GstreamerThread_2::signalNewFrame, this ,&MainWindow::slotDisplayImage_2);

    connect(pipelineThrd_1, &GstreamerThread_1::signalFPS, this ,[=](int num){
        ui->labelFps->setText(QString::number(num)+" fps");
    });
    connect(pipelineThrd_2, &GstreamerThread_2::signalFPS, this ,[=](int num){
        ui->labelFps_2->setText(QString::number(num)+" fps");
    });
    pipelineThrd_1->start();
    pipelineThrd_2->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete pipelineThrd_1;
    delete pipelineThrd_2;
}

void MainWindow::slotDisplayImage_1(QImage im)
{
//    qDebug() << "MainWindow::slotDisplayImage" << im;
    imgItem_1.setPixmap(QPixmap::fromImage(im));
    ui->graphicsView->fitInView(&imgItem_1);
    ui->graphicsView->update();
}
void MainWindow::slotDisplayImage_2(QImage im)
{
//    qDebug() << "MainWindow::slotDisplayImage" << im;
    imgItem_2.setPixmap(QPixmap::fromImage(im));
    ui->graphicsView_2->fitInView(&imgItem_2);
    ui->graphicsView_2->update();
}

void MainWindow::on_btnStart_clicked()
{
    qDebug() << "MainWindow::on_btnStart";
    pipelineThrd_1->play();
    pipelineThrd_2->play();
}


void MainWindow::on_btnStop_clicked()
{
    qDebug() << "MainWindow::on_btnStop";
    pipelineThrd_1->pause();
    pipelineThrd_2->pause();
}

/* /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 *
 *
 int MainWindow::read_packet(void *opaque, uint8_t *buf, int buf_size) {
    //auto	fil = static_cast<QUdpSocket*>(opaque);
    //QMutexLocker	l(&fil->mutex);
    //qDebug() << buf_size << fil->buffer.length();

    qDebug() << "MainWindow::read_packet" << buf_size << buffer.length();

    if (buffer.isEmpty() && !udpSocket->hasPendingDatagrams()) udpSocket->waitForReadyRead();
    while (udpSocket->hasPendingDatagrams()) {
        if (buffer.length() > 1024*500) {
            buffer = buffer.right(1024*500);
            buffer.clear();
        }
        buffer.append(udpSocket->receiveDatagram().data());
    }

    buf_size = buffer.length() > buf_size ? buf_size : buffer.length();
    memcpy(buf, buffer.constData(), buf_size);
    buffer.remove(0, buf_size);

    return buf_size;
};

QFuture<int> fns = QtConcurrent::run([this]() -> int{
    udpSocket = new QUdpSocket;
    udpSocket->bind(5000);

    AVFormatContext *fmt_ctx = nullptr;
    AVIOContext *avio_ctx = nullptr;
    uint8_t *buffer = nullptr, *avio_ctx_buffer = nullptr;
    size_t buffer_size = 0, avio_ctx_buffer_size = 4096;

    int ret = 0;
    fmt_ctx = avformat_alloc_context();
    Q_ASSERT(fmt_ctx);

    avio_ctx_buffer = reinterpret_cast<uint8_t*>(av_malloc(avio_ctx_buffer_size));
    Q_ASSERT(avio_ctx_buffer);
    avio_ctx = avio_alloc_context(avio_ctx_buffer, static_cast<int>(avio_ctx_buffer_size), 0, udpSocket, &MainWindow::read_packet, nullptr, nullptr);
    Q_ASSERT(avio_ctx);

    fmt_ctx->pb = avio_ctx;
    ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
    Q_ASSERT(ret >= 0);
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    Q_ASSERT(ret >= 0);

    AVStream	*video_stream = nullptr;
    for (uint i=0; i<fmt_ctx->nb_streams; ++i) {
        auto	st = fmt_ctx->streams[i];
        qDebug() << "st:" << st->id << st->index << st->start_time << st->duration << st->codecpar->codec_type;
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) video_stream = st;
    }
    Q_ASSERT(video_stream);


    AVCodecParameters	*codecpar = video_stream->codecpar;
    AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    int err = avcodec_open2(codec_context, codec, nullptr);
    Q_ASSERT(err>=0);
    qDebug() <<"name, id"<< codec->name << codec->id;
    qDebug() <<"codec"<< codecpar->width << codecpar->height << codecpar->format << codec_context->pix_fmt;

    AVFrame	*frameRGB = av_frame_alloc();
    frameRGB->format = AV_PIX_FMT_RGB24;
    frameRGB->width = codecpar->width;
    frameRGB->height = codecpar->height;
    err = av_frame_get_buffer(frameRGB, 0);
    Q_ASSERT(err == 0);
    ///
    SwsContext *imgConvertCtx = nullptr;

    AVFrame* frame = av_frame_alloc();
    AVPacket packet;

    while (av_read_frame(fmt_ctx, &packet) >= 0) {
        avcodec_send_packet(codec_context, &packet);
        err = avcodec_receive_frame(codec_context, frame);
        if (err == 0) {
            //qDebug() << frame->height << frame->width << codec_context->pix_fmt;

            imgConvertCtx = sws_getCachedContext(imgConvertCtx,
                                                 codecpar->width, codecpar->height, static_cast<AVPixelFormat>(codecpar->format),
                                                 frameRGB->width, frameRGB->height,
                                                 AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
            Q_ASSERT(imgConvertCtx);

            //conversion frame to frameRGB
            sws_scale(imgConvertCtx, frame->data, frame->linesize, 0, codec_context->height, frameRGB->data, frameRGB->linesize);

            //setting QImage from frameRGB
            QImage image(frameRGB->data[0],
                    frameRGB->width,
                    frameRGB->height,
                    frameRGB->linesize[0],
                    QImage::Format_RGB888);

            emit signalNewImage(image, QDateTime::currentMSecsSinceEpoch());
        }

        av_frame_unref(frame);
        av_packet_unref(&packet);
    }
    if (imgConvertCtx) sws_freeContext(imgConvertCtx);

    //end:
    avformat_close_input(&fmt_ctx);
    //note: the internal buffer could have changed, and be != avio_ctx_buffer
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);
    if (ret < 0) {
        qWarning() << ret;
        return 1;
    }
    return 0;
});

return fns.result();
*/
