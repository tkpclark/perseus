#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QTextCodec>
#include <pthread.h>

void* ppp(void *arg)
{
     ((MainWindow *)arg)->status_update();
     return 0;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QTextCodec *codec = QTextCodec::codecForName("GB2312");

    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
    w.setWindowFlags(Qt::FramelessWindowHint);
    w.show();



    pthread_t ntid;
    pthread_create (&ntid, NULL, ppp, (void *)&w);
    a.exec();


    return 0;
}

