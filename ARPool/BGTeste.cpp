#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include<time.h>

using namespace std;
using namespace cv;

int main1()
{
	VideoCapture cap;
    bool update_bg_model = true;
	cap.open("vid1.mp4");

	if( !cap.isOpened() )
    {
        printf("can not open camera or video file\n");
        return -1;
    }

	namedWindow("image", WINDOW_NORMAL);
    namedWindow("foreground mask", WINDOW_NORMAL);
    namedWindow("foreground image", WINDOW_NORMAL);
    namedWindow("mean background image", WINDOW_NORMAL);

	BackgroundSubtractorMOG2 bg_model;//(100, 3, 0.3, 5);

    Mat img, fgmask, fgimg;

	time_t start,end;
	time(&start);
	int counter=0;


    for(;;)
    {
        cap >> img;

        if( img.empty() )
            break;

		//ADICOES:: BLUR
		GaussianBlur( fgimg, fgimg, Size(9, 9), 2, 2 );
		//

        //cvtColor(_img, img, COLOR_BGR2GRAY);

        if( fgimg.empty() )
          fgimg.create(img.size(), img.type());

        //update the model
        bg_model(img, fgmask, update_bg_model ? -1 : 0);

        fgimg = Scalar::all(0);
        img.copyTo(fgimg, fgmask);

        Mat bgimg;
        bg_model.getBackgroundImage(bgimg);

		//ADDICOES:: Reducao de ruido
		erode(fgimg,fgimg,Mat());
		dilate(fgimg,fgimg,Mat());
		erode(fgmask,fgmask,Mat());
		dilate(fgmask,fgmask,Mat());
		//double fps = cap.get(CV_CAP_PROP_FPS);
		//printf("FPS: %d\n",fps);
		//vector<Vec3f> circles;
		/*HoughCircles(fgimg, circles, CV_HOUGH_GRADIENT, 1, 10,
                 100, 30, 1, 30 // change the last two parameters
                                // (min_radius & max_radius) to detect larger circles
                 );
		for( size_t i = 0; i < circles.size(); i++ )
		{
			Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			// circle center
			circle( fgimg, center, 3, Scalar(0,255,0), -1, 8, 0 );
			// circle outline
			circle( fgimg, center, radius, Scalar(0,0,255), 3, 8, 0 );
		}*/
		//

        imshow("image", img);
        imshow("foreground mask", fgmask);
        imshow("foreground image", fgimg);
        if(!bgimg.empty())
          imshow("mean background image", bgimg );
		time(&end);
		++counter;
		double sec=difftime(end,start);
		double fps=counter/sec;
		printf("\n%lf",fps);


        char k = 1;/*(char)waitKey(1);
        if( k == 27 ) break;
        if( k == ' ' )
        {
            update_bg_model = !update_bg_model;
            if(update_bg_model)
                printf("Background update is on\n");
            else
                printf("Background update is off\n");
        }*/
    }

    return 0;
}

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/features2d/features2d.hpp"
#include <stdio.h>
using namespace cv;

/// Global Variables
const int alpha_slider_max = 255;
int alpha_slider;
const int beta_slider_max = 255;
int beta_slider;
const int msize_slider_max = 255;
int msize_slider;
const int nsize_slider_max = 255;
int nsize_slider;
int alpha;
int beta;
int msize;
int nsize;

void on_alpha_trackbar( int, void* )
{
 alpha = alpha_slider;
}

void on_beta_trackbar( int, void* )
{
 beta = beta_slider;
}

void on_msize_trackbar( int, void* )
{
 msize = msize_slider;
}

void on_nsize_trackbar( int, void* )
{
 nsize = nsize_slider;
}


