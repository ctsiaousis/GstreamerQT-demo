#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QMainWindow>
#include <QMessageBox>
#include <QGraphicsPixmapItem>

#include "GstreamerThread.h"
#include "GstreamerThread_NIR.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT
    QGraphicsPixmapItem imgItem_rgb,imgItem_nir;
    GstreamerThread* pipelineThrd_rgb = nullptr;
    GstreamerThread_NIR* pipelineThrd_nir = nullptr;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
private slots:
    void slotDisplayImage_rgb(QImage im);
    void slotDisplayImage_nir(QImage im);
    void on_btnStart_clicked();
    void on_btnStop_clicked();

signals:
    void signalNewImage(QImage, qint64);
};
#endif // MAINWINDOW_H
