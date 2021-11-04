#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QMainWindow>
#include <QMessageBox>
#include <QGraphicsPixmapItem>

#ifdef USE_GPU_ACCEL
#include <QOpenGLWidget>
#endif

#include "GstreamerThread.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
    QGraphicsPixmapItem imgItem_1, imgItem_2;
    GstreamerThread* pipelineThrd_1 = nullptr;
    GstreamerThread* pipelineThrd_2 = nullptr;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
private slots:
    void slotDisplayImage_1(QImage im);
    void slotDisplayImage_2(QImage im);
    void on_btnStart_clicked();
    void on_btnStop_clicked();

signals:
    void signalNewImage(QImage, qint64);
};
#endif // MAINWINDOW_H
