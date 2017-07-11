#include "streamerclient.h"
#include "ui_streamerclient.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDateTime>

//构造函数StreamerClient(QWidget *parent)的实现
StreamerClient::StreamerClient(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StreamerClient)
{
    ui->setupUi(this);									//这句话，表示将UI界面刷新到显示器上

    pTcpClient      = new QTcpSocket;					//例化QTcpSocket一个对象
    pTcpClient->setReadBufferSize(64 * 1024);			//设置pTcpClient对象的ReadBuffer的缓冲区大小：64 * 1024
    BufferCnt       = 0;
    ContentSize     = 0;
    ImageByteArray  = new QByteArray;					//例化QByteArray一个对象
    LastLeftArray.clear();								//将对象LastLeftArray清空
    FrameArray.clear();									//将对象FrameArray清空
	
	/* 三个按钮状态都设置为false，表示打开软件之后，没有按下这三个按钮 */
    isSharpen   = false;
    isSoften    = false;
    isGrayscale = false;

    connect(ui->ConnectBtn, SIGNAL(clicked(bool)), this, SLOT(ConnectToServer()));	//将ConnectBtn这个按钮的clicked(bool)信号，与ConnectToServer()槽关联
    connect(ui->StartBtn, SIGNAL(clicked(bool)), this, SLOT(StreamStart()));		//将StartBtn这个按钮的clicked(bool)信号，与StreamStart()槽关联
    connect(ui->StopBtn, SIGNAL(clicked(bool)), this, SLOT(StreamStop()));			//将StopBtn这个按钮的clicked(bool)信号，与StreamStop()槽关联
    connect(ui->SnapBtn, SIGNAL(clicked(bool)), this, SLOT(Snap()));				//将SnapBtn这个按钮的clicked(bool)信号，与Snap()槽关联

    connect(ui->SharpenBtn, SIGNAL(clicked(bool)), this, SLOT(SetSharpen()));		//将SharpenBtn这个按钮的clicked(bool)信号，与SetSharpen()槽关联
    connect(ui->SoftenBtn, SIGNAL(clicked(bool)), this, SLOT(SetSoften()));			//将SoftenBtn这个按钮的clicked(bool)信号，与SetSoften()槽关联
    connect(ui->GrayscaleBtn, SIGNAL(clicked(bool)), this, SLOT(SetGrayscale()));	//将GrayscaleBtn这个按钮的clicked(bool)信号，与SetGrayscale()槽关联

    connect(ui->DisconnectBtn, SIGNAL(clicked(bool)), this, SLOT(DisconnectFromServer()));	//将DisconnectBtn这个按钮的clicked(bool)信号，与DisconnectFromServer()槽关联

    connect(pTcpClient, SIGNAL(readyRead()), this, SLOT(ReadImage()));						//将SnapBtn这个按钮的clicked(bool)信号，与ReadImage()槽关联

    connect(ui->CloseBtn, SIGNAL(clicked(bool)), this, SLOT(close()));						//将CloseBtn这个按钮的clicked(bool)信号，与close()槽关联
}

StreamerClient::~StreamerClient()															//析构函数，释放相应的对象
{
    delete pTcpClient;																		//删除对象pTcpClient
    delete ImageByteArray;																	//删除对象ImageByteArray
    delete ui;																				//删除对象ui
}


void StreamerClient::ConnectToServer(void)
{
    QString ServerIP    = ui->ServerIPEdit->text();							//从ServerIPEdit输入框中，获取用户输入的IP地址
    QString ServerPort  = ui->ServerPortEdit->text();						//从ServerPortEdit输入框中，获取用户输入的端口号

    pTcpClient->connectToHost(QHostAddress(ServerIP), ServerPort.toInt());	//开始一个TCP/IP连接
    if(!pTcpClient->waitForConnected(2))									//如果连接超时
    {
        pTcpClient->disconnectFromHost();									//将当前的连接断开
        ui->StatusLabel->setText(tr("Connect error!"));						//在StatusLabel中显示"Connect error!"，提示用户连接错误
        return;
    }
	else
    {
        ui->StatusLabel->setText(tr("Connect success!"));					//连接成功，在StatusLabel中显示"Connect success!"
    }
}

