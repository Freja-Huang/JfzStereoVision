#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define CAML 1     //��������ͷ�����ţ��Լ���һ��
#define CAMR 0
#define WIDTH  352 //����ͷ�ֱ�������
#define HEIGHT 288

int cnt = 1;//����ͼƬ����
char Leftname[25];//����ͼ���ļ���
char Rightname[25];

CvCapture *cap1 = NULL, *cap2 = NULL; //capture1Ϊleft, capture2Ϊright
Mat disp, disp8u, pointClouds, imageLeft, imageRight, disparityImage;
Mat depth;//����������Mat
bool left_mouse = false;//���������±�־λ

bool    m_Calib_Data_Loaded;        // �Ƿ�ɹ����붨�����
Mat m_Calib_Mat_Q;              // Q ����
Mat m_Calib_Mat_Remap_X_L;      // ����ͼ����У����������ӳ����� X
Mat m_Calib_Mat_Remap_Y_L;      // ����ͼ����У����������ӳ����� Y
Mat m_Calib_Mat_Remap_X_R;      // ����ͼ����У����������ӳ����� X
Mat m_Calib_Mat_Remap_Y_R;      // ����ͼ����У����������ӳ����� Y
Mat m_Calib_Mat_Mask_Roi;       // ����ͼУ�������Ч����
Rect m_Calib_Roi_L;             // ����ͼУ�������Ч�������
Rect m_Calib_Roi_R;             // ����ͼУ�������Ч�������

double          m_FL;//ĳ����
int pic_info[2];//����ʱx��y��������

StereoBM    m_BM;
int m_numberOfDisparies;            // �Ӳ�仯��Χ�����ͼα��ɫ��ʱҪ��

//�㷨�������켣�����ڲ���
int SGBM_SADWindowSize = 10;
int SGBM_numberOfDisparities = 64;
int SGBM_uniquenessRatio = 15;

//���򵥵����ĳ��pixel�����أ�����Ӧ����ά�����꣬���ص�ת���꣬Z����ʾ���,x��yС��ͼƬ������0��ʼ
void PixelToAxis(Mat xyz, int x, int y)
{
	Point p;
	p.x = x; p.y = y;//С��ͼƬ����
	//cout << x << "," << y << " ��������: " << xyz.at<Vec3f>(p) << endl;
	/**********OpenCV����תʵ�ʾ��봦����**********/
	// ��ȡ���ͼ��
	vector<Mat> xyzSet;
	split(xyz, xyzSet);//xyz��ͨ������
	xyzSet[2].copyTo(depth);//������ͨ����Ϊ���ͼ
	float distance;
	distance = depth.at<float>(p);//distance������xyz.at<Vec3f>(p)�����еõ���zֵ
	// ���Իع�ķ�ʽ��ӳ��ʵ�ʾ���
	distance = distance*(-1000);//����-1000��
	distance = 0.4164*distance + (-9.2568);
	cout << x << "," << y << " ����: " << distance << " cm" << endl;
}

static void onMouse(int event, int x, int y, int /*flags*/, void* /*param*/)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		pic_info[0] = x;
		pic_info[1] = y;
		PixelToAxis(pointClouds, x, y);//����ת����
		left_mouse = true;
	}
	else if (event == CV_EVENT_LBUTTONUP)
	{
		left_mouse = false;
	}
	else if ((event == CV_EVENT_MOUSEMOVE) && (left_mouse == true))
	{
	}
}

//CreateTrackbar�Ļص�����
int SADWindowSizeValue = 10; //SADWindowSizeValueֵ
static void SADWindowSizeControl(int, void *)
{
	if (SADWindowSizeValue < 5)
	{
		SADWindowSizeValue = 5;
		SGBM_SADWindowSize = SADWindowSizeValue;
	}
	else if (SADWindowSizeValue % 2 == 0)
	{
		SADWindowSizeValue += 1;
		SGBM_SADWindowSize = SADWindowSizeValue;
	}
	else
	{
		SGBM_SADWindowSize = SADWindowSizeValue;
	}
}

int numberOfDisparitiesValue = 64; //numberOfDisparitiesֵ
static void numberOfDisparitiesControl(int, void *)
{
	while (numberOfDisparitiesValue % 16 != 0 || numberOfDisparitiesValue == 0)
	{
		numberOfDisparitiesValue++;
	}
	SGBM_numberOfDisparities = numberOfDisparitiesValue;
}

