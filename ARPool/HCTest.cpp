#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>

#include <iostream>

using namespace cv;
using namespace std;

static void help()
{
    cout << "\nThis program demonstrates circle finding with the Hough transform.\n"
            "Usage:\n"
            "./houghcircles <image_name>, Default is pic1.png\n" << endl;
}

int main1(int argc, char** argv)
{
    const char* filename = argc >= 2 ? argv[1] : "D:/Libraries/opencv247/sources/samples/cpp/board.jpg";

    Mat img = imread(filename, 0);
    if(img.empty())
    {
        help();
        cout << "can not open " << filename << endl;
        return -1;
    }

    Mat cimg;
    medianBlur(img, img, 5);
    cvtColor(img, cimg, COLOR_GRAY2BGR);

    vector<Vec3f> circles;
    HoughCircles(img, circles, CV_HOUGH_GRADIENT, 1, 10,
                 100, 30, 10, 20 // change the last two parameters
                                // (min_radius & max_radius) to detect larger circles
                 );
    for( size_t i = 0; i < circles.size(); i++ )
    {
        Vec3i c = circles[i];
        circle( cimg, Point(c[0], c[1]), c[2], Scalar(0,0,255), 3, CV_AA);
        circle( cimg, Point(c[0], c[1]), 2, Scalar(0,255,0), 3, CV_AA);
    }

    imshow("detected circles", cimg);
    waitKey();

    return 0;
}

int main2(int argc, char** argv)
{
    VideoCapture cap("pool.avi");
    if(!cap.isOpened())
        return -1;
    namedWindow( "circles", 1 );

    Mat img, gray;
    for( ;; )
    {

        cap >> img;
        vector<Vec3f> circles;      
        cvtColor(img, gray, CV_BGR2GRAY);
        GaussianBlur(gray, gray, Size(7,7), 1.5, 1.5);
        HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 10, 100, 30,4,13 );
		for( size_t i = 0; i < circles.size(); i++ )
		{
			Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			// draw the circle center
			circle( img, center, 3, Scalar(0,255,0), -1, 8, 0 );
			// draw the circle outline
			circle( img, center, radius, Scalar(0,0,255), 3, 8, 0 );
		}
        imshow( "circles", img );
		
        if(waitKey(30) >= 0) break;
    }

    return 0;
}