void StreamerClient::DisconnectFromServer(void)
{
    pTcpClient->disconnectFromHost();										//从主机断开连接
    ui->StatusLabel->setText(tr("disconnect!"));							//在StatusLabel中显示"disconnect!"
}

void StreamerClient::StreamStart(void)
{
    RequestImage();															//获取图像流
    FrameTimer.start(20);													//打开定时器
}

void StreamerClient::StreamStop(void)
{
    FrameTimer.stop();														//停止定时器
}

void StreamerClient::Snap(void)
{
    QString DateStr = "123";
    QDateTime SysDateTime =QDateTime::currentDateTime();					//获取当前的系统时间
    DateStr = SysDateTime.toString("yyyyMMddhhmmss");						//将系统时间格式化为"yyyyMMddhhmmss"这种格式
    QPixmap p=ui->ImageLabel->grab(QRect(0,0,640,480));						//截取当前显示的这一帧图像，以截图的方式截取
    p.save("/home/larrychen/pictures/"+DateStr,"jpg");						//将截取到的图像，保存在指定的目录
    QString SnapInfoStr="Sreenshot Success!\n";
    SnapInfoStr= DateStr+".jpg";											//设置截图的名称
    ui->InfoTextEdit->append(SnapInfoStr);									//将截图的名称，追加到InfoTextEdit里，显示给用户
}

void StreamerClient::Record(void)
{

}

void StreamerClient::RequestImage()
{
    QByteArray ReqByteArray;		
    ReqByteArray.append("GET /?action=stream HTTP/1.1\r\n\r\n");			//通过TCP/IP协议，去模拟http协议，发送一个GET请求
    pTcpClient->write(ReqByteArray);										//将"GET /?action=stream HTTP/1.1\r\n\r\n"写入socket通过TCP/IP协议发送到指定的IP地址
}


