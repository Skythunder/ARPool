/*Creates final calibration matrix. Press 'f' for fullscreen followed by 'c' for calibration.
* Dependent on camera and projector position. Must recalibrate everytime either is moved.
*/

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
#define PROJECTOR_RESOLUTION Size(1400,1050)

int mainOFF3()//set to main
{
	//Loading camera calibration data
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
	//preparing Chessboard pattern
	Mat chess = imread("pattern.png");
	resize(chess,chess,PROJECTOR_RESOLUTION);
	Size chessSize = cvSize(9,6);
	bool fullscreen=false;
	//Initializing camera
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
	//preparing variables
	vector<Point2f> pointBufloc;
	vector<Point2f> pointBufproj;
	vector<Point2f> reprojectionBuf;
	bool loop = true;
	Mat res;
	bool calib=false;
	int times=0;
	cvNamedWindow("Chessboard", CV_WINDOW_NORMAL);
	//main loop
	while(loop)
	{
		//loading and undistorting frame
		videocap>>frame;
		Mat temp = frame.clone();
		undistort(temp,frame,cameraMatrix,distCoeffs);
		//printing text on screen
		int baseLine = 0;
		char *msg="Press 'f' for fullscreen. Press 'c' to calibrate!";
		Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(frame.cols - 2*textSize.width - 10, frame.rows - 2*baseLine - 10);
		const Scalar GREEN(0,255,0);
		putText( frame, msg, textOrigin, 1, 1, GREEN);
		//preparing threshold image for improved corner detection
		Mat thresh;
		cvtColor(frame,thresh,CV_BGR2GRAY);
		adaptiveThreshold(thresh,thresh,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,55,0);
		imshow("THRESH",thresh);
		//finding chessboard corners visible from camera
		bool found = findChessboardCorners( thresh, chessSize, pointBufproj, CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);
		//if not calibrated show detection
		if(!calib)
			drawChessboardCorners(frame,chessSize,Mat(pointBufproj),found);
		//if reprojection matrix is set, transform points and show reprojected corners
		reprojectionBuf.clear();
		if (res.data)
		{
			Mat invres = res.inv();
			if(found)
			{
				perspectiveTransform(pointBufproj,reprojectionBuf,res);
				if(times==1)
				{
					times++;
					drawChessboardCorners(chess,chessSize,Mat(reprojectionBuf),found);
				}
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
			cvSetWindowProperty("Chessboard", CV_WND_PROP_AUTOSIZE, CV_WINDOW_NORMAL);//not working
		if(c=='c'||c=='C')
		{
			calib=true;
			if(found)
			{
				times++;
				if(times>1)
				{
					times =1;
					chess = imread("pattern.png");
					resize(chess,chess,PROJECTOR_RESOLUTION);
				}
				//if calibrating, find corners of original image and calculate homography between local image and camera image
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