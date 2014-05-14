/*Small test program to validate calibration. Find object by color (hsv calibration by user) 
*and print blue circle around it.
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

//GLOBAL
int maxVal=255;
int h_slider=100;
int s_slider=140;
int v_slider=140;
int h=100;
int s=140;
int v=140;

//Trackbar Callbacks
void on_H_trackbar( int, void* )
{
	h=h_slider;
}
void on_S_trackbar( int, void* )
{
	s=s_slider;
}
void on_V_trackbar( int, void* )
{
	v=v_slider;
}

int mainOFF()//set to main
{
	//load calibration matrix
	const string inputSettingsFile = "projector_calib.yml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
	if (!fs.isOpened())
    {
        cout << "Could not open the calibration data file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }
	Mat rotationMatrix;
	fs["projector_correction"]>>rotationMatrix;
	fs.release();
	//initialize camera
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
	//set vars, trackbars, windows
	bool loop=true;
	cvNamedWindow("View", CV_WINDOW_NORMAL);
	cvNamedWindow("Final", CV_WINDOW_NORMAL);
	cvSetWindowProperty("Final", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
	createTrackbar("Hue","View",&h_slider,maxVal,on_H_trackbar);
	createTrackbar("Sat","View",&s_slider,maxVal,on_S_trackbar);
	createTrackbar("Val","View",&v_slider,maxVal,on_V_trackbar);
	//main loop
	while(loop)
	{
		//read frame from camera
		videocap>>frame;
		//hsv threshold and noise removal
		Mat hsv,thresh;
		cvtColor(frame,hsv,CV_BGR2HSV);
		GaussianBlur(hsv,hsv,Size(3,3),1);
		inRange(hsv,Scalar(h,s,v),Scalar(maxVal,maxVal,maxVal),thresh);
		erode(thresh,thresh,getStructuringElement(MORPH_RECT, Size(5, 5)) );
		dilate(thresh,thresh,getStructuringElement(MORPH_RECT, Size(5, 5)) );
		//find contours
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		int largest_area = -1;
		int index = -1;
		Mat thresh2=thresh.clone();
		findContours( thresh2, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		//Prepare final projection image
		Mat fin(Size(1400,1050),CV_8UC3);
		// Get the moments
		vector<Moments> mu(contours.size());
		for( int i = 0; i < contours.size(); i++ )
		{ 
			mu[i] = moments( contours[i], false );
		}
		// Get the mass centers:
		vector<Point2f> mc( contours.size() );
		for( int i = 0; i < contours.size(); i++ )
		{ 
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); 
		}
		//draw circles
		for( int i = 0; i < contours.size(); i++ )
		{ 
			circle(fin,mc[i],40,Scalar(255,0,0),2);
		}
		//calibrate image for projection
		warpPerspective(fin,fin,rotationMatrix,fin.size());//show circles
		//warpPerspective(thresh,fin,rotationMatrix,fin.size());//show hsv threshold overlay
		//thresh.copyTo(fin);//show decalibrated threshold overlay
		
		imshow("Final",fin);
		imshow("Result",thresh);
		imshow("View",frame);
		char c=waitKey(50);
		/*if(c=='f'||c=='F')
			cvSetWindowProperty("Final", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);*/
		if(c==27)
		{
			loop=false;
			break;
		}
	}


	return 0;
}