void StreamerClient::ReadImage()											//程序的核心部分：解析来自MJPEG-Streamer服务器的视频数据流
{
    qint32  TempIndex;
    QString FrameStr;
	
	//从socket里读取来自MJPEG-Streamer服务器的数据（注意：来自MJPEG-Streamer服务器并不是一次就将一帧完整的数据帧发送到客户端，而是断断续续，多次发送的，没有规律的，所以需要解析）
    *ImageByteArray = pTcpClient->readAll();/* 将socktet中的当前数据流全部读取出来 */
    //*ImageByteArray = pTcpClient->read(64 * 1024);
	
	/* "#if ... #endif" 这个是条件编译，如果#if后面是0，表示这段代码不编译，否则如果#if后面是1，则表示这段代码要编译 */
	
	/* 下面这段代码表示把来自MJPEG-Streamer服务器的数据全部写入文件"/home/larrychen/pictures/streamer.jpg"，只是测试和分析用的，实际并客户端运行的时候，并没有用到 */
#if 0
    QFile ImageData("/home/larrychen/pictures/streamer.jpg");
    ImageData.open(QIODevice::Append|QIODevice::WriteOnly);
    ImageData.write(*ImageByteArray);
    ImageData.close();
#endif

    //获取Content-Length
    TempIndex = ImageByteArray->indexOf("Content-Length: ");				/* 获取当前socket中的数据里的"Content-Length: "的序号 */
    if( TempIndex != -1)                             						//match the string "Content-Length: "
    {
        SizeArray = ImageByteArray->mid(TempIndex + 16);                	//去掉“Content-Length: ”前面所有的内容
        TempIndex = SizeArray.indexOf("\r\n");                          	//找到第一个\r\n
        SizeArray = SizeArray.left(TempIndex);                          	//去掉\r\n后面的内容，同时保存帧的字节数

        QString strFrameSize = QByteArray(SizeArray);						/* 将SizeArray转化为QString类型*/
        ContentSize = strFrameSize.toInt(&ok,10);							/* 将"Content-Length: "后面的数据流转化为Int类型，并保存在变量ContentSize中*/
        //qDebug("\n\tContent-Length:[ %d ]", ContentSize);

        FrameArray.clear();													/* 将FrameArray中的内容清空*/
        TempIndex = ImageByteArray->indexOf("X-Timestamp: ");						/* 获取当前socket中的数据里的"X-Timestamp: "的序号*/
        FrameArray = ImageByteArray->right(ImageByteArray->size() - TempIndex);		/* 去掉"X-Timestamp: "前面所有的内容*/

        TempIndex = ImageByteArray->indexOf("\r\n\r\n");							/* 获取当前数据流里的"\r\n\r\n"的序号*/
        FrameArray = ImageByteArray->right(ImageByteArray->size() - TempIndex - 4);	/* 去掉"\r\n\r\n"前面所有的内容*/

        //qDebug("FrameArray = %d",FrameArray.size());
        FrameStr = QByteArray(FrameArray);											/* 将"\r\n\r\n"后面的数据保存在FrameStr中*/
        //qDebug()<<FrameStr;
    }

#if 1
    FrameStr = QByteArray(LastLeftArray);							/* 上一次socket剩余的内容*/
    //qDebug()<<"\tLastLeftArray:"<<FrameStr;
    if(LastLeftArray.isNull() == false)								/* 如果不为空*/
    {	
        FrameArray.append(LastLeftArray);							/* 追加到FrameStr里*/
        LastLeftArray.clear();										/* 将LastLeftArray清空*/
    }
#endif

    FrameArray.append(*ImageByteArray);								/* 将ImageByteArray里面的数据流追加到FrameStr里*/

#if 1
    TempIndex = FrameArray.indexOf("--boundarydonotcross");			/* 获取"--boundarydonotcross"在当前数据流里的位置*/
    if(TempIndex != -1)/* */
    {
        FrameArray = FrameArray.left(TempIndex);					/* 去掉"--boundarydonotcross"后面的数据，并保存它前面的数据流*/
    }

    TempIndex = ImageByteArray->indexOf("--boundarydonotcross");	/* 获取"--boundarydonotcross"在当前数据流里的位置*/
    if(TempIndex != -1)/* */
    {
        //qDebug("----------------------------boundarydonotcross-------------------------------\n");
        LastLeftArray = ImageByteArray->right(ImageByteArray->size() - TempIndex);/* 去掉"--boundarydonotcross"前面的数据，并保存它后面的数据流，保存在LastLeftArray中*/
    }
    else
    {
        //qDebug("no boundary");
    }
#endif


    //qDebug("FrameArray.size() = %d", FrameArray.size());
    //跟将Content-Length与缓冲区接收到的数据长度比较,
    if( FrameArray.size() >= ContentSize && ContentSize != 0)	/* 如果从数据流里读出的长度ContentSize不等于0，并且解析出来的视频流数据长度大于等于ContentSize就刷新显示 */
    {
        /*==========================================*/
        UpdateFrame();											/*刷新图像，让图像数据动态显示*/
    }
}