int mainA()//Interessa!!
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
    Mat frame,foreground,image,edges,canny;
	alpha=alpha_slider=166;
	beta=beta_slider=171;
	msize=msize_slider=60;
	nsize=nsize_slider=10;
    BackgroundSubtractorMOG2 mog;
    int fps=cap.get(CV_CAP_PROP_FPS);
    if(fps<=0)
        fps=10;
    else
        fps=1000/fps;
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
        //mog(image,foreground,-1);

		createTrackbar( "A", "Capture ", &alpha_slider, alpha_slider_max, on_alpha_trackbar );
		createTrackbar( "B", "Capture ", &beta_slider, beta_slider_max, on_beta_trackbar );
		createTrackbar( "M", "Capture ", &msize_slider, msize_slider_max, on_msize_trackbar );
		createTrackbar( "N", "Capture ", &nsize_slider, nsize_slider_max, on_nsize_trackbar );
		Canny(image,edges,alpha,beta);
		canny=edges.clone();
		Rect rect = Rect(40, 40, 435, 205);
		edges = edges(rect);
		canny = canny(rect);

		dilate(edges,edges,Mat());
		dilate(edges,edges,Mat());
		erode(edges,edges,Mat());
		erode(edges,edges,Mat());

		//foreground = foreground(rect);
		
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(edges,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( edges.size(), CV_8UC3 );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			if(contours[i].size()<msize&&contours[i].size()>nsize)
				drawContours( drawing, contours, i, color, -1, 8, hierarchy, 1, Point());
		}

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

        imshow( "Capture ",frame );
        //image.copyTo(foreground,foreground); //cores
        imshow("Foreground ",drawing);
		imshow("Edges ",canny);
        //char c = (char)waitKey(fps);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }


}

int mainY()
{
    VideoCapture cap;

    //cap.open(-1); 
    //cap.open("pool.avi"); 
	cap.open("vid1.mp4"); 
    if( !cap.isOpened() )
    {

        puts("***Could not initialize capturing...***\n");
        return 0;
    }
    namedWindow( "Capture ", CV_WINDOW_AUTOSIZE);
    //namedWindow( "Foreground ", CV_WINDOW_AUTOSIZE );
	namedWindow( "Edges ", CV_WINDOW_AUTOSIZE );
    Mat frame,foreground,image,edges;
	alpha=alpha_slider=180;
	beta=beta_slider=255;
    BackgroundSubtractorMOG2 mog;
    int fps=cap.get(CV_CAP_PROP_FPS);
    if(fps<=0)
        fps=10;
    else
        fps=1000/fps;
    for(;;)
    {
        cap>>frame;  
		
        if( frame.empty() )
                break;
        image=frame.clone();
		GaussianBlur(image,image,Size(3, 3), 2, 2 );//mais rapido q median mas menos qualidade
        //mog(image,foreground,-1);

		createTrackbar( "A", "Edges ", &alpha_slider, alpha_slider_max, on_alpha_trackbar );
		createTrackbar( "B", "Edges ", &beta_slider, beta_slider_max, on_beta_trackbar );
		Canny(image,edges,alpha,beta);
		Rect rect = Rect(40, 40, 435, 205);

		//foreground = foreground(rect);
		//edges = edges(rect);
		dilate(edges,edges,Mat());
		dilate(edges,edges,Mat());
		erode(edges,edges,Mat());
		erode(edges,edges,Mat());

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

        imshow( "Capture ",frame );
        //image.copyTo(foreground,foreground); //cores
        //imshow("Foreground ",drawing);
		imshow("Edges ",edges);
        //char c = (char)waitKey(fps);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }


}

int mainZ()
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
    Mat frame,foreground,image,edges,canny;
	alpha=alpha_slider=166;
	beta=beta_slider=171;
	msize=msize_slider=190;
	nsize=nsize_slider=50;
    BackgroundSubtractorMOG2 mog;
    int fps=cap.get(CV_CAP_PROP_FPS);
    if(fps<=0)
        fps=10;
    else
        fps=1000/fps;
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
		//GaussianBlur(image,image,Size(3, 3), 2, 2 );//mais rapido q median mas menos qualidade
        //mog(image,foreground,-1);

		createTrackbar( "A", "Capture ", &alpha_slider, alpha_slider_max, on_alpha_trackbar );
		createTrackbar( "B", "Capture ", &beta_slider, beta_slider_max, on_beta_trackbar );
		createTrackbar( "M", "Capture ", &msize_slider, msize_slider_max, on_msize_trackbar );
		createTrackbar( "N", "Capture ", &nsize_slider, nsize_slider_max, on_nsize_trackbar );
		/*Canny(image,edges,alpha,beta);
		canny=edges.clone();
		Rect rect = Rect(40, 40, 435, 205);
		edges = edges(rect);
		canny = canny(rect);

		dilate(edges,edges,Mat());
		dilate(edges,edges,Mat());
		erode(edges,edges,Mat());
		erode(edges,edges,Mat());*/
		Rect rect = Rect(40, 40, 435, 205);
		image = image(rect);
		//foreground = foreground(rect);
		cvtColor(image,image,CV_BGR2GRAY);
		threshold(image,image,nsize,msize,THRESH_BINARY);
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(image,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( image.size(), CV_8UC3 );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			if(contours[i].size()<msize&&contours[i].size()>nsize)
				drawContours( drawing, contours, i, color, -1, 8, hierarchy, 1, Point());
		}

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

        imshow( "Capture ",frame );
        //image.copyTo(foreground,foreground); //cores
        imshow("Foreground ",drawing);
		imshow("Edges ",image);
        //char c = (char)waitKey(fps);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }


}