int uniquenessRatioValue = 15; //uniquenessRatioֵ
static void uniquenessRatioControl(int, void *)
{

	SGBM_uniquenessRatio = uniquenessRatioValue;
}


int loadCalibData()
{
	// ��������ͷ������� Q roi1 roi2 mapx1 mapy1 mapx2 mapy2
	try
	{
		cv::FileStorage fs("calib_paras.xml", cv::FileStorage::READ);
		cout << fs.isOpened() << endl;

		if (!fs.isOpened())
		{
			return (0);
		}

		cv::Size imageSize;
		cv::FileNodeIterator it = fs["imageSize"].begin();

		it >> imageSize.width >> imageSize.height;

		vector<int> roiVal1;
		vector<int> roiVal2;

		fs["leftValidArea"] >> roiVal1;

		m_Calib_Roi_L.x = roiVal1[0];
		m_Calib_Roi_L.y = roiVal1[1];
		m_Calib_Roi_L.width = roiVal1[2];
		m_Calib_Roi_L.height = roiVal1[3];

		fs["rightValidArea"] >> roiVal2;
		m_Calib_Roi_R.x = roiVal2[0];
		m_Calib_Roi_R.y = roiVal2[1];
		m_Calib_Roi_R.width = roiVal2[2];
		m_Calib_Roi_R.height = roiVal2[3];


		fs["QMatrix"] >> m_Calib_Mat_Q;
		fs["remapX1"] >> m_Calib_Mat_Remap_X_L;
		fs["remapY1"] >> m_Calib_Mat_Remap_Y_L;
		fs["remapX2"] >> m_Calib_Mat_Remap_X_R;
		fs["remapY2"] >> m_Calib_Mat_Remap_Y_R;

		cv::Mat lfCamMat;
		fs["leftCameraMatrix"] >> lfCamMat;
		m_FL = lfCamMat.at<double>(0, 0);

		m_Calib_Mat_Q.at<double>(3, 2) = -m_Calib_Mat_Q.at<double>(3, 2);

		m_Calib_Mat_Mask_Roi = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC1);
		cv::rectangle(m_Calib_Mat_Mask_Roi, m_Calib_Roi_L, cv::Scalar(255), -1);

		m_BM.state->roi1 = m_Calib_Roi_L;
		m_BM.state->roi2 = m_Calib_Roi_R;

		m_Calib_Data_Loaded = true;

		string method;
		fs["rectifyMethod"] >> method;
		if (method != "BOUGUET")
		{
			return (-2);
		}

	}
	catch (std::exception& e)
	{
		m_Calib_Data_Loaded = false;
		return (-99);
	}

	return 1;


}

int getDisparityImage(cv::Mat& disparity, cv::Mat& disparityImage, bool isColor)
{
	// ��ԭʼ�Ӳ����ݵ�λ��ת��Ϊ 8 λ
	cv::Mat disp8u;
	if (disparity.depth() != CV_8U)
	{
		if (disparity.depth() == CV_8S)
		{
			disparity.convertTo(disp8u, CV_8U);
		}
		else
		{
			disparity.convertTo(disp8u, CV_8U, 255 / (m_numberOfDisparies*16.));
		}
	}
	else
	{
		disp8u = disparity;
	}

	// ת��Ϊα��ɫͼ�� �� �Ҷ�ͼ��
	if (isColor)
	{
		if (disparityImage.empty() || disparityImage.type() != CV_8UC3 || disparityImage.size() != disparity.size())
		{
			disparityImage = cv::Mat::zeros(disparity.rows, disparity.cols, CV_8UC3);
		}

		for (int y = 0; y < disparity.rows; y++)
		{
			for (int x = 0; x < disparity.cols; x++)
			{
				uchar val = disp8u.at<uchar>(y, x);
				uchar r, g, b;

				if (val == 0)
					r = g = b = 0;
				else
				{
					r = 255 - val;
					g = val < 128 ? val * 2 : (uchar)((255 - val) * 2);
					b = val;
				}

				disparityImage.at<cv::Vec3b>(y, x) = cv::Vec3b(r, g, b);

			}
		}
	}
	else
	{
		disp8u.copyTo(disparityImage);
	}

	return 1;
}

