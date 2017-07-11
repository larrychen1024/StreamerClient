#ifndef STREAMERCLIENT_H
#define STREAMERCLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

namespace Ui {
class StreamerClient;
}

class StreamerClient : public QMainWindow							//声明一个StreamerClient，它继承于QMainWindow类
{
    Q_OBJECT

public:
    explicit StreamerClient(QWidget *parent = 0);					//构造函数
    ~StreamerClient();												//析构函数

private:															//私有成员变量
    Ui::StreamerClient *ui;											//StreamerClient的界面指针

    QByteArray  *ImageByteArray;
    QByteArray  LastLeftArray;
    QByteArray  FrameArray;

    QByteArray  SizeArray;
    QTcpSocket  *pTcpClient;
    qint32      BufferCnt;
    bool        ok;
    qint32      ContentSize;
    QTimer      FrameTimer;
	
	/* 三个按钮状态的标志变量 */
    bool        isSharpen;
    bool        isSoften;
    bool        isGrayscale;

private:															//私有成员函数
    void RequestImage();
    void ImageSharpen();											//图像锐化处理
    void ImageSoften();												//图像柔化处理
    void ImageGrayScale();											//图像灰度处理

public slots:														//私有槽
    void ConnectToServer(void);										//连接到服务器				
    void DisconnectFromServer(void);								//从服务器断开连接
    void StreamStart(void);											//开始传输视频流
    void StreamStop(void);											//停止传输视频流

    void Snap(void);												//视频图像捕获
    void Record(void);												//视频图像录制（空的槽，功能暂时未实现）

    void ReadImage();												//读取一帧图像
    void UpdateFrame();												//将读取到的图像刷新到ImageLabel组件上
    
	void SetSharpen();												//图像锐化处理槽
    void SetSoften();												//图像柔化处理槽
    void SetGrayscale();											//图像灰度处理槽
};

#endif // STREAMERCLIENT_H