void StreamerClient::UpdateFrame()/* */
{
    //qDebug("show frame\n");
    //QImage PixelFrame;
    QByteArray StartFlag = QByteArray::fromHex("FFD8");			/* 获取视频数据帧中"FFD8"的序号*/
    QByteArray EndFlag = QByteArray::fromHex("FFD9");			/* 获取视频数据帧中"FFD9"的序号*/

    qint64  IndexOfStart = FrameArray.indexOf(StartFlag);		/* 获取视频数据帧中"FFD8"的序号*/
    qint64  IndexOfEnd = FrameArray.lastIndexOf(EndFlag);		/* 获取视频数据帧中"FFD9"的序号*/

    if(IndexOfStart != -1)/* */
    {
        FrameArray = FrameArray.right(FrameArray.size() - IndexOfStart);/* 获取视频数据帧中"FFD8"和"FFD9"中间的部分*/
    }
    else
    {
        qDebug("Can't find the flag \"FFD8\"");
        return;
    }

    if(IndexOfEnd != -1)
    {
        FrameArray = FrameArray.left(IndexOfEnd + 2);					/* 为了把末尾的"FFD9"包含进来，所以要加2（两个十六进制数，0xFF和0xD9）*/
    }
    else
    {
        qDebug("Can't find the flag \"FFD8\"");
        return;
    }

    QPixmap PixelFrame;												/* 声明一个QPixmap对象PixelFrame*/
    PixelFrame.loadFromData(FrameArray);							/* 从FrameArray载入数据到PixelFrame对象里*/
#if 1
    if(isSharpen == true || isSoften ==true || isGrayscale == true)	/* 如果从选择了任意一种图像处理的方式，相应的标志变量就会为ture*/
    {
        if(isSharpen == true)
        {
            ImageSharpen();											/* 图像锐化*/
        }
        else if (isSoften ==true)
        {
            ImageSoften();											/* 图像柔化*/
        }
        else if(isGrayscale == true){
            ImageGrayScale();										/* 图像灰度处理*/
        }
    }
#endif
    else//如果正常显示
    {
        ui->ImageLabel->setPixmap(PixelFrame);/* 将视频数据流显示在ImageLabel控件上*/
    }

    /*==========================================*/
    ContentSize = 0;							//每显示完一帧图像数据，就将ContentSize清空
}


void StreamerClient::SetSharpen()
{
    if(ui->SharpenBtn->text() == "Sharpen")			/* 如果按钮SharpenBtn显示的是"Sharpen"*/
    {
        isSharpen = true;							/* 设置isSharpen标志位为true*/
        ui->SharpenBtn->setText("Cancel");			/* 设置按钮SharpenBtn显示为"Cancel"*/

        ui->SoftenBtn->setEnabled(false);			/* 让按钮SoftenBtn不能使用*/
        ui->GrayscaleBtn->setEnabled(false);		/* 让按钮GrayscaleBtn不能使用*/
    }
    else if( ui->SharpenBtn->text() == "Cancel" )	/* 如果按钮SharpenBtn显示的是"Cancel*/
    {
        isSharpen = false;							/* 设置isSharpen标志位为false*/
        ui->SharpenBtn->setText("Sharpen");			/* 设置按钮SharpenBtn显示为"Sharpen"*/

        ui->SoftenBtn->setEnabled(true);			/* 让按钮SoftenBtn可以使用*/
        ui->GrayscaleBtn->setEnabled(true);			/* 让按钮GrayscaleBtn可以使用*/
    }
}

void StreamerClient::SetSoften()/* */
{
    if(ui->SoftenBtn->text() == "Soften")			/* 如果按钮SoftenBtn显示的是"Soften"*/
    {
        isSoften = true;							/* 设置isSoften标志位为true*/
        ui->SoftenBtn->setText("Cancel");			/* 设置按钮SoftenBtn显示为"Cancel"*/

        ui->SharpenBtn->setEnabled(false);			/* 让按钮SharpenBtn不能使用*/
        ui->GrayscaleBtn->setEnabled(false);		/* 让按钮GrayscaleBtn不能使用*/
    }
    else if( ui->SoftenBtn->text() == "Cancel" )	/* 如果按钮SoftenBtn显示的是"Cancel*/
    {
        isSoften = false;							/* 设置isSoften标志位为false*/
        ui->SoftenBtn->setText("Soften");			/* 设置按钮SoftenBtn显示"Soften"*/

        ui->SharpenBtn->setEnabled(true);			/* 让按钮SharpenBtn可以使用*/
        ui->GrayscaleBtn->setEnabled(true);			/* 让按钮GrayscaleBtn可以使用*/
    }
}