Mat Match_BM(Mat left, Mat right, Rect roi1, Rect roi2)//BMƥ���㷨������left��rightΪ�Ҷ�ͼ,roi1��roi2Ϊ������ͼ����Ч��������һ����˫ĿУ���׶ε�cvStereoRectify �������ݣ����disp8Ϊ�Ҷ�ͼ
{
	StereoBM bm;
	int SADWindowSize = 21;//��ҪӰ�����
	int numberOfDisparities = 64;//��ҪӰ�����
	int uniquenessRatio = 3;//��ҪӰ�����
	bm.state->roi1 = roi1;
	bm.state->roi2 = roi2;
	bm.state->preFilterCap = 63;
	bm.state->SADWindowSize = SADWindowSize > 0 ? SADWindowSize : 9;
	bm.state->minDisparity = 0;
	bm.state->numberOfDisparities = numberOfDisparities;
	bm.state->textureThreshold = 29;
	bm.state->uniquenessRatio = uniquenessRatio;
	bm.state->speckleWindowSize = 200;
	bm.state->speckleRange = 32;
	bm.state->disp12MaxDiff = 2;
	Mat disp, disp8;
	int64 t = getTickCount();
	bm(left, right, disp);
	t = getTickCount() - t;
	cout << "BM��ʱ:" << t * 1000 / getTickFrequency() << "���룬��" << t / getTickFrequency() << "��" << endl;
	disp.convertTo(disp8, CV_8U, 255 / (numberOfDisparities*16.));
	return disp8;
}

Mat Match_SGBM(Mat left, Mat right)//SGBMƥ���㷨������left��rightΪ�Ҷ�ͼ�����disp8Ϊ�Ҷ�ͼ
{
	StereoSGBM sgbm;
	int SADWindowSize = SGBM_SADWindowSize;//��ҪӰ�����
	int numberOfDisparities = SGBM_numberOfDisparities;//��ҪӰ�����
	int uniquenessRatio = SGBM_uniquenessRatio;//��ҪӰ�������Խ����ƥ��ԽС������ƥ������Խ��
	int cn = left.channels();
	sgbm.preFilterCap = 63;
	sgbm.SADWindowSize = SADWindowSize > 0 ? SADWindowSize : 3;
	sgbm.P1 = 8 * cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
	sgbm.P2 = 32 * cn*sgbm.SADWindowSize*sgbm.SADWindowSize;
	sgbm.minDisparity = 0;
	sgbm.numberOfDisparities = numberOfDisparities;
	sgbm.uniquenessRatio = uniquenessRatio;
	sgbm.speckleWindowSize = 100;
	sgbm.speckleRange = 32;
	sgbm.disp12MaxDiff = 1;
	Mat disp, disp8;
	int64 t = getTickCount();
	sgbm(left, right, disp);
	t = getTickCount() - t;
	cout << "SGBM��ʱ:" << t * 1000 / getTickFrequency() << "���룬��" << t / getTickFrequency() << "��" << endl;
	disp.convertTo(disp8, CV_8U, 255 / (numberOfDisparities*16.));
	return disp8;
}

Mat Match_Var(Mat left, Mat right)//Var����ƥ���㷨(OpenCV2.3����)������left��rightΪ�Ҷ�ͼ�����disp8Ϊ�Ҷ�ͼ
{
	StereoVar var;
	int numberOfDisparities = 128;//Ӱ�����64
	var.levels = 3;//������ʹ���Զ�����(USE_AUTO_PARAMS)
	var.pyrScale = 0.5;//������ʹ���Զ�����(USE_AUTO_PARAMS)
	var.nIt = 25;
	var.minDisp = -numberOfDisparities;
	var.maxDisp = 0;
	var.poly_n = 3;
	var.poly_sigma = 0.0;
	var.fi = 15.0f;
	var.lambda = 0.03f;
	var.penalization = var.PENALIZATION_TICHONOV; //������ʹ���Զ�����(USE_AUTO_PARAMS)
	var.cycle = var.CYCLE_V;// ������ʹ���Զ�����(USE_AUTO_PARAMS)
	var.flags = var.USE_SMART_ID | var.USE_AUTO_PARAMS | var.USE_INITIAL_DISPARITY | var.USE_MEDIAN_FILTERING;
	Mat disp, disp8;
	int64 t = getTickCount();
	var(left, right, disp);
	t = getTickCount() - t;
	cout << "Var��ʱ:" << t * 1000 / getTickFrequency() << "���룬��" << t / getTickFrequency() << "��" << endl;
	disp.convertTo(disp8, CV_8U);//ע��Var�㷨�����������ﲻһ��
	return disp8;
}

