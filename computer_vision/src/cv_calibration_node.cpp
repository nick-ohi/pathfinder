//Calibrate HSV and YUV values

#include <ros/ros.h> 						//includes headers common for ros system
#include <vector> 							//hue detection
#include <sstream> 							//hue detection
#include <armadillo>						//linear algebra library
#include <image_transport/image_transport.h> 	//publush and subscribe to ros images
#include <cv_bridge/cv_bridge.h> 				//convert between ros and opencv image formats
#include <sensor_msgs/image_encodings.h> 		//constants for image encoding
#include <opencv2/imgproc/imgproc.hpp> 			//image processing
#include <opencv2/highgui/highgui.hpp> 			//gui handling
#include <opencv2/core/core.hpp>				//core functions and data types
#include <opencv2/features2d/features2d.hpp>		//2d features framework
#include <opencv2/nonfree/features2d.hpp>		//SURF algorithms
#include <opencv2/calib3d/calib3d.hpp>			//camera calibration and 3d reconstruction
#include <opencv2/legacy/legacy.hpp>			//brute force matching
#include <computer_vision/Beacon.h>			//message for homing beacon

using namespace std;
namespace enc = sensor_msgs::image_encodings;

//Debugging
const bool HZ = true; //Print sampling rate

//Topic Names
static const char IMAGE_TOPIC[] = "logitech_c920/image_raw";

//Global variables for surf
ros::Publisher pub;

//Callback function
void imageCallback(const sensor_msgs::ImageConstPtr& raw_image);

//Functions
void filterHSV(cv::Mat &frame, const int &COLOR);
void filterYUV(cv::Mat &frame, const int &COLOR);
void morphImg(cv::Mat &thresh);

void createTrackbarsYUV();
void createTrackbarsHSV();
void on_trackbarHSV(int, void*){} 
void on_trackbarYUV(int, void*){}

//HSV YUV
int H_MIN = 0;
int S_MIN = 0;
int Vh_MIN = 0;
int H_MAX = 255;
int S_MAX = 255;
int Vh_MAX = 255;

int Y_MIN = 0;
int U_MIN = 0;
int Vy_MIN = 0;
int Y_MAX = 255;
int U_MAX = 255;
int Vy_MAX = 255;

int main(int argc, char **argv)
{
   ros::init(argc, argv, "det_chroma_node");

   ros::NodeHandle nh;
   image_transport::ImageTransport it(nh);
    
   cv::namedWindow("Origional Image", CV_WINDOW_AUTOSIZE);
   cv::destroyWindow("Origional Image");

   image_transport::Subscriber sub = it.subscribe(IMAGE_TOPIC, 1, imageCallback);

   double time_curr = ros::Time::now().toSec();
   double time_prev = time_curr;
   
   ROS_INFO("Running det_chroma_node...");
   ros::spin();
   
   return 0;
}


void imageCallback(const sensor_msgs::ImageConstPtr& raw_image)
{
	double begin = ros::Time::now().toSec(); //start timer
    
	//structures for subscribing and publishing
	cv_bridge::CvImagePtr cv_ptr;

	try
	{
        cv_ptr = cv_bridge::toCvCopy(raw_image, enc::BGR8);
	}
	catch (cv_bridge::Exception& e)
	{
        ROS_ERROR("computer_vision::det_chroma_node.cpp::cv_bridge exception: %s", e.what());
        return;
	}
    

	cv::imshow("Origional Image", cv_ptr->image);
	cv::waitKey(1);

	cv::Mat imgThreshHSV = cv_ptr->image;
	cv::Mat imgThreshYUV = cv_ptr->image;
	filterHSV(imgThreshHSV, 0);
	filterYUV(imgThreshYUV, 0);
    
	double end = ros::Time::now().toSec(); //stop timer
	if(HZ) ROS_INFO("Sample Rate = %f", 1/(end-begin)); //print execution time
}

//Converts given image to binary image of threshold values
void filterHSV(cv::Mat &frame, const int &COLOR)
{
	//Create trackbars
	createTrackbarsHSV(); 

	//Image variables
	cv::Mat imgHSV, imgThresh;
	cv::Mat img = frame;

	//Process source image
	cv::GaussianBlur(frame, img, cv::Size(9,9), 0); //(41,41), (11,11)
	cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);

	cv::inRange(imgHSV, cv::Scalar(H_MIN, S_MIN, Vh_MIN), cv::Scalar(H_MAX, S_MAX, Vh_MAX), imgThresh); 

	morphImg(imgThresh);			

	frame = imgThresh;

	cv::namedWindow("Threshold Image HSV");
	cv::imshow("Threshold Image HSV", imgThresh); cv::waitKey(1);
}

