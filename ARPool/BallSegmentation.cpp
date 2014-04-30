/*Normal (RGB) camera Ball segmentation. Will require infrared camera or filter 
*to work with projected effects. Blob centroid calculation pending.
*/
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"
#include <stdio.h>
#include <iostream>
#include <time.h>

using namespace std;
using namespace cv;

//choose video
//#define VIDEOPATH "vid1.mp4"
//#define VIDEOPATH "vid2.mp4"
#define VIDEOPATH "pool.avi"
//#define VIDEOPATH "The Rocket Quickfire.mp4"

//set repeat
#define REPEATVIDEO 1

//parameter definition
#define CANNY_L 100
#define CANNY_H 150
#define CANNY_ALPHA_UPDATE 0.001
#define CANNY_THRESH_DIFF  125

#define GRAY_ALPHA_UPDATE 0.01
#define GRAY_THRESH_DIFF 10

#define GRAY_DIFF_THRESH_DIFF 10
#define NONMOTIONBUFFER  20

#define OPT_WINDOW_WIDTH 320

#define MIN_BLOB_SIZE 10
#define MIN_PERCENT_MASK 0.05
#define MASK_START 1500;

//mix images
int MixImgs(			cv::Mat *p_mat_in1,
						cv::Mat *p_mat_in2,
						cv::Mat *p_mat_dst,
						cv::Scalar * p_color_1,
						cv::Scalar * p_color_2,
						cv::Scalar * p_color_none,
						cv::Scalar * p_both){

	uchar * p_mat1 = (uchar*) p_mat_in1->data;
	uchar * p_mat2 = (uchar*) p_mat_in2->data;
	uchar * p_matdst = (uchar*) p_mat_dst->data;

	int size = p_mat_in1->rows*p_mat_in1->cols;

	for(int i=0;i<size;i++){
		int pix = 3*i;
		if(p_mat1[i]){
			if(!p_mat2[i]){ // Only 1
				p_matdst[pix] = p_color_1->val[0];
				p_matdst[pix+1] = p_color_1->val[1];
				p_matdst[pix+2] = p_color_1->val[2];
			}
			else{ // Both
				p_matdst[pix] = p_both->val[0];
				p_matdst[pix+1] = p_both->val[1];
				p_matdst[pix+2] = p_both->val[2];

			}

		}else{
			if(p_mat2[i]){ // Only 2
				p_matdst[pix] = p_color_2->val[0];
				p_matdst[pix+1] = p_color_2->val[1];
				p_matdst[pix+2] = p_color_2->val[2];
			}
			else{
				p_matdst[pix] = p_color_none->val[0];
				p_matdst[pix+1] = p_color_none->val[1];
				p_matdst[pix+2] = p_color_none->val[2];

			}

		}


	}


	return 0;
}


