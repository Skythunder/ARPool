
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "package_bgs/dp/DPZivkovicAGMMBGS.h"
#include "package_bgs\StaticFrameDifferenceBGS.h"

using namespace std;
using namespace cv;

const int balpha_slider_max = 255;
int balpha_slider;
const int bbeta_slider_max = 255;
int bbeta_slider;
int balpha;
int bbeta;
const int max_min_blob_size = 255;
int min_blob_size_slider;
int min_blob_size;

void on_balpha_trackbar( int, void* )
{
 balpha = balpha_slider;
}

void on_bbeta_trackbar( int, void* )
{
 bbeta = bbeta_slider;
}

void on_min_blob_size_trackbar( int, void* )
{
 min_blob_size = min_blob_size_slider;
}

int mainXPTO()
{
    VideoCapture cap;
    //cap.open(0); 
    cap.open("pool.avi"); 
	//cap.open("vid1.mp4"); 
    if( !cap.isOpened() )
    {

        puts("***Could not initialize capturing...***\n");
        return 0;
    }
    namedWindow( "Capture ", CV_WINDOW_AUTOSIZE);
    namedWindow( "Foreground ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Edges ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Canny ", CV_WINDOW_AUTOSIZE );
    Mat frame,foreground,image,edges,canny,mogcanny,mask;
	balpha=balpha_slider=166;
	bbeta=bbeta_slider=171;
	min_blob_size=min_blob_size_slider=5;
	IBGS *mog,*mogc;
	mog = new DPZivkovicAGMMBGS();
	mogc = new DPZivkovicAGMMBGS();
	cap>>frame;
	mask=Mat::zeros(frame.size(),CV_8UC1);
	vector<vector<Point> > maskc;
    for(;;)
    {
        cap>>frame; 
		/*for(int i=0;i<10;i++)
		{
			if(frame.empty())
				cap>>frame;
		}*/
        if( frame.empty() )
                break;
        image=frame.clone();
		GaussianBlur(image,image,Size(3, 3), 2, 2 );//mais rapido q median mas menos qualidade
		mog->process(image,foreground,Mat());
		createTrackbar( "A", "Capture ", &balpha_slider, balpha_slider_max, on_balpha_trackbar );
		createTrackbar( "B", "Capture ", &bbeta_slider, bbeta_slider_max, on_bbeta_trackbar );
		createTrackbar( "Min Blob Size", "Capture ", &min_blob_size_slider, max_min_blob_size, on_min_blob_size_trackbar );
		Canny(image,canny,balpha,bbeta);
		mogc->process(canny,mogcanny,Mat());
		//int minsize = 5;//ajustar
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		mask = Mat::zeros( foreground.size(), CV_8UC1 );
		findContours(foreground,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( foreground.size(), CV_8UC1 );

		vector<Moments> mu;
		int momsize=0;
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(foreground.size(),CV_8UC1);
			if(contours[i].size()>min_blob_size)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(mogcanny,aux,aux);
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per>0.1)//percentagem de pixeis de contorno dentro de bgs
				{
					drawContours( mask, contours, i, color, -1, 8, hierarchy, 0, Point());
					mu.push_back(moments( contours[i], false ));
					momsize++;
				}
			}
		}
		Mat centers=Mat::zeros( canny.size(), CV_8UC3 );
		///  Get the mass centers:
		vector<Point2f> mc(momsize);
		for( int i = 0; i < momsize; i++ )
		{ 
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); 
		}
		//draw
		for( int i = 0; i < momsize; i++ )
		{ 
			circle(centers,mc[i],10,Scalar(255,0,0),-1);
		}

        imshow( "Capture ",frame );
		imshow("Foreground ",mask);
		//imshow("Foreground ",foreground);
		imshow("Canny ",centers);
		imshow("Edges ",mogcanny);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }
	delete mog;
	delete mogc;
	return 0;
}

int mainXPTO2()
{
    VideoCapture cap;
    //cap.open(0); 
    cap.open("pool.avi"); 
	//cap.open("vid1.mp4"); 
    if( !cap.isOpened() )
    {

        puts("***Could not initialize capturing...***\n");
        return 0;
    }
    namedWindow( "Capture ", CV_WINDOW_AUTOSIZE);
    namedWindow( "Foreground ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Edges ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Canny ", CV_WINDOW_AUTOSIZE );
    Mat frame,foreground,image,edges,canny,mogcanny,mask;
	balpha=balpha_slider=166;
	bbeta=bbeta_slider=171;
	min_blob_size=min_blob_size_slider=5;
	IBGS *mog,*mogc;
	mog = new DPZivkovicAGMMBGS();
	mogc = new DPZivkovicAGMMBGS();
	cap>>frame;
	mask=Mat::zeros(frame.size(),CV_8UC1);
	vector<vector<Point> > maskc;
    for(;;)
    {
        cap>>frame; 
		/*for(int i=0;i<10;i++)
		{
			if(frame.empty())
				cap>>frame;
		}*/
        if( frame.empty() )
                break;
        image=frame.clone();
		GaussianBlur(image,image,Size(3, 3), 2, 2 );//mais rapido q median mas menos qualidade
		mog->process(image,foreground,Mat());
		createTrackbar( "A", "Capture ", &balpha_slider, balpha_slider_max, on_balpha_trackbar );
		createTrackbar( "B", "Capture ", &bbeta_slider, bbeta_slider_max, on_bbeta_trackbar );
		createTrackbar( "Min Blob Size", "Capture ", &min_blob_size_slider, max_min_blob_size, on_min_blob_size_trackbar );
		Canny(image,canny,balpha,bbeta);
		mogc->process(canny,mogcanny,Mat());
		//int minsize = 5;//ajustar
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		mask = Mat::zeros( foreground.size(), CV_8UC1 );
		findContours(foreground,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( foreground.size(), CV_8UC1 );

		vector<Moments> mu;
		int momsize=0;
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(foreground.size(),CV_8UC1);
			if(contours[i].size()>min_blob_size)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(mogcanny,aux,aux);
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per>0.1)//percentagem de pixeis de contorno dentro de bgs
				{
					drawContours( mask, contours, i, color, -1, 8, hierarchy, 0, Point());
				}
			}
		}
		Mat mask2=mask.clone();
		findContours(mask2,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat centers=Mat::zeros( canny.size(), CV_8UC3 );
		///  Get the mass centers:
		for( int i = 0; i < contours.size(); i++ )
		{ 
			mu.push_back(moments( contours[i], false ));
			momsize++;
		}
		vector<Point2f> mc(momsize);
		for( int i = 0; i < momsize; i++ )
		{ 
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); 
		}
		//draw
		for( int i = 0; i < momsize; i++ )
		{ 
			circle(centers,mc[i],10,Scalar(255,0,0),-1);
		}

        imshow( "Capture ",frame );
		imshow("Foreground ",mask);
		//imshow("Foreground ",foreground);
		imshow("Canny ",centers);
		imshow("Edges ",mogcanny);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }
	delete mog;
	delete mogc;
	return 0;
}