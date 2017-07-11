#include "streamerclient.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	
    StreamerClient w;			//例化一个StreamerClient对象，此时会执行StreamerClient对象的构造函数，执行一些对象的构建和信号槽的关联等
    w.show();					//将UI界面刷新出来，这时就能看到程序的界面了

    return a.exec();			
}
