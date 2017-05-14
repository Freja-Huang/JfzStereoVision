#include <iostream>
#include "afx.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

//��������ͷ�����ţ��Լ���һ��
#define CAML 1
#define CAMR 2

//����ͷ�ߴ�����
#define WIDTH 352
#define HEIGHT 288

CvCapture *capture1 = NULL, *capture2 = NULL; //capture1Ϊleft, capture2Ϊright
Mat  imageLeft;
Mat  imageRight;

int cnt = 1;//����ͼƬ����
char Leftname[25];//����ͼ���ļ���
char Rightname[25];

int main(int argc, char *argv[])
{
	//��������ͷ
	capture1 = cvCaptureFromCAM(CAML);
	assert(capture1 != NULL);
	cvWaitKey(100);
	capture2 = cvCaptureFromCAM(CAMR);
	assert(capture2 != NULL);

	//���û���ߴ� WIDTH��HEIGHT�ں궨���и�
	cvSetCaptureProperty(capture1, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cvSetCaptureProperty(capture1, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
	cvSetCaptureProperty(capture2, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cvSetCaptureProperty(capture2, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);

	cout << "��Ƶ�ֱ�������Ϊ��" << WIDTH << " x " << HEIGHT << endl;

	imageLeft = cvQueryFrame(capture1);
	imageRight = cvQueryFrame(capture2);

	while (true)
	{ 
		imageLeft = cvQueryFrame(capture1);
		imageRight = cvQueryFrame(capture2);
		imshow("Left", imageLeft);
		imshow("Right", imageRight);
		char c = waitKey(100);
		if (c == '1') //��'g'������Ƭ
		{
			sprintf(Leftname, "stereoData\\Left%02d.jpg", cnt);
			sprintf(Rightname, "stereoData\\Right%02d.jpg", cnt);
			imwrite(Leftname, imageLeft);
			imwrite(Rightname, imageRight);
			cnt++;
			cout << Leftname <<" �� "<<Rightname << " ����ɹ���" << endl;
		}
	}
	char c = waitKey();
	if (c == 27) //��ESC���˳�
		return 0;
}
