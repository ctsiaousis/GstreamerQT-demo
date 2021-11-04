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
    pipelineThrd_1 = new GstreamerThread(5000);
    pipelineThrd_2 = new GstreamerThread(5001);
    connect(pipelineThrd_1, &GstreamerThread::signalNewFrame, this ,&MainWindow::slotDisplayImage_1);
    connect(pipelineThrd_2, &GstreamerThread::signalNewFrame, this ,&MainWindow::slotDisplayImage_2);

    connect(pipelineThrd_1, &GstreamerThread::signalFPS, this ,[=](int num){
        ui->labelFps->setText(QString::number(num)+" fps");
    });
    connect(pipelineThrd_2, &GstreamerThread::signalFPS, this ,[=](int num){
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

