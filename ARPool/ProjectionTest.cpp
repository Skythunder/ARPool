/*Small test program to validate calibration. Find object by color (hsv calibration by user) 
*and print blue circle around it.
*/
#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include <winsock2.h>

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
#define IMAGE_SIZE Size(240,180)

//server info
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 7777

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

int mainPT()//set to main
{
	bool send_centroids=false;
	//load calibration matrix
	const string inputSettingsFile = "projector_calib.yml";
	const string inputSettingsMaskFile = "projector_calib_mask.yml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // Read the settings
	if (!fs.isOpened())
    {
        cout << "Could not open the calibration data file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }
	FileStorage fsm(inputSettingsMaskFile, FileStorage::READ); // Read the settings
	if (!fsm.isOpened())
    {
        cout << "Could not open the calibration data file: \"" << inputSettingsMaskFile << "\"" << endl;
        return -1;
    }
	Mat rotationMatrix,maskMatrix;
	fs["projector_correction"]>>rotationMatrix;
	fs.release();
	fsm["projector_correction"]>>maskMatrix;
	fsm.release();
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

	//create socket
	WORD wVersionRequested;
    WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);

	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{ 
		cout<<"Cannot create socket"<<endl;
		const DWORD last_error = WSAGetLastError();
		cout<<last_error<<endl;
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

	//set vars, trackbars, windows
	bool loop=true;
	cvNamedWindow("View", CV_WINDOW_NORMAL);
	//cvNamedWindow("Final", CV_WINDOW_NORMAL);
	//cvSetWindowProperty("Final", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
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
		//warpPerspective(fin,fin,rotationMatrix,fin.size());//show circles
		//warpPerspective(thresh,fin,rotationMatrix,fin.size());//show hsv threshold overlay
		//thresh.copyTo(fin);//show decalibrated threshold overlay
		if(send_centroids && mc.size()>0)
		{
			perspectiveTransform(mc,mc,rotationMatrix);
			vector<float>mcf;
			for(int i=0;i<mc.size();i++)
			{
				mcf.push_back(mc[i].x);
				mcf.push_back(mc[i].y);
			}
		
			//Send centroids over socket
			const float *p_floats = &(mcf[0]);
			const char *p_bytes = reinterpret_cast<const char *>(p_floats);
			vector<const char>tosend(p_bytes, p_bytes + sizeof(float) * mcf.size());
			int ccc=sendto(fd, p_bytes, sizeof(float) * mcf.size(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			//cout<<ccc<<endl;
		}
		if(!send_centroids)
		{
			Mat msend = thresh.clone();
			int imgW = msend.cols;
			float rel = 1;
			if(imgW>240){
				rel = float(imgW)/float(240);
			}
			//warpPerspective(msend,msend,maskMatrix,msend.size());
			Mat msend2(IMAGE_SIZE,CV_8U);
			//cv::resize(msend, msend2, cv::Size(msend.cols/rel,msend.rows/rel),0,0,1);
			
			//cout<<"Cols: "<<msend.cols<<endl<<"Rows: "<<msend.rows<<endl;
			//resize(msend,msend,PROJECTOR_RESOLUTION);
			for(int i=0;i<50;i++)
				dilate(msend,msend,Mat());
			warpPerspective(msend,msend2,maskMatrix,msend2.size());
			imshow("Resized",msend2);
			msend2=msend2.reshape(0,1);
			int imgsize = msend2.total()*msend2.elemSize();
			const char *p_bytes = reinterpret_cast<const char *>(msend2.data);
			int ccc=sendto(fd, p_bytes, imgsize, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			//cout<<ccc<<endl;
		}
		
		//imshow("Final",fin);
		imshow("Result",thresh);
		imshow("View",frame);
		char c=waitKey(50);
		/*if(c=='f'||c=='F')
			cvSetWindowProperty("Final", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);*/
		if(c=='c'||c=='C')
			send_centroids=!send_centroids;
		if(c==27)
		{
			loop=false;
			break;
		}
	}

	WSACleanup();
	return 0;
}