void updatebm()
{
	m_BM.state->preFilterSize = 63;
	m_BM.state->preFilterCap = 63;
	m_BM.state->SADWindowSize = 21;
	m_BM.state->minDisparity = 0;
	m_BM.state->numberOfDisparities = 64;
	m_BM.state->textureThreshold = 29;
	m_BM.state->uniquenessRatio = 3;
	m_BM.state->speckleWindowSize = 200;
	m_BM.state->speckleRange = 32;
	m_BM.state->disp12MaxDiff = 2;

}

int  bmMatch(cv::Mat& frameLeft, cv::Mat& frameRight, cv::Mat& disparity, cv::Mat& imageLeft, cv::Mat& imageRight)
{
	Mat Left_processing, Right_processing;
	// ������
	if (frameLeft.empty() || frameRight.empty())
	{
		disparity = cv::Scalar(0);
		return 0;
	}
	if (WIDTH == 0 || HEIGHT == 0)
	{
		return 0;
	}

	// ת��Ϊ�Ҷ�ͼ
	cv::Mat img1proc, img2proc;
	cvtColor(frameLeft, img1proc, CV_BGR2GRAY);
	cvtColor(frameRight, img2proc, CV_BGR2GRAY);

	// У��ͼ��ʹ������ͼ�ж���    
	cv::Mat img1remap, img2remap;

	if (m_Calib_Data_Loaded)
	{
		remap(img1proc, img1remap, m_Calib_Mat_Remap_X_L, m_Calib_Mat_Remap_Y_L, cv::INTER_LINEAR);     // �������Ӳ����Ļ������У��
		remap(img2proc, img2remap, m_Calib_Mat_Remap_X_R, m_Calib_Mat_Remap_Y_R, cv::INTER_LINEAR);
	}
	else
	{
		img1remap = img1proc;
		img2remap = img2proc;
	}

	// ��������ͼ����߽��б߽����أ��Ի�ȡ��ԭʼ��ͼ��ͬ��С����Ч�Ӳ�����
	cv::Mat img1border, img2border;
	if (m_numberOfDisparies != m_BM.state->numberOfDisparities)
		m_numberOfDisparies = m_BM.state->numberOfDisparities;
	copyMakeBorder(img1remap, img1border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);
	copyMakeBorder(img2remap, img2border, 0, 0, m_BM.state->numberOfDisparities, 0, IPL_BORDER_REPLICATE);

	//���������ͼƬ����Ч���֡����ҹ��в���
	Rect vroiOK[3];//����Ч������Σ������󽻼�
	vroiOK[2] = m_Calib_Roi_L & m_Calib_Roi_R;//vroiOK[2]Ϊ����

	if (m_Calib_Data_Loaded)
	{
		remap(frameLeft, Left_processing, m_Calib_Mat_Remap_X_L, m_Calib_Mat_Remap_Y_L, cv::INTER_LINEAR);
		//rectangle(Left_processing, vroiOK[2], CV_RGB(0, 0, 255), 3);//��ɫ�򻭳����л���Ч����
	}
	else
		frameLeft.copyTo(Left_processing);


	if (m_Calib_Data_Loaded)
	{ 
		remap(frameRight, Right_processing, m_Calib_Mat_Remap_X_R, m_Calib_Mat_Remap_Y_R, cv::INTER_LINEAR);
		//rectangle(Right_processing, vroiOK[2], CV_RGB(0, 0, 255), 3);//��ɫ�򻭳����л���Ч����
	}
	else
		frameRight.copyTo(Right_processing);
	
	// ����ͼ���в��ּ�������
	imageLeft = Left_processing(vroiOK[2]);//left�и���Ȥ����(vroiOK[2]���ε�����)����imageROI
	imageRight = Right_processing(vroiOK[2]);//left�и���Ȥ����(vroiOK[2]���ε�����)����imageROI

	// �����Ӳ�
	Mat imageLeftGRAY, imageRihtGRAY;
	cvtColor(imageLeft, imageLeftGRAY, CV_BGR2GRAY);//����ͼ�Ҷ�ת��ɫ
	cvtColor(imageRight, imageRihtGRAY, CV_BGR2GRAY);//����ͼ�Ҷ�ת��

	////BM�㷨�����Ӳ�
	//Mat BMdisparity, BMdisparity2;
	//m_BM(imageLeftGRAY, imageRihtGRAY, BMdisparity);
	//getDisparityImage(BMdisparity, BMdisparity2, true);
	//imshow("BM�㷨", BMdisparity2);

	//Rect roi1, roi2;
	//Mat BMdisparity3, BMdisparity4;
	//BMdisparity3 = Match_BM(imageLeftGRAY, imageRihtGRAY, roi1, roi2);
	//getDisparityImage(BMdisparity3, BMdisparity4, true);
	//imshow("BM cpp�㷨", BMdisparity4);

	//SGBM�㷨�����Ӳ�
	disparity = Match_SGBM(imageLeftGRAY, imageRihtGRAY);//SGBMƥ���㷨������left��rightΪ�Ҷ�ͼ�����disp8Ϊ�Ҷ�ͼ

	////Var�㷨�����Ӳ�
	//Mat Vardisparity, Vardisparity2;
	//Vardisparity = Match_Var(imageLeftGRAY, imageRihtGRAY);
	//getDisparityImage(Vardisparity, Vardisparity2, true);
	//imshow("Var�㷨", Vardisparity2);

	return 1;
}



