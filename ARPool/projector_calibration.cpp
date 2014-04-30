#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif

using namespace cv;
using namespace std;

#define VIDEOPATH 0

int mainOFF3()
{
	const string inputSettingsFile = "out_camera_data.xml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
	if (!fs.isOpened())
    {
        cout << "Could not open the calibration data file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }
	Mat cameraMatrix, distCoeffs;
	fs["Camera_Matrix"]>>cameraMatrix;
	fs["Distortion_Coefficients"]>>distCoeffs;
	Mat chess = imread("pattern.png");
	resize(chess,chess,Size(1400,1050));
	Size chessSize = cvSize(9,6);
	bool fullscreen=false;

	VideoCapture videocap;
	videocap.open(VIDEOPATH);

	if( !videocap.isOpened() )
    {
        puts("***Could not initialize capturing...***\n");
        return 0;
    }

	Mat frame;
	for(int i =0;i<10;i++)
	{
		videocap>>frame;
		if(frame.data) break;
	}
	if(!frame.data)
	{
        puts("***Could not read from capture...***\n");
        return 0;
    }
	vector<Point2f> pointBufloc;
	vector<Point2f> pointBufproj;
	vector<Point2f> reprojectionBuf;
	bool loop = true;
	Mat res;
	bool calib=false;
	cvNamedWindow("Chessboard", CV_WINDOW_NORMAL);
	while(loop)
	{
		videocap>>frame;
		Mat temp = frame.clone();
		undistort(temp,frame,cameraMatrix,distCoeffs);

		int baseLine = 0;
		char *msg="Press c to calibrate!";
		Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(frame.cols - 2*textSize.width - 10, frame.rows - 2*baseLine - 10);
		const Scalar GREEN(0,255,0);
		putText( frame, msg, textOrigin, 1, 1, GREEN);

		Mat thresh;
		cvtColor(frame,thresh,CV_BGR2GRAY);
		adaptiveThreshold(thresh,thresh,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,55,0);
		imshow("THRESH",thresh);

		bool found = findChessboardCorners( thresh, chessSize, pointBufproj, CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);

		if(!calib)
			drawChessboardCorners(frame,chessSize,Mat(pointBufproj),found);
		
		vector<Point2f> point;
		reprojectionBuf.clear();

		if (res.data)
		{
			/*for(int j=0;j<pointBufproj.size();j++)
			{
				Mat aux = res*Mat(pointBufproj[j], false);
				aux.copyTo(Mat(point, false));
				reprojectionBuf.push_back(point);
			}*/
			Mat invres = res.inv();
			if(found)
			{
				perspectiveTransform(pointBufproj,reprojectionBuf,res);
				cout<<pointBufproj.size()<<endl;
				drawChessboardCorners(chess,chessSize,Mat(reprojectionBuf),found);
			}
		}

		imshow("Chessboard",chess);
		imshow("Capture",frame);
		char c=waitKey(50);
		if(c=='f'||c=='F')
			fullscreen=!fullscreen;
		if(fullscreen)
			cvSetWindowProperty("Chessboard", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		else 
			cvSetWindowProperty("Chessboard", CV_WND_PROP_AUTOSIZE, CV_WINDOW_NORMAL);
		if(c=='c'||c=='C')
		{
			calib=true;
			if(found)
			{
				findChessboardCorners( chess, chessSize, pointBufloc,CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);
				res = findHomography(pointBufproj,pointBufloc);
				//save matrix
				FileStorage storage("projector_calib.yml", cv::FileStorage::WRITE);
				storage << "projector_correction" << res;
				storage.release(); 
				cout<<"Calibration Success!"<<endl;
			}
			else cout<<"Chessboard not found, please try again."<<endl;
		}
		if(c == 27)
			return 0;
	}


	return 0;
}