void StreamerClient::SetGrayscale()/* */
{
    if(ui->GrayscaleBtn->text() == "Grayscale")			/* 如果按钮SoftenBtn显示的是"Soften"*/
    {
        isGrayscale = true;								/* 设置isGrayscale标志位为true*/
        ui->GrayscaleBtn->setText("Cancel");			/* 设置按钮GrayscaleBtn显示为"Cancel"*/

        ui->SoftenBtn->setEnabled(false);				/* 让按钮SoftenBtn不能使用*/
        ui->SharpenBtn->setEnabled(false);				/* 让按钮SharpenBtn不能使用*/
    }
    else if( ui->GrayscaleBtn->text() == "Cancel" )		/* 如果按钮GrayscaleBtn显示的是"Cancel*/
    {
        isGrayscale = false;							/* 设置isGrayscale标志位为false*/
        ui->GrayscaleBtn->setText("Grayscale");			/* 设置按钮GrayscaleBtn显示"Soften"*/

        ui->SoftenBtn->setEnabled(true);				/* 让按钮SoftenBtn可以使用*/
        ui->SharpenBtn->setEnabled(true);				/* 让按钮SharpenBtn可以使用*/
    }
}

void StreamerClient::ImageSharpen()						/* 图像锐化处理的算法实现*/
{
    QImage image;
    image.loadFromData(FrameArray);						/* 从FrameArray中载入一帧图像数据*/
    if(image.isNull())									/* 如果图像数据为空*/
    {
        qDebug() << "load image data failed! ";
        return;
    }
    int width = ui->ImageLabel->width();				/* 获取图像的宽*/
    int height = ui->ImageLabel->height();				/* 获取图像的高*/
    int threshold = 40;									/* 图像锐化的阈值，可以根据需要修改这个阈值*/
    QImage sharpen(width, height, QImage::Format_ARGB32);	/*图像显示的格式 */
    int r, g, b, gradientR, gradientG, gradientB;			/* 定义一个像素点的r、g、b分量*/
    QRgb rgb00, rgb01, rgb10;								/* 定义三个QRgb对象，用于处理r、g、b分量 */
    for(int i = 0; i < width; i++)							/* 遍历所有的列*/
    {
        for(int j= 0; j < height; j++)						/* 遍历所有的行*/	
        {
            if(image.valid(i, j) &&							/*Returns true if QPoint(x, y) is a valid coordinate pair within the image; otherwise returns false*/
                    image.valid(i+1, j) &&
                    image.valid(i, j+1))					/* 图像是否有效*/	
            {
                rgb00 = image.pixel(i, j);                  /* 获取图像的像素点(i, j) */
                rgb01 = image.pixel(i, j+1);				/* 获取图像的像素点(i, j+1) */
                rgb10 = image.pixel(i+1, j);				/* 获取图像的像素点(i+1, j) */
                
				r = qRed(rgb00);							/* 将这几个像素点的R、G、B颜色分量从新组合 */
                g = qGreen(rgb00);							/* 将这几个像素点的R、G、B颜色分量从新组合 */
                b = qBlue(rgb00);							/* 将这几个像素点的R、G、B颜色分量从新组合 */
                gradientR = abs(r - qRed(rgb01)) + abs(r - qRed(rgb10));/* 将这几个像素点的R、G、B颜色分量从新组合 */
                gradientG = abs(g - qGreen(rgb01)) +
                        abs(g - qGreen(rgb10));							/* 将这几个像素点的R、G、B颜色分量从新组合 */
                gradientB = abs(b - qBlue(rgb01)) +
                        abs(b - qBlue(rgb10));							/* 将这几个像素点的R、G、B颜色分量从新组合 */

                if(gradientR > threshold)								/* 如果合成之后的R分量大于阈值 */
                {
                    r = qMin(gradientR + 100, 255);						/* 取(gradientR + 100, 255)之间的最小值 */
                }

                if(gradientG > threshold)								/* 如果合成之后的G分量大于阈值 */
                {
                    g = qMin( gradientG + 100, 255);					/* 取(gradientG + 100, 255)之间的最小值 */
                }

                if(gradientB > threshold)								/* 如果合成之后的B分量大于阈值 */
                {
                    b = qMin( gradientB + 100, 255);					/* 取(gradientB + 100, 255)之间的最小值 */
                }

                sharpen.setPixel(i, j, qRgb(r, g, b));					/* 将像素点(i, j)的值设置为qRgb(r, g, b) */
            }
        }
    }
    ui->ImageLabel->setPixmap(QPixmap::fromImage(sharpen));				/* 将处理之后的图像数据刷新到ImageLabel控件上*/
}