int nalpha,nbeta,ngama;
const int nalpha_slider_max = 255;
int nalpha_slider;
const int nbeta_slider_max = 255;
int nbeta_slider;
const int ngama_slider_max = 255;
int ngama_slider;
int maxsize,minsize;
int maxsize_slider;
int minsize_slider;
const int maxsize_slider_max=255;
const int minsize_slider_max=255;
void on_nalpha_trackbar( int, void* )
{
 nalpha = nalpha_slider;
}

void on_nbeta_trackbar( int, void* )
{
 nbeta = nbeta_slider;
}

void on_ngama_trackbar( int, void* )
{
 ngama = ngama_slider;
}

void on_maxsize_trackbar( int, void* )
{
 maxsize = maxsize_slider;
}
void on_minsize_trackbar( int, void* )
{
 minsize = minsize_slider;
}
int main()
{
    VideoCapture cap;
	nalpha=nalpha_slider=91;
	nbeta=nbeta_slider=173;
	ngama=ngama_slider=255;
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
    Mat frame,foreground,image,edges,canny;
	IplImage *iplframe;
	/*alpha=alpha_slider=43;
	beta=beta_slider=75;
	msize=msize_slider=89;
	maxsize=maxsize_slider=100;
	minsize=minsize_slider=10;
	//nsize=nsize_slider=10;*/
	alpha=alpha_slider=50;
	beta=beta_slider=25;
	msize=msize_slider=60;
	maxsize=maxsize_slider=100;
	minsize=minsize_slider=20;
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
        //mog(image,foreground,-1);

		createTrackbar( "H1", "Capture ", &alpha_slider, alpha_slider_max, on_alpha_trackbar );
		createTrackbar( "S1", "Capture ", &beta_slider, beta_slider_max, on_beta_trackbar );
		createTrackbar( "V1", "Capture ", &msize_slider, msize_slider_max, on_msize_trackbar );
		createTrackbar( "H2", "Capture ", &nalpha_slider, nalpha_slider_max, on_nalpha_trackbar );
		createTrackbar( "S2", "Capture ", &nbeta_slider, nbeta_slider_max, on_nbeta_trackbar );
		createTrackbar( "V2", "Capture ", &ngama_slider, ngama_slider_max, on_ngama_trackbar );
		createTrackbar( "MaxSize", "Capture ", &maxsize_slider, maxsize_slider_max, on_maxsize_trackbar );
		createTrackbar( "MinSize", "Capture ", &minsize_slider, minsize_slider_max, on_minsize_trackbar );
		/*dilate(edges,edges,Mat());
		dilate(edges,edges,Mat());
		erode(edges,edges,Mat());
		erode(edges,edges,Mat());*/

		Mat hsv;
		cvtColor(frame,hsv,CV_BGR2HSV);
		IplImage iplimg=hsv;
		iplframe = &iplimg;
		IplImage *iplhsv=cvCreateImage(cvGetSize(iplframe),IPL_DEPTH_8U, 1);;
		cvInRangeS(iplframe,cvScalar(alpha,beta,msize), cvScalar(nalpha,nbeta,ngama),iplhsv);
		Mat fin = Mat(iplhsv,false);
		bitwise_not ( fin, fin );
		//image.copyTo(fin,fin);
		//Canny(fin,edges,alpha,beta);
		//canny=edges.clone();
		Rect rect = Rect(42, 42, 435, 205);
		//edges = edges(rect);
		//canny = canny(rect);
		fin=fin(rect);
		//foreground = foreground(rect);
		dilate(fin,fin,Mat());
		dilate(fin,fin,Mat());
		erode(fin,fin,Mat());
		erode(fin,fin,Mat());
		
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours(fin,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		Mat drawing = Mat::zeros( fin.size(), CV_8UC3 );
		int momsize=0;
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			if(contours[i].size()<maxsize&&contours[i].size()>minsize)
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
		}

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

		vector<Mat> hsv_planes;
		split( hsv, hsv_planes );
		int histSize = 256;
		float range[] = { 0, 256 } ;
		const float* histRange = { range };
		bool uniform = true; bool accumulate = false;
		Mat h_hist, s_hist, v_hist;
		calcHist( &hsv_planes[0], 1, 0, Mat(), h_hist, 1, &histSize, &histRange, uniform, accumulate );
		calcHist( &hsv_planes[1], 1, 0, Mat(), s_hist, 1, &histSize, &histRange, uniform, accumulate );
		calcHist( &hsv_planes[2], 1, 0, Mat(), v_hist, 1, &histSize, &histRange, uniform, accumulate );
		int hist_w = 512; int hist_h = 400;
		int bin_w = cvRound( (double) hist_w/histSize );
		Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
		normalize(h_hist, h_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
		normalize(s_hist, s_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
		normalize(v_hist, v_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
		/// Draw for each channel
		double maxHist=0;
		int pos=-1;
		j=0;
		for( int i = 1; i < histSize; i++ )
		{
			line( histImage, Point( bin_w*(i-1), hist_h - cvRound(h_hist.at<float>(i-1)) ) ,
                       Point( bin_w*(i), hist_h - cvRound(h_hist.at<float>(i)) ),
                       Scalar( 255, 0, 0), 2, 8, 0  );
			/*double aux = h_hist.at<float>(j);
			if (maxHist<aux)
				{
					maxHist=aux;
					pos=j;
				}
			j++;*/
			line( histImage, Point( bin_w*(i-1), hist_h - cvRound(s_hist.at<float>(i-1)) ) ,
                       Point( bin_w*(i), hist_h - cvRound(s_hist.at<float>(i)) ),
                       Scalar( 0, 255, 0), 2, 8, 0  );
			line( histImage, Point( bin_w*(i-1), hist_h - cvRound(v_hist.at<float>(i-1)) ) ,
                       Point( bin_w*(i), hist_h - cvRound(v_hist.at<float>(i)) ),
                       Scalar( 0, 0, 255), 2, 8, 0  );
		}
		namedWindow("calcHist Demo", CV_WINDOW_AUTOSIZE );
		imshow("calcHist Demo", histImage );
		/*double maxHist=0;
		double minHist=0;
		minMaxLoc(h_hist, &minHist, &maxHist, 0, 0);*/
		//printf("Max H: %d\n",pos);

        imshow( "Capture ",frame );
        //image.copyTo(foreground,foreground); //cores
        imshow("Foreground ",drawing);
		imshow("Edges ",fin);
        //char c = (char)waitKey(fps);
		char c = (char)waitKey(1); //mais rapido
        if( c == 27 )   
            break;

    }
}