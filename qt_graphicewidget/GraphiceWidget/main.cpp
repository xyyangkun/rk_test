
#include <QApplication>
#include "mainwindow.h"
#include "librkmedia_vi_vo.h"


int main(int argc, char *argv[])
{
	int ret=0; 
	//ret = librkmedia_vi_vo_init();
	assert(ret == 0);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
	//ret = librkmedia_vi_vo_deinit();
	assert(ret == 0);
}