int getPointClouds(cv::Mat& disparity, cv::Mat& pointClouds)
{
	if (disparity.empty())
	{
		return 0;
	}
	reprojectImageTo3D(disparity, pointClouds, m_Calib_Mat_Q, true);//����������ά����
	pointClouds *= 1.6;//������������1.6��
	return 1;
}



int main(int argc, char** argv)
{
	//��������ͷ
	cap1 = cvCaptureFromCAM(CAML);
	assert(cap1 != NULL);
	cvWaitKey(100);
	cap2 = cvCaptureFromCAM(CAMR);
	assert(cap2 != NULL);
	//���û���ߴ� WIDTH��HEIGHT�ں궨���и�
	cvSetCaptureProperty(cap1, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cvSetCaptureProperty(cap1, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
	cvSetCaptureProperty(cap2, CV_CAP_PROP_FRAME_WIDTH, WIDTH);
	cvSetCaptureProperty(cap2, CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
	//������ʾ����
	namedWindow("��Left", 1);
	namedWindow("��Right", 1);
	namedWindow("�Ӳ�ͼ", 1);
	//���������
	setMouseCallback("�Ӳ�ͼ", onMouse, 0);
	//��������ͷ�����
	loadCalibData();
	cout << "˫Ŀ�궨��������ɹ���" << endl;

	/*�켣��ʹ��*/
	namedWindow("��SGBM�������ڡ�", 1);
	Mat Logo = imread("Logo.png", 1);
	imshow("��SGBM�������ڡ�", Logo);
	createTrackbar("SADWindowSize��", "��SGBM�������ڡ�", &SADWindowSizeValue, 25, SADWindowSizeControl);//���Ĳ�������������������ֵ�����ֵ���ص�����
	SADWindowSizeControl(SADWindowSizeValue, 0);//���ûص����� 
	createTrackbar("numberOfDisparities��", "��SGBM�������ڡ�", &numberOfDisparitiesValue, 256, numberOfDisparitiesControl);
	numberOfDisparitiesControl(numberOfDisparitiesValue, 0);
	createTrackbar("uniquenessRatio��", "��SGBM�������ڡ�", &uniquenessRatioValue, 50, uniquenessRatioControl);
	uniquenessRatioControl(uniquenessRatioValue, 0);


	while (true)
	{
		Mat frame1;
		Mat frame2;
		frame1 = cvQueryFrame(cap1);
		frame2 = cvQueryFrame(cap2);
		if (frame1.empty())   break;
		if (frame2.empty())   break;
		updatebm();//ƥ���㷨��������
		bmMatch(frame1, frame2, disp, imageLeft, imageRight);//����+ƥ��
		imshow("��Left", imageLeft);
		imshow("��Right", imageRight);
		getDisparityImage(disp, disparityImage, true);//trueʱ��dispת��ɫ������Ϊ�Ҷ�ͼ
		getPointClouds(disp, pointClouds);//�Ӳ�ͼ3D�����ؽ�
		imshow("�Ӳ�ͼ", disparityImage);

		char c = waitKey(100);
		if (c == '1') //��'1'�������ҽ����õ���Ƭ
		{
			sprintf(Leftname, "%02dLeft-Rectified.png", cnt);
			sprintf(Rightname, "%02dRight-Rectified.png", cnt);
			imwrite(Leftname, imageLeft);
			imwrite(Rightname, imageRight);
			cnt++;
			cout << Leftname << " �� " << Rightname << " ����ɹ���" << endl;
		}
	}
	return 0;
}

