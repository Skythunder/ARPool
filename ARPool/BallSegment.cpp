#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <time.h>

using namespace std;
using namespace cv;

Mat backsub2(Mat image,Mat background,float alpha)
{
	int rows = image.rows;
	int cols = image.cols;
	for(int i=0;i<rows;i++)
	{
		for(int j=0; j<cols;j++)
		{
			background.at<float>(i,j)=(background.at<float>(i,j)*(1-alpha))+float(alpha*image.at<uchar>(i,j));
		}
	}
	//threshold(background,background,100,255,0);
	return background;
}
Mat backsub(Mat image,Mat background,float alpha,Mat mask=Mat())
{
	accumulateWeighted(image,background,alpha,mask);
	Mat aux;
	background.convertTo(aux, CV_8UC1);
	subtract(image,aux,aux);
	return aux;
}
Mat colorbacksub(Mat image,Mat background,float alpha,Mat mask=Mat())
{
	accumulateWeighted(image,background,alpha,mask);
	Mat aux;
	subtract(image,background,aux,mask,CV_8UC1);
	cvtColor(aux,aux,CV_BGR2GRAY);
	threshold(aux,aux,10,255,THRESH_BINARY);
	return aux;
}

int backsubtest()
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
    Mat frame,foreground,image,edges,canny,mogcanny,mask;
	int balpha=166;
	int bbeta=171;
	cap>>frame;
	mask=Mat::zeros(frame.size(),CV_32FC1);
	//Acelerar a aprendizagem da mascara
	image=frame.clone();
	GaussianBlur(image,image,Size(3, 3), 2, 2 );
	Canny(image,canny,balpha,bbeta);
	mask=canny.clone();
	mask.convertTo(mask,CV_32FC1);
	//
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
		GaussianBlur(image,image,Size(3, 3), 2, 2 );
		Canny(image,canny,balpha,bbeta);
		foreground=backsub(canny,mask,0.001);
        imshow( "Capture ",frame );
		imshow("Foreground ",foreground);
		char c = (char)waitKey(1);
        if( c == 27 )   
            break;

    }
	return 0;
}

int newBallSegmentation()
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
	namedWindow( "Canny ", CV_WINDOW_AUTOSIZE );
	Mat frame,image,canny,mask,mogmask;
	Mat mcanny,fcanny;
	int cannymin,cannymax,minsize;
	minsize=5;
	cannymin=166;
	cannymax=171;
	int alpha=0.001;

	cap>>frame;
	mcanny=Mat::zeros(frame.size(),CV_32FC1);
	/*Canny(frame,mcanny,cannymin,cannymax);
	mcanny.convertTo(mcanny,CV_32FC1);*/

	BackgroundSubtractorMOG2 mog,cmog;
	mask=Mat(frame.size(),CV_8UC1);
	mogmask=Mat(frame.size(),CV_8UC1);
	for(;;)
    {
        cap>>frame; 
		/*for(int i=0;i<10;i++)
		{
			if(frame.empty())
				cap>>frame;
		}*/
        if( frame.empty() ) break;
		image=frame.clone();
		GaussianBlur(image,image,Size(3, 3), 2, 2 );
		Canny(image,canny,cannymin,cannymax);

		//fcanny=backsub(canny,mcanny,alpha);
		cmog(canny,fcanny,0.001);

		//clean mask
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		Mat cleanMask = Mat::zeros(mask.size(),CV_8UC1);
		findContours(mask,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(mask.size(),CV_8UC1);
			if(contours[i].size()>minsize)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(fcanny,aux,aux);//change to bgscanny
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per>0.05)
				{
					drawContours( cleanMask, contours, i, color, -1, 8, hierarchy, 0, Point());
				}
			}
		}
		mask=cleanMask.clone();

		//clean MOG
		mog(image,mogmask,-1);
		cleanMask = Mat::zeros(mogmask.size(),CV_8UC1);
		findContours(mogmask,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(mask.size(),CV_8UC1);
			if(contours[i].size()>minsize)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(fcanny,aux,aux);//change to bgscanny
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per>0.01)
				{
					drawContours( cleanMask, contours, i, color, -1, 8, hierarchy, 0, Point());
				}
			}
		}
		mogmask=cleanMask.clone();

		//build final mask
		cleanMask = Mat::zeros(mogmask.size(),CV_8UC1);
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(mask.size(),CV_8UC1);
			if(contours[i].size()>minsize)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(mask,aux,aux);
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per<0.1)
				{
					drawContours( mask, contours, i, color, -1, 8, hierarchy, 0, Point());
				}
			}
		}

		imshow("Capture ",frame);
		imshow("Foreground ",mask);
		imshow("Canny ",fcanny);
		char c = (char)waitKey(1);
        if( c == 27 )   
            break;
	}
	return 0;
}

int colorbacksubtest()
{
    VideoCapture cap;
    //cap.open(0); 
    //cap.open("pool.avi"); 
	cap.open("vid1.mp4"); 
    if( !cap.isOpened() )
    {

        puts("***Could not initialize capturing...***\n");
        return 0;
    }
    namedWindow( "Capture ", CV_WINDOW_AUTOSIZE);
    namedWindow( "Foreground ", CV_WINDOW_AUTOSIZE );
    Mat frame,foreground,image,edges,canny,mogcanny,mask;
	int balpha=166;
	int bbeta=171;
	cap>>frame;
	mask=Mat::zeros(frame.size(),CV_32FC3);
	frame.convertTo(mask,CV_32FC3);
	foreground=Mat::zeros(mask.size(),CV_8UC1);
    for(;;)
    {
        cap>>frame; 
		for(int i=0;i<10;i++)
		{
			if(frame.empty())
				cap>>frame;
		}
        if( frame.empty() )
                break;
        image=frame.clone();
		GaussianBlur(image,image,Size(3, 3), 2, 2 );
		//Canny(image,canny,balpha,bbeta);
		Mat aux;
		bitwise_not(foreground,aux);
		foreground=colorbacksub(image,mask,0.1);
        imshow( "Capture ",frame );
		imshow("Foreground ",foreground);
		char c = (char)waitKey(1);
        if( c == 27 )   
            break;

    }
	return 0;
}

int main()
{
	//backsubtest();
	newBallSegmentation();
	//colorbacksubtest();
}