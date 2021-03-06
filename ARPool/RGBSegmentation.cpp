/*Normal (RGB) camera Ball segmentation. Will require infrared camera or filter 
*to work with projected effects. Blob centroid calculation pending.
*/
#pragma comment(lib, "Ws2_32.lib")
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <winsock2.h>

using namespace std;
using namespace cv;

//choose video
//#define VIDEOPATH "vid1.mp4"
//#define VIDEOPATH "vid2.mp4"
//#define VIDEOPATH "pool.avi"
#define VIDEOPATH 0
//#define VIDEOPATH "The Rocket Quickfire.mp4"

//set repeat
#define REPEATVIDEO 1

//server info
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 7777

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
#define MIN_BLOB_AREA 20
#define MIN_PERCENT_MASK 0.05
#define MASK_START 1500;

#define DILATION_AMNT 20;
#define IMAGE_SIZE Size(240,180)

#define PROJECTOR_RESOLUTION Size(1400,1050)


int mainRGBS(){//set to main
	bool send_centroids=false;
	//initialize camera
	VideoCapture videocap;
	videocap.open(VIDEOPATH);

	if( !videocap.isOpened() )
    {
        puts("***Could not initialize capturing...***\n");
        return 0;
    }
	
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

	Mat img_src;
	Mat img_frame;

	videocap >> img_frame;
	for(int i;i<20;i++)
	{
		if(!img_frame.empty())
			break;
		videocap >> img_frame;
	}
	if(img_frame.empty()){
		cout <<"ERROR READING FROM CAMERA!"<<endl;
		return 0;
	}
	//resize image to improve speed
	int imgW = img_frame.cols;
	float rel = 1;
	if(imgW>OPT_WINDOW_WIDTH){
		rel = float(imgW)/float(OPT_WINDOW_WIDTH);
	}
	cv::resize(img_frame, img_src, cv::Size(img_frame.cols/rel,img_frame.rows/rel),0,0,1);
	//cv::resize(img_frame, img_src, IMAGE_SIZE,0,0,1);
	Mat img_gray;
	Mat img_gray_bg = Mat::zeros(img_src.size(),CV_32FC1);
	Mat img_gray_fg = Mat(img_src.size(),CV_8UC1);
	//blur to reduce noise
	Mat blur;
	GaussianBlur(img_src,blur,Size(3, 3), 2, 2 );
	//get gray image
	cvtColor(blur,img_gray,CV_BGR2GRAY);
	//imshow("Gray",img_gray);
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
	//imshow("Canny",img_canny);
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

	//create socket
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{ 
		cout<<"Cannot create socket"<<endl;
		return -1;
	}
	struct sockaddr_in myaddr;
	memset((char *)&myaddr, 0, sizeof(myaddr)); 
	myaddr.sin_family = AF_INET; 
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	myaddr.sin_port = htons(0);
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
	{ 
		perror("bind failed"); 
		return 0; 
	}
	struct sockaddr_in servaddr; 
	memset((char*)&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(SERVER_PORT);
	struct hostent *hp; 
	char *host = SERVER_ADDR;
	hp = gethostbyname(host); 
	if (!hp) 
	{ 
		fprintf(stderr, "could not obtain address of %s\n", host); 
		return -1; 
	} 
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

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
		//imshow("Gray",img_gray);
		//get gray bg/fg
		img_gray_bg.convertTo(img_1Cdummy,CV_8UC1);
		absdiff(img_1Cdummy,img_gray,img_gray_fg);
		threshold(img_gray_fg,img_gray_fg,GRAY_THRESH_DIFF,255,THRESH_BINARY);
		//imshow("GRAY FG",img_gray_fg);
		if (mcnt<mstart)
		{
			accumulateWeighted(img_gray,img_gray_bg,GRAY_ALPHA_UPDATE);///////<========Learn bg without mask
			mcnt++;
		}
		else accumulateWeighted(img_gray,img_gray_bg,GRAY_ALPHA_UPDATE,weight_mask);///////<========Mask bg subtraction
		img_gray_bg.convertTo(img_1Cdummy,CV_8UC1);
		//imshow("GRAY BG",img_1Cdummy);


		//get canny, canny bg/fg
		Canny(img_gray,img_canny,canny_l, canny_h);
		//imshow("Canny",img_canny);

		img_canny_bg.convertTo(img_1Cdummy,CV_8UC1);
		absdiff(img_1Cdummy,img_canny,img_canny_fg);
		threshold(img_canny_fg,img_canny_fg,CANNY_THRESH_DIFF,255,THRESH_BINARY);
		///reduce noise
		erode( img_canny_fg, img_canny_fg, MORPH_CROSS );
		dilate( img_canny_fg, img_canny_fg, MORPH_CROSS );
		///
		//imshow("Canny FG",img_canny_fg);

		if (mcnt<mstart)
		{
			accumulateWeighted(img_canny,img_canny_bg,CANNY_ALPHA_UPDATE);///////<=======Learn bg without mask
		}
		else accumulateWeighted(img_canny,img_canny_bg,CANNY_ALPHA_UPDATE,weight_mask);///////<=======Mask bg subtraction
		
		img_canny_bg.convertTo(img_1Cdummy,CV_8UC1);
		//imshow("Canny BG",img_1Cdummy);

		//get color bg/fg, difference and mix
		/*absdiff(img_gray,img_gray_buffer,img_gray_diff);
		threshold(img_gray_diff,img_gray_diff,GRAY_DIFF_THRESH_DIFF,255,THRESH_BINARY);
		imshow("Gray Diff",img_gray_diff);*/

		img_gray.copyTo(img_gray_buffer);

		img_3Cdummy=25;
		img_src.copyTo(img_3Cdummy,img_gray_fg);
		imshow("Mask",img_3Cdummy);
		//find and filter blobs, prepare final mask
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		int minsize = MIN_BLOB_SIZE;
		int minarea = MIN_BLOB_AREA;
		double percent=MIN_PERCENT_MASK;
		vector<Moments> mu;
		Mat cleanMask = Mat::zeros(img_gray_fg.size(),CV_8UC1);
		findContours(img_gray_fg,contours,hierarchy,CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
		for( int i = 0; i< contours.size(); i++ )
		{
			Scalar color = Scalar( 255,255,255 );
			Mat aux = Mat::zeros(img_gray_fg.size(),CV_8UC1);
			if(contours[i].size()>minsize)
			{
				if(contourArea(contours[i],false)>minarea)
				{
					drawContours( aux, contours, i, color, 3, 8, hierarchy, 0, Point());
					int nzt = countNonZero(aux);
					bitwise_and(img_canny_fg,aux,aux);
					int nz=countNonZero(aux);
					double per = nz/double(nzt);
					if(per>percent)
					{
						drawContours( cleanMask, contours, i, color, -1, 8, hierarchy, 0, Point());
						mu.push_back(moments( contours[i], false ));
					}
				}
			}
		}
		///  Get the mass centers:
		vector<float> mc( mu.size()*2 );
		int j=0;
		for( int i = 0; i < mu.size()*2; i+=2 )
		{ 
			mc[i] = mu[j].m10/mu[j].m00; 
			mc[i+1] = mu[j].m01/mu[j].m00; 
			j++;
		}
		
		if(send_centroids)
		{
			//Send centroids over socket
			const float *p_floats = &(mc[0]);
			const char *p_bytes = reinterpret_cast<const char *>(p_floats);
			vector<const char>tosend(p_bytes, p_bytes + sizeof(float) * mc.size());
			//sendto(fd, reinterpret_cast<const char*>(tosend.data()), sizeof(tosend.data()), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			sendto(fd, p_bytes, sizeof(float) * mc.size(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		}

		//draw
		Mat cntr=Mat::zeros(img_gray_fg.size(),CV_8UC1);
		for( int i = 0; i < mc.size(); i+=2 )
		{ 
			circle(cntr,Point(mc[i],mc[i+1]),10,Scalar(255,0,0),-1);
		}
		imshow("Centroids",cntr);
		if(mcnt>=mstart) 
		{
			weight_mask=cleanMask.clone();
			bitwise_not(weight_mask,weight_mask);
			weight_mask.convertTo(weight_mask,CV_8U);
			//img_src.copyTo(cleanMask,cleanMask);//create color mask
			imshow("Final",cleanMask);
		}
		else 
		{
			imshow("Final",img_3Cdummy);
			cout<<mcnt<<endl;
		}

		//sendmask over socket
		if(!send_centroids)
		{
			if(mcnt>=mstart)
			{
				Mat msend = cleanMask.clone();
				int dilamnt=DILATION_AMNT;
				Mat element = getStructuringElement( MORPH_ELLIPSE,Size( 2*dilamnt + 1, 2*dilamnt+1 ),Point( dilamnt, dilamnt ) );
				dilate(msend,msend,element);
				//resize(msend,msend,PROJECTOR_RESOLUTION);
				//warpPerspective(msend,msend,rotationMatrix,msend.size());
				msend=msend.reshape(0,1);
				int imgsize = msend.total()*msend.elemSize();
				const char *p_bytes = reinterpret_cast<const char *>(msend.data);
				sendto(fd, p_bytes, imgsize, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			}
		}
		
		char key = waitKey(1);
		if(key=='c'||key=='C')
			send_centroids=!send_centroids;
		if(key == 27)
			break;
		
	}

	return 0;
}