int mainOFF (){//set to main
	//initialize camera
	VideoCapture videocap;
	videocap.open(VIDEOPATH);

	if( !videocap.isOpened() )
    {
        puts("***Could not initialize capturing...***\n");
        return 0;
    }
	
	Mat img_src;
	Mat img_frame;

	videocap >> img_frame;
	if(img_frame.empty())
			return 0;
	//resize image to improve speed
	int imgW = img_frame.cols;
	float rel = 1;
	if(imgW>OPT_WINDOW_WIDTH){
		rel = float(imgW)/float(OPT_WINDOW_WIDTH);
	}
	cv::resize(img_frame, img_src, cv::Size(img_frame.cols/rel,img_frame.rows/rel),0,0,1);

	Mat img_gray;
	Mat img_gray_bg = Mat::zeros(img_src.size(),CV_32FC1);
	Mat img_gray_fg = Mat(img_src.size(),CV_8UC1);
	//blur to reduce noise
	Mat blur;
	GaussianBlur(img_src,blur,Size(3, 3), 2, 2 );
	//get gray image
	cvtColor(blur,img_gray,CV_BGR2GRAY);
	imshow("Gray",img_gray);
	img_gray.convertTo(img_gray_bg,CV_32FC1);

	namedWindow("Control");
	int canny_l = CANNY_L;
	int canny_h = CANNY_H;
	//canny control
	createTrackbar("Canny L","Control",&canny_l,255);
	createTrackbar("Canny H","Control",&canny_h,255);

	Mat img_canny;
	Mat img_canny_bg = Mat::zeros(img_src.size(),CV_32FC1);
	Mat img_canny_fg = Mat(img_src.size(),CV_8UC1);
	//get initial canny
	Canny(img_gray,img_canny,canny_l, canny_h);
	imshow("Canny",img_canny);
	img_canny.convertTo(img_canny_bg,CV_32FC1);

	Mat img_1Cdummy = Mat(img_src.size(),CV_8UC1);
	Mat img_3Cdummy = Mat(img_src.size(),CV_8UC3);

	Mat img_gray_buffer = Mat(img_src.size(),CV_8UC1);
	Mat img_gray_diff;
	Mat img_diff_hist = Mat(img_src.size(),CV_8UC1);;
	Mat pseudocolor = Mat(img_src.size(),CV_8UC3);
	//prepare initial (white - all pixels) mask
	////////////////////////////////////
	Mat weight_mask = Mat::zeros(img_src.size(),CV_8U);
	bitwise_not(weight_mask,weight_mask);
	weight_mask.convertTo(weight_mask,CV_8U);
	////////////////////////////////////
	int mcnt = 0;
	int mstart = MASK_START;
	//main loop
	while(1){
		//read frame
		videocap >> img_frame;
		//set repeat if necessary
		if(img_frame.empty()){
			if(	REPEATVIDEO ){
				videocap.set(CV_CAP_PROP_POS_FRAMES,0);
				videocap >> img_frame; // get a new frame from camera
				if(img_frame.empty()){
					printf("Error");
					return -1;
				}
			}else{
				return 0;
			}
			
		}
		cv::resize(img_frame, img_src, cv::Size(img_frame.cols/rel,img_frame.rows/rel),0,0,1);
		//show original
		imshow("Src",img_src);

		//blur to reduce noise
		/////////////////////////////////
		GaussianBlur(img_src,blur,Size(3, 3), 2, 2 );
		/////////////////////////////////
		//get gray image
		cvtColor(blur,img_gray,CV_BGR2GRAY);
		imshow("Gray",img_gray);
		//get gray bg/fg
		img_gray_bg.convertTo(img_1Cdummy,CV_8UC1);
		absdiff(img_1Cdummy,img_gray,img_gray_fg);
		threshold(img_gray_fg,img_gray_fg,GRAY_THRESH_DIFF,255,THRESH_BINARY);
		imshow("GRAY FG",img_gray_fg);
		if (mcnt<mstart)
		{
			accumulateWeighted(img_gray,img_gray_bg,GRAY_ALPHA_UPDATE);///////<========Learn bg without mask
			mcnt++;
		}
		else accumulateWeighted(img_gray,img_gray_bg,GRAY_ALPHA_UPDATE,weight_mask);///////<========Mask bg subtraction
		img_gray_bg.convertTo(img_1Cdummy,CV_8UC1);
		imshow("GRAY BG",img_1Cdummy);


		//get canny, canny bg/fg
		Canny(img_gray,img_canny,canny_l, canny_h);
		imshow("Canny",img_canny);

		img_canny_bg.convertTo(img_1Cdummy,CV_8UC1);
		absdiff(img_1Cdummy,img_canny,img_canny_fg);
		threshold(img_canny_fg,img_canny_fg,CANNY_THRESH_DIFF,255,THRESH_BINARY);
		///reduce noise
		erode( img_canny_fg, img_canny_fg, MORPH_CROSS );
		dilate( img_canny_fg, img_canny_fg, MORPH_CROSS );
		///
		imshow("Canny FG",img_canny_fg);

		if (mcnt<mstart)
		{
			accumulateWeighted(img_canny,img_canny_bg,CANNY_ALPHA_UPDATE);///////<=======Learn bg without mask
		}
		else accumulateWeighted(img_canny,img_canny_bg,CANNY_ALPHA_UPDATE,weight_mask);///////<=======Mask bg subtraction
		
		img_canny_bg.convertTo(img_1Cdummy,CV_8UC1);
		imshow("Canny BG",img_1Cdummy);

		//get color bg/fg, difference and mix
		absdiff(img_gray,img_gray_buffer,img_gray_diff);
		threshold(img_gray_diff,img_gray_diff,GRAY_DIFF_THRESH_DIFF,255,THRESH_BINARY);
		imshow("Gray Diff",img_gray_diff);

		img_gray.copyTo(img_gray_buffer);


		img_diff_hist+=1;
		img_diff_hist.setTo(Scalar(0),img_gray_diff);
		threshold(img_diff_hist,img_diff_hist,NONMOTIONBUFFER,NONMOTIONBUFFER,CV_THRESH_TRUNC);
		img_diff_hist.at<uchar>(0)=0;
		img_diff_hist.at<uchar>(1)=NONMOTIONBUFFER;
		normalize(img_diff_hist,img_1Cdummy,255,0,CV_MINMAX,0);
	    applyColorMap(img_1Cdummy,pseudocolor,2);
		cv::imshow("Diff Non Motion",pseudocolor);

		MixImgs(&img_gray_fg,&img_canny_fg,&pseudocolor,&Scalar(255,0,0),&Scalar(0,255,0),&Scalar(0,0,0),& Scalar(0,0,255));
		imshow("Mix",pseudocolor);

		img_3Cdummy=25;
		img_src.copyTo(img_3Cdummy,img_gray_fg);
		imshow("Mask",img_3Cdummy);
		//find and filter blobs, prepare final mask
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		int minsize = MIN_BLOB_SIZE;
		double percent=MIN_PERCENT_MASK;
		Mat cleanMask = Mat::zeros(img_gray_fg.size(),CV_8UC1);
		findContours(img_gray_fg,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(img_gray_fg.size(),CV_8UC1);
			if(contours[i].size()>minsize)
			{
				drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
				int nzt = countNonZero(aux);
				bitwise_and(img_canny_fg,aux,aux);
				int nz=countNonZero(aux);
				double per = nz/double(nzt);
				if(per>percent)
				{
					drawContours( cleanMask, contours, i, color, -1, 8, hierarchy, 0, Point());
				}
			}
		}
		if(mcnt>=mstart) 
		{
			weight_mask=cleanMask.clone();
			bitwise_not(weight_mask,weight_mask);
			weight_mask.convertTo(weight_mask,CV_8U);
			img_src.copyTo(cleanMask,cleanMask);
			imshow("Final",cleanMask);
		}
		else 
		{
			imshow("Final",img_3Cdummy);
			cout<<mcnt<<endl;
		}

		if(waitKey(1) == 27)
			break;

	}

	return 0;
}