//Converts given image to binary image of threshold values
void filterYUV(cv::Mat &frame, const int &COLOR)
{
	//Create trackbars
	createTrackbarsYUV(); 
			
	//Image variables
	cv::Mat imgYUV, imgThresh;
	cv::Mat img = frame;
    
	//Process source image
	cv::GaussianBlur(frame, img, cv::Size(9,9), 0); //(41,41), (11,11)
	cv::cvtColor(img, imgYUV, cv::COLOR_BGR2YUV);
    
	cv::inRange(imgYUV, cv::Scalar(Y_MIN, U_MIN, Vy_MIN), cv::Scalar(Y_MAX, U_MAX, Vy_MAX), imgThresh); 

	morphImg(imgThresh);											
    
	frame = imgThresh;
    
	cv::namedWindow("Threshold Image YUV");
	cv::imshow("Threshold Image YUV", imgThresh); cv::waitKey(1);
}

//Trackbars for calibration HSV
void createTrackbarsHSV()
{
	cv::namedWindow("Trackbars HSV", 0);

	char TrackbarNameHSV[50];
	sprintf(TrackbarNameHSV, "H_MIN", H_MIN);
	sprintf(TrackbarNameHSV, "H_MAX", H_MAX);
	sprintf(TrackbarNameHSV, "S_MIN", S_MIN);
	sprintf(TrackbarNameHSV, "S_MAX", S_MAX);
	sprintf(TrackbarNameHSV, "Vh_MIN", Vh_MIN);
	sprintf(TrackbarNameHSV, "Vh_MAX", Vh_MAX);
  
	cv::createTrackbar("H_MIN", "Trackbars HSV", &H_MIN, H_MAX, on_trackbarHSV);
	cv::createTrackbar("H_MAX", "Trackbars HSV", &H_MAX, H_MAX, on_trackbarHSV);
	cv::createTrackbar("S_MIN", "Trackbars HSV", &S_MIN, S_MAX, on_trackbarHSV);
	cv::createTrackbar("S_MAX", "Trackbars HSV", &S_MAX, S_MAX, on_trackbarHSV);
	cv::createTrackbar("Vh_MIN", "Trackbars HSV", &Vh_MIN, Vh_MAX, on_trackbarHSV);
	cv::createTrackbar("Vh_MAX", "Trackbars HSV", &Vh_MAX, Vh_MAX, on_trackbarHSV);
}

//Trackbars for calibration HSV
void createTrackbarsYUV()
{
	cv::namedWindow("Trackbars YUV", 0);

	char TrackbarNameYUV[50];
	sprintf(TrackbarNameYUV, "Y_MIN", Y_MIN);
	sprintf(TrackbarNameYUV, "Y_MAX", Y_MAX);
	sprintf(TrackbarNameYUV, "U_MIN", U_MIN);
	sprintf(TrackbarNameYUV, "U_MAX", U_MAX);
	sprintf(TrackbarNameYUV, "Vy_MIN", Vy_MIN);
	sprintf(TrackbarNameYUV, "Vy_MAX", Vy_MAX);

	cv::createTrackbar("Y_MIN", "Trackbars YUV", &Y_MIN, Y_MAX, on_trackbarYUV);
	cv::createTrackbar("Y_MAX", "Trackbars YUV", &Y_MAX, Y_MAX, on_trackbarYUV);
	cv::createTrackbar("U_MIN", "Trackbars YUV", &U_MIN, U_MAX, on_trackbarYUV);
	cv::createTrackbar("U_MAX", "Trackbars YUV", &U_MAX, U_MAX, on_trackbarYUV);
	cv::createTrackbar("Vy_MIN", "Trackbars YUV", &Vy_MIN, Vy_MAX, on_trackbarYUV);
	cv::createTrackbar("Vy_MAX", "Trackbars YUV", &Vy_MAX, Vy_MAX, on_trackbarYUV);
}

//Erodes and dilates image for tracking
void morphImg(cv::Mat &thresh)
{
	cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
	cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);

	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);
}