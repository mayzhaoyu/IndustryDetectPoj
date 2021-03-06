﻿#include "WorkThread.h"

extern ofstream fout;
WorkThread::WorkThread(QObject *parent)
	: QThread(parent)
{
	//设置接收数据长度
	int width = AppConfig::getInstance()->width;
	int height = AppConfig::getInstance()->height;

	bufLength = ((width * height) / 1024 + 1) * 1024;
	//mm====>转为行
	captureDelay = (AppConfig::getInstance()->captureDelay)*11.7;

	//分配内存
	if (databuf == NULL)
	{
		databuf = new unsigned char[bufLength];
	}

	if (inbuf == NULL)
	{
		inbuf = new unsigned char[bufLength];
	}
}

WorkThread::~WorkThread()
{
	if (databuf != NULL)
	{

		delete[]databuf;
		databuf = NULL;
	}
	if (inbuf != NULL)
	{

		delete[]inbuf;
		inbuf = NULL;
	}
	if (m_pUSBDevice != NULL)
	{

		delete m_pUSBDevice;
		m_pUSBDevice = NULL;
	}

}
WorkThread* WorkThread::workthread = NULL;

WorkThread*WorkThread::getinstance()
{
	if (workthread == nullptr)
	{
		workthread = new WorkThread();
	}
	return workthread;
};

void WorkThread::run()
{
	countFlag = true;

	if (m_pUSBDevice == NULL) {
		m_pUSBDevice = new CCyUSBDevice(NULL);
	}

	if ((databuf == NULL) || (inbuf == NULL))
	{

		return;
	}

	UCHAR bufOrder[2] = { 0,0 };
	if (captureDelay >= 600 * 11.7) {
		captureDelay = 600 * 11;
	}
	int temp = captureDelay / 8;
	if (temp > 255)
	{
		if (513 > temp && temp > 255) {
			bufOrder[0] = temp - 256;
			bufOrder[1] = 0xFC + 1;
		}
		if (767 >= temp && temp >= 513) {
			bufOrder[0] = temp - 513;
			bufOrder[1] = 0xFC + 2;
		}
		if (1024 >= temp && temp > 767) {
			bufOrder[0] = temp - 767;
			bufOrder[1] = 0xFC + 3;
		}
	}
	else {
		bufOrder[0] = temp;
		bufOrder[1] = 0xFC;
	}

	UCHAR bufOrder1[2] = { 0x07,0xF0 };//结束开灯
	long Two = 2;
	long Two1 = 2;
	long L512 = 512;
	long buflength = bufLength;

	//接收数据
	isRun = true;
	//计时
	clock_t start, ends;

	while (1)
	{

		Two = 2;
		Two1 = 2;

		if (isRun == false)
		{
			MessageBox(NULL, TEXT("接受数据线程退出！"), TEXT("Warnning"), MB_OK);
			break;
		}

		m_pUSBDevice->BulkInEndPt->TimeOut = 3 * 1000; //触发信号3s   无限等待 0xffffffff

		L512 = 512;

		if (m_pUSBDevice->BulkInEndPt->XferData(bufTest512, L512))
		{
			if (bufTest512[2] == 0x99 && bufTest512[3] == 0x99 && bufTest512[4] == 0x99 && bufTest512[5] == 0x99) //判断是否接收到开启信号
			{
				isStart = true; //开启接收
			}
			else
			{
				continue;  //继续下一次检测
			}
		}
		else
		{
			isStart = false;
			continue;
		}
		//qDebug() << "start..." << endl;


		if (isStart == true)
		{
			//计时
			if (countFlag == true) {
				start = clock();
				countFlag = false;
			}
			else {
				ends = clock();
				emit updateSpeed((ends - start) / 1000.0);		//更新速度表
				countFlag = true;
			}

			//发送开灯指令
			if (m_pUSBDevice->BulkOutEndPt == NULL)		//检测usb是否断开连接
			{
				return;
			}

			if (m_pUSBDevice->BulkOutEndPt->XferData(bufOrder, Two) == false)
			{

				return;
			}
			//qDebug() << bufOrder[0] << endl;

			m_pUSBDevice->BulkInEndPt->TimeOut = 3000/*0xffffffff*/;	//等待时间设置为 3s
			if (!(m_pUSBDevice->BulkInEndPt->XferData(inbuf, buflength)))
			{
				MessageBox(NULL, TEXT("接收数据失败！"), TEXT("Warnning"), MB_OK);
				isRun = false;
			}

			//发送关灯指令
			if ((m_pUSBDevice->BulkOutEndPt->XferData(bufOrder1, Two1)) == false)
			{
				return;
			}

			if (isRun == false) {
				MessageBox(NULL, TEXT("接受数据线程退出！"), TEXT("Warnning"), MB_OK);
				break;
			}

			buflength = bufLength;
			memcpy(databuf, inbuf, buflength);		//将inbuf数据拷贝到databuf
		}


		//qDebug() <<( ends - start )<< endl;
		//数据不安全 开启拼接图像和算法识别线程
		imageGet1->modType = PROCESSIMG;
		imageGet1->data = databuf;
		imageGet1->start();

		//fout << "==============" << endl;
		//fout << "采集图像用时:" << ends - start << "ms " << endl;
	}

}