void StreamerClient::ImageSoften()									/* 图像柔化处理的算法实现*/
{
    QImage image;
    image.loadFromData(FrameArray);									/* 从FrameArray中载入一帧图像数据*/
    if(image.isNull())												/* 如果图像数据为空*/
    {
        qDebug() << "load image data failed! ";
        return;
    }
    int width = ui->ImageLabel->width();			/* 获取图像的宽*/
    int height = ui->ImageLabel->height();			/* 获取图像的高*/
    int r, g, b;
    QRgb color;
    int xLimit = width - 1;							
    int yLimit = height - 1;
    for(int i = 1; i < xLimit; i++)					/* 遍历所有列*/
    {
        for(int j = 1; j < yLimit; j++)				/* 遍历所有行*/
        {
            r = 0;
            g = 0;
            b = 0;
            for(int m = 0; m < 9; m++)				/* 下面是具体柔化算法的实现，就是将每个像素点的R、G、B三个分量按照柔化算法从新设置 */
            {
                int s = 0;
                int p = 0;
                switch(m)
                {
                case 0:
                    s = i - 1;
                    p = j - 1;
                    break;
                case 1:
                    s = i;
                    p = j - 1;
                    break;
                case 2:
                    s = i + 1;
                    p = j - 1;
                    break;
                case 3:
                    s = i + 1;
                    p = j;
                    break;
                case 4:
                    s = i + 1;
                    p = j + 1;
                    break;
                case 5:
                    s = i;
                    p = j + 1;
                    break;
                case 6:
                    s = i - 1;
                    p = j + 1;
                    break;
                case 7:
                    s = i - 1;
                    p = j;
                    break;
                case 8:
                    s = i;
                    p = j;
                }
                color = image.pixel(s, p);
                r += qRed(color);
                g += qGreen(color);
                b += qBlue(color);
            }

            r = (int) (r / 9.0);
            g = (int) (g / 9.0);
            b = (int) (b / 9.0);

            r = qMin(255, qMax(0, r));
            g = qMin(255, qMax(0, g));
            b = qMin(255, qMax(0, b));

            image.setPixel(i, j, qRgb(r, g, b));			/* 将像素点(i, j)的值设置为qRgb(r, g, b) */
        }
    }
    ui->ImageLabel->setPixmap(QPixmap::fromImage(image));	/* 将处理之后的图像数据刷新到ImageLabel控件上*/
}

void StreamerClient::ImageGrayScale()						/* 图像灰度处理的算法实现*/
{
    QImage image;
    image.loadFromData(FrameArray);							/* 从FrameArray中载入一帧图像数据*/
    if(image.isNull())										/* 如果图像数据为空*/
    {
        qDebug() << "load image data failed! ";
        return;
    }
    qDebug() << "depth - " << image.depth();				/* 输出图像的深度，就是每个像素点的位宽 */

    int width = ui->ImageLabel->width();					/* 获取图像的宽*/
    int height = ui->ImageLabel->height();					/* 获取图像的高*/
    QRgb color;
    int gray;
    for(int i = 0; i < width; i++)							/* 遍历所有列*/
    {
        for(int j= 0; j < height; j++)						/* 遍历所有行*/
        {
            color = image.pixel(i, j);						/* 获取像素点(i, j)*/
            gray = qGray(color);							/* 获取像素点(i, j)的灰度值*/	
            image.setPixel(i, j,
                           qRgba(gray, gray, gray, qAlpha(color)));	/* 用qRgba(gray, gray, gray, qAlpha(color))重新设置这个像素点的值*/
        }
    }

    ui->ImageLabel->setPixmap(QPixmap::fromImage(image));			/* 将处理之后的图像数据刷新到ImageLabel控件上*/
}
