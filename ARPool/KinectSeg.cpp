#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <OpenNI.h>

#include <iostream>

using namespace cv;
using namespace std;

int fps=30;
int resX=640;
int resY=480;
openni::Device device;
openni::VideoStream video_depth, video_color;
openni::VideoStream** video_stream_depth, **video_stream_color;
    
void calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame)
 {
    const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
    // Calculate the accumulative histogram (the yellow display...)
    memset(pHistogram, 0, histogramSize*sizeof(float));
     int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
    int height = frame.getHeight(); int width = frame.getWidth();
    unsigned int nNumberOfPoints = 0;
    for (int y = 0; y < height; ++y)
     {
        for (int x = 0; x < width; ++x, ++pDepth)
            if (*pDepth != 0) { pHistogram[*pDepth]++; nNumberOfPoints++; }
        pDepth += restOfRow;
    }
    for (int nIndex=1; nIndex<histogramSize; nIndex++)
         pHistogram[nIndex] += pHistogram[nIndex-1];
    if (nNumberOfPoints)
        for (int nIndex=1; nIndex<histogramSize; nIndex++)
            pHistogram[nIndex] = (256 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
 }

void on_trackbar(int, void*){}
int threshNear = 60;
int threshFar = 100;

int main(){
	namedWindow("DEPTH",1);
	createTrackbar( "threshold near", "DEPTH", &threshNear,255, on_trackbar );
    createTrackbar( "threshold far", "DEPTH", &threshFar,255, on_trackbar );
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

	device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR );

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


	// CAPTURE WEBCAM _
	openni::VideoFrameRef videoDepthFrame, videoColorFrame;
	bool active = true;
	int index;
	Mat image,image2;
	while (1)
	{
		// VIDEO STREAM _______________________________________________________________________________
		if (video_depth.isValid())
			//if (!openni::OpenNI::waitForAnyStream(video_stream_depth, 1, &index))
			if (!video_depth.readFrame(&videoDepthFrame))
			{
				float depth_histogram[10000];
				//cout<<"OK!"<<endl;
				calculateHistogram(depth_histogram, 10000, videoDepthFrame);

				Mat aux(videoDepthFrame.getHeight(), videoDepthFrame.getWidth(), CV_16U,(unsigned short*)videoDepthFrame.getData());

				const float scaleFactor = 0.05f;
				Mat show,dnear,dfar;
				aux.convertTo( show, CV_8UC1, scaleFactor );
				show.copyTo(dnear);
				show.copyTo(dfar);
				threshold(dnear,dnear,threshNear,255,CV_THRESH_TOZERO);
                threshold(dfar,dfar,threshFar,255,CV_THRESH_TOZERO_INV);
				show = dnear & dfar;
				imshow("DEPTH",show);
				image=aux;
				//Mat image(videoDepthFrame.getWidth(), videoDepthFrame.getHeight(), CV_16U);
				const openni::DepthPixel *ptr = (const openni::DepthPixel *) videoDepthFrame.getData();

				/*for (int i = 0; i < image.rows; i++)
				{
					vector<uchar> *scanline = image.at<uchar>(i);
					for (int j = 0; j < image.cols; j++)
						{
						if (*ptr)
						{
							scanline[j] = qRgb((int)depth_histogram[*ptr], (int)depth_histogram[*ptr], 0);
						} else scanline[j] = qRgb(0, 0, 0);
							ptr++;
					}
				}*/
				//emit webcam_display(WEBCAM::DEPTH, image);
			}

		if (video_color.isValid())
		{
			if (!video_color.readFrame(&videoColorFrame))
			{
				/*QImage image((const unsigned char *)videoColorFrame.getData(),
								videoColorFrame.getWidth(),
								videoColorFrame.getHeight(),
								QImage::Format_RGB888);*/
				//Mat aux2(videoColorFrame.getHeight(), videoColorFrame.getWidth(), CV_16UC3,(uchar*)videoColorFrame.getData());
				const openni::RGB888Pixel* imageBuffer = (const openni::RGB888Pixel*)videoColorFrame.getData();
				image2.create(videoColorFrame.getHeight(), videoColorFrame.getWidth(), CV_8UC3);
				memcpy( image2.data, imageBuffer, 3*videoColorFrame.getHeight()*videoColorFrame.getWidth()*sizeof(uint8_t) );

				cvtColor(image2,image2,CV_RGB2BGR);
				//emit webcam_display(WEBCAM::COLOR, image);
			}
		}

 
		Mat img_thresh;
		image.convertTo(image,CV_8UC3);
		threshold(image, img_thresh, 0, 255, THRESH_BINARY_INV);
		medianBlur(img_thresh, img_thresh, 5);


		//imshow("DEPTH",img_thresh);
		imshow("COLOR",image2);
		if(waitKey(30)==27) break;
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
	cout<<"CLOSING DEVICE!"<<endl;
	//device.close();
	cout<<"SHUTING DOWN OPENNI!"<<endl;
	//nite::NiTE::shutdown();
	//openni::OpenNI::shutdown();
	cout<<"OK!"<<endl;
}