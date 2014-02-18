#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include<time.h>

using namespace std;
using namespace cv;

const int calpha_slider_max = 255;
int calpha_slider;
const int cbeta_slider_max = 255;
int cbeta_slider;
int calpha;
int cbeta;

void on_calpha_trackbar( int, void* )
{
 calpha = calpha_slider;
}

void on_cbeta_trackbar( int, void* )
{
 cbeta = cbeta_slider;
}


int mainS()
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
    namedWindow( "Capture ", CV_WINDOW_NORMAL);
    namedWindow( "Foreground ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Edges ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Canny ", CV_WINDOW_AUTOSIZE );
    Mat frame,foreground,image,edges,canny;
	IplImage *iplframe;
	calpha=calpha_slider=166;
	cbeta=cbeta_slider=171;
    BackgroundSubtractorMOG2 mog;
    int fps=cap.get(CV_CAP_PROP_FPS);
    if(fps<=0)
        fps=10;
    else
        fps=1000/fps;
	//cap>>frame;
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
		GaussianBlur(image,image,Size(3, 3), 2, 2 );//mais rapido q median mas menos qualidade
        mog(image,foreground,-1);
		createTrackbar( "A", "Capture ", &calpha_slider, calpha_slider_max, on_calpha_trackbar );
		createTrackbar( "B", "Capture ", &cbeta_slider, cbeta_slider_max, on_cbeta_trackbar );
		Canny(image,canny,calpha,cbeta);
		Canny(foreground,edges,calpha,cbeta);
		//canny=edges.clone();
		Rect rect = Rect(42, 42, 435, 205);
		//edges = edges(rect);
		//canny = canny(rect);
		//fin=fin(rect);
		//foreground = foreground(rect);
		
		/*vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(fin,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( fin.size(), CV_8UC3 );
		int momsize=0;
		int minsize = 5;
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			if(contours[i].size()>minsize)
				drawContours( drawing, contours, i, color, -1, 8, hierarchy, 0, Point());
				momsize++;
		}

		/// Get the moments
		vector<Moments> mu(momsize);
		int j=0;
		for( int i = 0; i < contours.size(); i++ )
		{ 
			if(contours[i].size()<maxsize&&contours[i].size()>minsize)
				mu[j] = moments( contours[i], false ); 
				j++;
		}

		///  Get the mass centers:
		vector<Point2f> mc( momsize );
		for( int i = 0; i < momsize; i++ )
		{ 
			mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); 
		}
		//draw
		for( int i = 0; i < momsize; i++ )
		{ 
			circle(drawing,mc[i],10,Scalar(255,0,0),-1);
		}*/

		/*for(int i = 0;i<contours.size();i++)
		{
			printf("(%d;%d)\n",mc[i].x,mc[i].y);
		}*/
        //threshold(foreground,foreground,128,255,THRESH_BINARY);
        //medianBlur(foreground,foreground,9);//melhor mas lento
        //erode(foreground,foreground,Mat());
		//dilate(foreground,foreground,Mat());
		//foreground |= edges;
		//erode(foreground,foreground,Mat());
		//Mat for2=foreground.clone(); 
		//bitwise_not ( for2, for2 );
		//Mat match = foreground.clone(); 
		
		/*vector<Vec3f> circles;
		HoughCircles(foreground, circles, CV_HOUGH_GRADIENT,2,10,100,30,2,20);
		for( size_t i = 0; i < circles.size(); i++ )
		{
			Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			// draw the circle center
			circle( image, center, 3, Scalar(0,255,0), -1, 8, 0 );
			// draw the circle outline
			circle( image, center, radius, Scalar(0,0,255), 3, 8, 0 );
		}*/

		
		/*double maxHist=0;
		double minHist=0;
		minMaxLoc(h_hist, &minHist, &maxHist, 0, 0);*/
		//printf("Max H: %d\n",pos);

        imshow( "Capture ",frame );
        //image.copyTo(foreground,foreground); //cores
        imshow("Foreground ",foreground);
		imshow("Canny ",canny);
		imshow("Edges ",edges);
        //char c = (char)waitKey(fps);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }
}