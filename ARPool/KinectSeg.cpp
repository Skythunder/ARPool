#pragma comment(lib, "Ws2_32.lib")
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <OpenNI.h>

#include <iostream>
#include <winsock2.h>

using namespace cv;
using namespace std;

//server info
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 7777

#define PROJECTOR_RESOLUTION Size(1400,1050)
#define IMAGE_SIZE Size(240,180)

int fps=30;
int resX=640;
int resY=480;
openni::Device device;
openni::VideoStream video_depth, video_color;
openni::VideoStream** video_stream_depth, **video_stream_color;

void on_trackbar(int, void*){}
int threshNear = 112;
int threshFar = 114;
int dilateAmt = 6;
int erodeAmt = 6;
int blurAmt = 1;
int blurPre = 1;

int main(){
	namedWindow("DEPTH",WINDOW_NORMAL);
	createTrackbar( "threshold near", "DEPTH", &threshNear,4000, on_trackbar );
    createTrackbar( "threshold far", "DEPTH", &threshFar,4000, on_trackbar );
	createTrackbar( "amount dilate", "DEPTH", &dilateAmt,40, on_trackbar );
    createTrackbar( "amount erode", "DEPTH", &erodeAmt,40, on_trackbar );
    createTrackbar( "amount blur", "DEPTH", &blurAmt,16, on_trackbar );
    createTrackbar( "blur pre", "DEPTH", &blurPre,1, on_trackbar );
	bool centroid_mode=false;
	cout<<"Centroid Mode OFF!"<<endl;

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

	//create socket
	WORD wVersionRequested;
    WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);

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

	// Inicialização _____
	if (!openni::OpenNI::initialize())
	{
		device.open(openni::ANY_DEVICE);
		if (device.isValid())
		{
			video_stream_depth = new openni::VideoStream*();
			video_stream_depth[0] = &video_depth;
			video_stream_color = new openni::VideoStream*();
			video_stream_color[0] = &video_color;

			device.setDepthColorSyncEnabled(true);
		}
	}
	
	// START WEBCAM _
	openni::VideoMode videoMode;
	// DEPTH
	if (!video_depth.isValid())
	{
		video_depth.create(device, openni::SENSOR_DEPTH);
		if (video_depth.isValid())
		{
			videoMode = video_depth.getVideoMode();
			videoMode.setFps(fps);
			videoMode.setResolution(resX, resY);
			video_depth.setVideoMode(videoMode);
        
			video_depth.start();
		}
	}
	// COLOR:
	if (!video_color.isValid())
	{
		video_color.create(device, openni::SENSOR_COLOR);
		 if (video_color.isValid())
		{
			videoMode = video_color.getVideoMode();
			videoMode.setFps(fps);
			videoMode.setResolution(resX, resY);
			video_color.setVideoMode(videoMode);
         
			video_color.start();
		}
	}
	// _______________________
	cout <<device.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR) << endl;
	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR );

	// CAPTURE WEBCAM _
	openni::VideoFrameRef videoDepthFrame, videoColorFrame;
	bool active = true;
	Mat image,image2;
	while (active)
	{
		// VIDEO STREAM _______________________________________________________________________________
		if (video_depth.isValid())
			//if (!openni::OpenNI::waitForAnyStream(video_stream_depth, 1, &index))
			if (!video_depth.readFrame(&videoDepthFrame))
			{
				float depth_histogram[10000];
				//cout<<"OK!"<<endl;

				Mat aux(videoDepthFrame.getHeight(), videoDepthFrame.getWidth(), CV_16U,(unsigned short*)videoDepthFrame.getData());
				//CONVERT AND FILTER DEPTH IMAGE
				/*const float scaleFactor = 0.05f;
				Mat show,dnear,dfar;
				aux.convertTo( show, CV_8UC1, scaleFactor );
				show.copyTo(dnear);
				show.copyTo(dfar);
				threshold(dnear,dnear,threshNear,255,CV_THRESH_TOZERO);
                threshold(dfar,dfar,threshFar,255,CV_THRESH_TOZERO_INV);
				show = dnear & dfar;*/
				Mat show;
				show=aux.clone();
				int channels = show.channels();

				int nRows = show.rows;
				int nCols = show.cols * channels;

				if (show.isContinuous())
				{
					nCols *= nRows;
					nRows = 1;
				}
				for(int row = 0; row < nRows; ++row) 
				{
					unsigned short* p = show.ptr<unsigned short>(row);
					for(int col = 0; col < nCols; ++col) 
					{
						if(p[col]>threshNear&&p[col]<threshFar)
							p[col]=255;
						else
							p[col]=0;
					}
				}
				show.convertTo( show, CV_8UC1);
				//PROCESS CONTOURS
				if(blurPre == 1) blur(show,show,Size(blurAmt+1,blurAmt+1));
                Mat cntr; show.copyTo(cntr);
                erode(cntr,cntr,Mat(),Point(-1,-1),erodeAmt);
                if(blurPre == 0) blur(cntr,cntr,Size(blurAmt+1,blurAmt+1));
                dilate(cntr,cntr,Mat(),Point(-1,-1),dilateAmt);
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				findContours(cntr,contours,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,Point(0,0));
				//FILTER CONTOURS
				int numContours = contours.size();
				vector<Moments> mu;
                /*vector<vector<Point> > contours_poly( numContours );
                vector<Rect> boundRect( numContours );
                vector<Point2f> centers( numContours );
                vector<float> radii(numContours);*/
                for(int i = 0; i < numContours; i++ ){
					drawContours(cntr,contours,i,Scalar(192,0,0),-1,8,hierarchy,0,Point());
					if(centroid_mode)
						mu.push_back(moments( contours[i], false ));
                    /*approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
                    boundRect[i] = boundingRect( Mat(contours_poly[i]) );
                    minEnclosingCircle(contours_poly[i],centers[i],radii[i]);
                    rectangle( cntr, boundRect[i].tl(), boundRect[i].br(), Scalar(64), 2, 8, 0 );
                    circle(cntr,centers[i],radii[i],Scalar(192));*/
                 }
				///  Get the mass centers:
				vector<Point2f> mc( mu.size() );
				if(centroid_mode)
				{
					for( int i = 0; i < contours.size(); i++ )
					{ 
						mc[i] = Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 ); 
					}
					Mat centroids=Mat::zeros(image.size(),CV_8UC1);
					for( int i = 0; i < mc.size(); i+=2 )
					{ 
						circle(centroids,Point(mc[i]),10,Scalar(255,0,0),-1);
					}
					image=centroids;
					//send centroids over socket
					if(mc.size()>0)
					{
						vector<float>mcf;
						perspectiveTransform(mc,mc,rotationMatrix);
						for(int i=0;i<mc.size();i++)
						{
							mcf.push_back(mc[i].x);
							mcf.push_back(mc[i].y);
						}
						const float *p_floats = &(mcf[0]);
						const char *p_bytes = reinterpret_cast<const char *>(p_floats);
						vector<const char>tosend(p_bytes, p_bytes + sizeof(float) * mcf.size());
						int ccc=sendto(fd, p_bytes, sizeof(float) * mcf.size(), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
						//cout<<ccc<<endl;
					}
				}
				else
				{
					image=cntr;

				}
			}

		if (video_color.isValid())
		{
			if (!video_color.readFrame(&videoColorFrame))
			{
				const openni::RGB888Pixel* imageBuffer = (const openni::RGB888Pixel*)videoColorFrame.getData();
				image2.create(videoColorFrame.getHeight(), videoColorFrame.getWidth(), CV_8UC3);
				memcpy( image2.data, imageBuffer, 3*videoColorFrame.getHeight()*videoColorFrame.getWidth()*sizeof(uint8_t) );

				cvtColor(image2,image2,CV_RGB2BGR);
				//emit webcam_display(WEBCAM::COLOR, image);
			}
		}
		//image2.copyTo(image,image);
		threshold(image,image,0,255,CV_THRESH_BINARY);
		imshow("DEPTH",image);
		imshow("COLOR",image2);
		if(!centroid_mode)
		{
			Mat msend(IMAGE_SIZE,CV_8UC1);
			warpPerspective(image,msend,maskMatrix,msend.size());
			flip(msend,msend,0);
			/*Mat aux(Size(240,240),CV_8UC1);
			resize(image,aux,aux.size());
			Mat msend=aux.clone();
			imshow("Resize",aux);*/
			msend=msend.reshape(0,1);
			int imgsize = msend.total()*msend.elemSize();
			const char *p_bytes = reinterpret_cast<const char *>(msend.data);
			int ccc=sendto(fd, p_bytes, imgsize, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
			//cout<<ccc<<endl;
		}

		char c = waitKey(30);
		if(c=='c'||c=='C')
		{
			centroid_mode=!centroid_mode;
			if(centroid_mode)
				cout<<"Centroid Mode ON!"<<endl;
			else
				cout<<"Centroid Mode OFF!"<<endl;
		}
		if(c==27) 
		{
			active=false;
			break;
		}
	}
	// _______________________
	// STOP WEBCAM _
	if (video_depth.isValid())
	{
		video_depth.stop();
		video_depth.destroy();
	}
	if (video_color.isValid())
	 {
		video_color.stop();
		video_color.destroy();
	}
	// _______________________
	delete video_stream_depth;
	delete video_stream_color;
	device.close();
	//nite::NiTE::shutdown();
	openni::OpenNI::shutdown();

	WSACleanup();

	return 0;
}