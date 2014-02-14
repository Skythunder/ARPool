#include <opencv2/core/core.hpp>
#include <opencv\cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main3( int argc, char** argv )
{
    /*if( argc != 2)
    {
     cout <<" Usage: display_image ImageToLoadAndDisplay" << endl;
     return -1;
    }*/

    Mat image;
	char* imageName = "01_test.tif";
    image = imread(imageName, CV_LOAD_IMAGE_COLOR); // Read the file

    if(! image.data ) // Check for invalid input
    {
        cout << "Could not open or find the image" << std::endl ;
        return -1;
    }
	imshow(imageName,image);
    Mat gray_image;
	cvtColor( image, gray_image, CV_BGR2GRAY );

	imwrite( "Gray_Image.jpg", gray_image );

	namedWindow( imageName, CV_WINDOW_AUTOSIZE );
	namedWindow( "Gray image", CV_WINDOW_AUTOSIZE );

	imshow( imageName, image );
	imshow( "Gray image", gray_image );
	
	waitKey(0);

	return 0;
}
