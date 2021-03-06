#include <ros/ros.h> 								//includes headers common for ros system
#include <vector> 									//hue detection
#include <sstream> 									//hue detection
#include <armadillo>								//linear algebra library
#include <image_transport/image_transport.h> 		//publush and subscribe to ros images
#include <cv_bridge/cv_bridge.h> 					//convert between ros and opencv image formats
#include <sensor_msgs/image_encodings.h> 			//constants for image encoding
#include <opencv2/imgproc/imgproc.hpp> 				//image processing
#include <opencv2/highgui/highgui.hpp> 				//gui handling
#include <opencv2/core/core.hpp>					//core functions and data types
#include <opencv2/features2d/features2d.hpp>		//2d features framework
#include <opencv2/nonfree/features2d.hpp>			//SURF algorithms
#include <opencv2/calib3d/calib3d.hpp>				//camera calibration and 3d reconstruction
#include <opencv2/legacy/legacy.hpp>				//brute force matching
#include <computer_vision/Beacon.h>					//message for homing beacon

using namespace std;
namespace enc = sensor_msgs::image_encodings;

//Show images hue
const bool SHOW_ORIGIONAL_IMAGE = false;	//image callback	
const bool SHOW_THRESH_IMAGE = true;		//filterHue function
const bool DRAW_HUE_TRACKING = true;		//findCandidates function 
const bool DRAW_CIRCLES = true; 			//hough circle function

//Show images features
const bool DRAW_FOUND_MATCHES = true;

//Debugging
const bool CALIBRATE_MODE = false;
const bool HZ = false; //print tracking frequency
const bool DEBUG_HOMOGRAPHY = false;
const bool PRINT_POSE = false;
const bool ORB_VERIFICATION = true;

//Topic Names
static const char IMAGE_TOPIC[] = "logitech_c920/image_raw";
static const char OUTPUT_DATA_TOPIC[] = "det/mis_out_data";

//File locations
static const string home = getenv("HOME");
static const string BEACON_IMG_ADDRESS = home + "/catkin_ws/src/computer_vision/images/beacon4.png";

//Global variables for surf
ros::Publisher pub;
double time_curr, time_prev;
int counter = 0, wait = 0;
int x_, y_;


//Class for homing beacon tracking
class TrackingObject
{
	public:
	TrackingObject(void){};
	~TrackingObject(void){};

	TrackingObject(std::string name)
	{
		setType(name);
		
		if(name ==  "SideOne")
		{

			setHSVmin(cv::Scalar(0, 0, 0));
			setHSVmax(cv::Scalar(180, 256, 256));
			setColor(cv::Scalar(255, 0, 0));
		}

		if(name ==  "SideTwo")
		{
			setHSVmin(cv::Scalar(0, 0, 0));
			setHSVmax(cv::Scalar(180, 256, 256));
			setColor(cv::Scalar(0, 0, 255));
		}
		
		if(name == "Cropped")
		{
			setHSVmin(cv::Scalar(0, 0, 0));
			setHSVmax(cv::Scalar(180, 256, 256));
			setColor(cv::Scalar(0, 0, 255));		
		}
		
		if(name == "Segment")
		{
			setHSVmin(cv::Scalar(0, 0, 0));
			setHSVmax(cv::Scalar(180, 256, 256));
			setColor(cv::Scalar(0, 0, 255));		
		}
	}

	int getxPos() {return xPos;}
	void setxPos(int x) {xPos=x;}

	int getyPos() {return yPos;}
	void setyPos(int y) {yPos=y;};
	
	int getarea() {return areaObj;}
	void setarea(int area) {areaObj=area;};

	cv::Scalar getHSVmin() {return HSVmin;}
	cv::Scalar getHSVmax() {return HSVmax;}

	void setHSVmin(cv::Scalar min) {HSVmin=min;}
	void setHSVmax(cv::Scalar max) {HSVmax=max;}

	std::string getType() {return type;}
	void setType(std::string t) {type = t;}

	cv::Scalar getColor() {return Color;}
	void setColor(cv::Scalar c) {Color = c;}
	
	private:
	int xPos, yPos, area, areaObj;
	std::string type;
	cv::Scalar HSVmin, HSVmax;
	cv::Scalar Color;
};

//Structure containing verification information
struct Verified
{
	int x,y;
	float area;
	unsigned int seen;
}; 

//beacon stuff
int pink = 0;
int yellow = 0;
int x_beacon=0, x_beacon2=0, y_beacon=0;;
int area_beacon=0;
int detected = 0;

//Callback function
void imageCallback(const sensor_msgs::ImageConstPtr& raw_image);

//EXPERIMENTAL
int findNumberHues(cv::Mat &frame, TrackingObject Object, vector <TrackingObject> &Objects);

//Hue tracking functions
void filterHue(cv::Mat &frame, const int &Y); //Converts given image to binary image of threshold values
vector <TrackingObject> findCandidates(const cv::Mat &frame, const cv::Mat imgThresh); //Return objects containing circles
int findHoughCircles(const cv::Mat &frame); //return number of circles in an image
void morphImg(cv::Mat &thresh); //erode and dilate image
string intToString(int number); //sub-function

//Calibration functions
void createTrackbars();
void on_trackbar(int, void*);

//Feature tracking funtions
cv::Mat trackHomingBeacon(const cv::Mat &frame);
void findCenter(const cv::Mat &imgA, const vector<cv::KeyPoint> &keypointsA, const cv::Mat &imgB, const vector<cv::KeyPoint> &keypointsB, const vector<cv::DMatch> good_matches, const cv::Mat H);
void findRotation(cv::Mat &homography);
template <typename T> int sgn(T val);

//Max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;

//Minimum and maximum object hue area
const int MIN_OBJECT_AREA = 1 * 1;
const int MAX_OBJECT_AREA = 400 * 400;

/*
//temp yellow sensitive
int H_MINy = 13;//19;//20;
int S_MINy = 59;//39;//100;
int V_MINy = 81;//73;//100;
int H_MAXy = 48;//25;//30;
int S_MAXy = 198;//125;//255;
int V_MAXy = 255;//192;//255;
*/


//temp ball yellow narrow
int H_MINy = 17;//20;//20;
int S_MINy = 133;//28;//100;
int V_MINy = 149;//139;//100;
int H_MAXy = 39;//53;//30;
int S_MAXy = 240;//255;//255;
int V_MAXy = 204;//255;//255;

/*
//temp ball pink 
int H_MINy = 163;
int S_MINy = 50;
int V_MINy = 135;
int H_MAXy = 192;
int S_MAXy = 171;
int V_MAXy = 236;
*/
/*
//beacon yellow narrow
int H_MINy = 17;//20;//20;
int S_MINy = 58;//28;//100;
int V_MINy = 149;//139;//100;
int H_MAXy = 31;//53;//30;
int S_MAXy = 82;//255;//255;
int V_MAXy = 196;//255;//255;
*/

//temp blue
int H_MINb = 82;//;
int S_MINb = 17;//48;
int V_MINb = 81;//79;
int H_MAXb = 126;//113;
int S_MAXb = 152;//110;
int V_MAXb = 244;//154;

//pink 
int H_MINp = 120;
int S_MINp = 82;
int V_MINp = 11;
int H_MAXp = 197;
int S_MAXp = 184;
int V_MAXp = 244;

//temp trackbars
int H_MIN = 0;//20;
int S_MIN = 0;//100;
int V_MIN = 0;//100;
int H_MAX = 255;//30;
int S_MAX = 255;//255;
int V_MAX = 255;//255;

int main(int argc, char **argv)
{
   ros::init(argc, argv, "det_hue_node");

   ros::NodeHandle nh;
   image_transport::ImageTransport it(nh);
    
   cv::namedWindow("Origional Image (det_hue_node)", CV_WINDOW_AUTOSIZE);
   cv::destroyWindow("Origional Image (det_hue_node)");

   image_transport::Subscriber sub = it.subscribe(IMAGE_TOPIC, 1, imageCallback);
   pub = nh.advertise<computer_vision::Beacon>(OUTPUT_DATA_TOPIC, 1);

   double time_curr = ros::Time::now().toSec();
   double time_prev = time_curr;
   
   ROS_INFO("Running det_hue_node...");
   ros::spin();
   
   return 0;
}


void imageCallback(const sensor_msgs::ImageConstPtr& raw_image)
{
    double begin = ros::Time::now().toSec(); //start timer
    
    //structures for subscribing and publishing
    cv_bridge::CvImagePtr cv_ptr;
    
    //Image variables
    vector <TrackingObject> Segments;
    vector <TrackingObject> Segments2;
    
    //Output variable
    computer_vision::Beacon outData;
    
    try
    {
        cv_ptr = cv_bridge::toCvCopy(raw_image, enc::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("computer_vision::det_hue_node.cpp::cv_bridge exception: %s", e.what());
        return;
    }
    
    if(SHOW_ORIGIONAL_IMAGE==true) 
    {
		cv::imshow("Origional Image (det_hue_node)", cv_ptr->image);
		cv::waitKey(1);
    }

    cv::Mat imgThreshP = cv_ptr->image;
	filterHue(imgThreshP, 0);
	Segments = findCandidates(cv_ptr->image, imgThreshP);
	pink = Segments.size();
	if(Segments.size()>0)
	{
		cv::Mat imgThreshY = cv_ptr->image;
		filterHue(imgThreshY, 1);
		Segments2 = findCandidates(cv_ptr->image, imgThreshY);
		yellow = Segments2.size();
	}
	else  yellow = 0;
    
    //cout << "(Y, B) = (" << pink << ", " << yellow << ")" << endl;
    
    if(pink>0 && yellow>0)
    {
    	x_beacon=Segments[0].getxPos();
    	x_beacon2=Segments2[0].getxPos();
    	//area_beacon=x_beacon2-x_beacon;
    	area_beacon=Segments2[0].getarea();
    	detected = 1;
    	//cout << "(x,y, area) = (" << x_beacon << ", " << x_beacon2 << ", " << area_beacon << ")" << endl;
    	cout << "detected =" << detected << endl;
    } else detected = 0;
    
    outData.beaconx = x_beacon;
    outData.beacony = y_beacon;
    outData.areabeacon = area_beacon;
    outData.beacon = detected;
    
    pub.publish(outData);
    
    double end = ros::Time::now().toSec(); //stop timer
    if(HZ) ROS_INFO("Sample Rate = %f", 1/(end-begin)); //print execution time
}

string intToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

//Draws detected hues from TrackingObject class in given frame
void drawHueDetection(const cv::Mat &frame, vector <TrackingObject> &Segments)
{
	cv::Mat img = frame;
	
	for (int i = 0; i < Segments.size(); i++)
	{
		Segments.at(i).getxPos();
		Segments.at(i).getyPos();
		Segments.at(i).getarea();

		cv::circle(img, cv::Point(Segments.at(i).getxPos(), Segments.at(i).getyPos()), 10, cv::Scalar(0, 0, 255));
		cv::putText(img, intToString(Segments.at(i).getxPos()) + " , " + intToString(Segments.at(i).getyPos()), 
						cv::Point(Segments.at(i).getxPos(), Segments.at(i).getyPos() + 20), 1, 1, cv::Scalar(0, 255, 0));
		cv::putText(img, Segments.at(i).getType(), cv::Point(Segments.at(i).getxPos(), Segments.at(i).getyPos() - 30), 1, 2, Segments.at(i).getColor());
		cv::putText(img, intToString(Segments.at(i).getarea()), cv::Point(Segments.at(i).getxPos(), Segments.at(i).getyPos() - 20), 1, 1, cv::Scalar(0, 255, 0));
	}
}

double area_=0;
// Returns vector of class TrackingObject of segments constaining circles
vector <TrackingObject> findCandidates(const cv::Mat &frame, const cv::Mat imgThresh)
{
	//Copy frame
	cv::Mat img = frame;
	
	//Object variables
	vector <TrackingObject> Segments;	//vector of candidates
	TrackingObject Segment("Segment");	//candidate of interest
	
	//Contour variables
    vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
    
    //Find contours in threshold image
    cv::findContours(imgThresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	
	//Bounding Box variables
	vector<cv::Rect> boundRect(contours.size());
    vector<vector<cv::Point> > contours_poly(contours.size());
    int numberCircles = 0;

	//If not too noisy and not empty
	if(hierarchy.size() < MAX_NUM_OBJECTS && hierarchy.size()>0) 
	{
		//Loop through each contour
		for(int i = 0; i>=0; i=hierarchy[i][0])
		{
			//Moment for particular contour
			cv::Moments moment = moments((cv::Mat)contours[i]);
    		//If object with size limites
			if((moment.m00>MIN_OBJECT_AREA) && (moment.m00<MAX_OBJECT_AREA))
			{
				//Crop image using bounding box
				cv::approxPolyDP( cv::Mat(contours[i]), contours_poly[i], 3, true );
				boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));

					cv::rectangle(img, boundRect[i].tl(), boundRect[i].br(), cv::Scalar(255,0,255), 2, 8, 0 );
				
					cv::Mat croppedImage = img(boundRect[i]).clone();
					//cout << boundRect[i].tl() << endl;
    
    				//Find if cropped contains circle
    				//numberCircles = findHoughCircles(croppedImage);

    			
    			
    			//If image contains circle save coordinates
    			//if(numberCircles>0)
    			//{
    				//cout << "circles " << numberCircles << endl;
					Segment.setxPos(moment.m10/moment.m00);//x_=moment.m10/area_;
					Segment.setyPos(moment.m01/moment.m00);//y_=moment.m01/area_;
					Segment.setarea(moment.m00);
					Segment.setType(Segment.getType());
					Segment.setColor(Segment.getColor());
    				
					Segments.push_back(Segment);
				//}
    				//cv::Mat H = trackHomingBeacon(croppedImage);
    			//if(DRAW_HUE_TRACKING==true) drawHueDetection(frame, Segments);				
    		}
    	}
    }
    
    /*
    time_curr = ros::Time::now().toSec(); //current time
    
    if(Segments.size()>0 && time_curr-time_prev>7)
    {
    	counter = 0;
    }
       
    if(wait==0) 
    {
    	time_prev = ros::Time::now().toSec();
    	wait = 1;
    }
    else if(counter<15)
    {
    	trackHomingBeacon(frame);
    	wait = 0;
    	counter++;					
    }
	*/
	
	drawHueDetection(frame, Segments);
    if(DRAW_HUE_TRACKING == true) imshow("Hue Tracking", img); cv::waitKey(1);
    return Segments;
}

//Converts given image to binary image of threshold values
void filterHue(cv::Mat &frame, const int &Y)
{
	if(CALIBRATE_MODE==true) createTrackbars(); 

	if(Y==1)
	{
		//temp ball yellow narrow
		H_MINy = 34;
		S_MINy = 58;
		V_MINy = 33;
		H_MAXy = 88;
		S_MAXy = 117;
		V_MAXy = 112;
	}
	else
	{
		H_MINy = 161;
		S_MINy = 89;
		V_MINy = 79;
		H_MAXy = 180;
		S_MAXy = 181;
		V_MAXy = 244;
	}
			
	//Image variables
    cv::Mat imgHSV, imgThresh;
    cv::Mat img = frame;
    
    //Process source image
    cv::GaussianBlur(frame, img, cv::Size(9,9), 0); //(41,41), (11,11)
    cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);															
    
    if(CALIBRATE_MODE==true) cv::inRange(imgHSV, cv::Scalar(H_MIN, S_MIN, V_MIN), cv::Scalar(H_MAX, S_MAX, V_MAX), imgThresh); 
    else cv::inRange(imgHSV, cv::Scalar(H_MINy, S_MINy, V_MINy), cv::Scalar(H_MAXy, S_MAXy, V_MAXy), imgThresh); 
    
    morphImg(imgThresh);																					
    
    frame = imgThresh;
    
	if(SHOW_THRESH_IMAGE) cv::namedWindow("Threshold Image");
    if(SHOW_THRESH_IMAGE) cv::imshow("Threshold Image", imgThresh); cv::waitKey(1);
}

//Returns number of circles in frame
int findHoughCircles(const cv::Mat &frame)
{
	//Image variables
    cv::Mat src, gray;
    
    //Hough Transform variables
    vector<cv::Vec3f> circles;
    
    //Copy input image
    src = frame;
    
    //Process source image
    cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);		//Convert image to grayscale
    //cv::GaussianBlur(gray, gray, cv::Size(11,11), 2, 2);	//Smooth grayscale image
    cv::medianBlur(gray, gray, 27);
    
    //Hough Transform to find circles
    cv::HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 2, gray.rows/8, 300, 5, 1, 1000);
    
    //Find detected circles
	for(size_t i = 0; i < circles.size(); i++ )
	{
		cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));	//find center of circle
		int radius = cvRound(circles[i][2]);								//find radius of circle
		
		if(DRAW_CIRCLES==true) circle(src, center, 3, cv::Scalar(0,255,0), -1, 8, 0 );		//draw center    
		if(DRAW_CIRCLES==true) circle(src, center, radius, cv::Scalar(0,0,255), 3, 8, 0 );	//draw outline
		//cout << "center : " << center << "\nradius : " << radius << endl;
	}
    
    if(DRAW_CIRCLES==true) {imshow("Hough Circle Transform Demo", src); cv::waitKey(1);}
    return circles.size();
}

//Erodes and dilates image for hue tracking
void morphImg(cv::Mat &thresh)
{
	cv::Mat erodeElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	cv::Mat dilateElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);

	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);
}

//This function gets called whenever a trackbar position is changed
void on_trackbar(int, void*) {} 

//Trackbars for calibration
void createTrackbars()
{
	cv::namedWindow("Trackbars", 0);

	char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);
  
	cv::createTrackbar("H_MIN", "Trackbars", &H_MIN, H_MAX, on_trackbar);
	cv::createTrackbar("H_MAX", "Trackbars", &H_MAX, H_MAX, on_trackbar);
	cv::createTrackbar("S_MIN", "Trackbars", &S_MIN, S_MAX, on_trackbar);
	cv::createTrackbar("S_MAX", "Trackbars", &S_MAX, S_MAX, on_trackbar);
	cv::createTrackbar("V_MIN", "Trackbars", &V_MIN, V_MAX, on_trackbar);
	cv::createTrackbar("V_MAX", "Trackbars", &V_MAX, V_MAX, on_trackbar);
}

cv::Mat trackHomingBeacon(const cv::Mat &imgB)
{
    cv::Mat imgA = cv::imread(BEACON_IMG_ADDRESS, CV_LOAD_IMAGE_GRAYSCALE); 
    cv::Mat H;
 
    if (!imgA.data) ROS_WARN("Cannot load beacon image!");
    
    //feature variables
    vector<cv::KeyPoint> keypointsA, keypointsB;
    cv::Mat descriptorsA, descriptorsB;
    vector<vector<cv::DMatch> > matches; 

    //detector, extractor, and matcher declarations
    cv::ORB detector;
    cv::ORB extractor;
	//int minHessian = 800;
	//cv::SurfFeatureDetector detector(minHessian);
	//cv::SurfDescriptorExtractor extractor;

	//Descriptor Match Variables
	//cv::FlannBasedMatcher matcher;
    cv::BFMatcher matcher;

    //computer keypoints
    double t = (double)cv::getTickCount();
    detector.detect(imgA, keypointsA);
    detector.detect(imgB, keypointsB);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    if(HZ) ROS_INFO("detection time [s]: %f", t/1.0);
    
    //extract descriptors
    t = (double)cv::getTickCount();
    extractor.compute(imgA, keypointsA, descriptorsA);
    extractor.compute(imgB, keypointsB, descriptorsB);
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    if(HZ) ROS_INFO("extraction time [s]: %f", t);
    
	//match descriptors
    t = (double)cv::getTickCount();
    matcher.knnMatch(descriptorsA, descriptorsB, matches, 2);  // Find two nearest matches 
    t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
    if(HZ) ROS_INFO("matching time [s]: %f", t);
    
    //Find good matches using the ratio test
    vector<cv::DMatch> good_matches;

    for (int i = 0; i < matches.size(); ++i)
    {
		const float ratio = 0.8; // Value from Lowe's paper
		if (matches[i][0].distance < ratio * matches[i][1].distance)
		{
            good_matches.push_back(matches[i][0]);
		}
    }
	//cout << "Good Matches = " << good_matches.size() << endl;
    if (good_matches.size() >= 4 && good_matches.size() <= 85) //[8,30]
    {
    	
		vector<cv::Point2f> obj;
		vector<cv::Point2f> scene;

		for (unsigned int i = 0; i < good_matches.size(); i++)
		{
            //-- Get the keypoints from the good matches
	 	   obj.push_back(keypointsA[good_matches[i].queryIdx].pt);
		   scene.push_back(keypointsB[good_matches[i].trainIdx].pt);
		}

		H = findHomography(obj, scene, CV_RANSAC);
		
		//Find center of tracked image
		findCenter(imgA, keypointsA, imgB, keypointsB, good_matches, H);
		return H;
    }
	
	H = cv::Mat::zeros(3, 3, CV_32F);
    //H = findHomography(obj, scene, CV_RANSAC);
    return H; 
}

void findCenter(const cv::Mat &imgA, const vector<cv::KeyPoint> &keypointsA, const cv::Mat &imgB, const vector<cv::KeyPoint> &keypointsB, const vector<cv::DMatch> good_matches, const cv::Mat H)
{      
	//Get the corners from the training image
	std::vector<cv::Point2f> obj_corners(4);
	obj_corners[0] = cvPoint(0, 0); 
	obj_corners[1] = cvPoint(imgA.cols, 0);
	obj_corners[2] = cvPoint(imgA.cols, imgA.rows); 
	obj_corners[3] = cvPoint(0, imgA.rows);
	std::vector<cv::Point2f> scene_corners(4);

	perspectiveTransform(obj_corners, scene_corners, H);

	//Draw lines between the corners of beacon
	if(DRAW_FOUND_MATCHES==true) 
	{
		cv::Mat imgMatch;
		drawMatches(imgA, keypointsA, imgB, keypointsB, good_matches, imgMatch);
		cv::line(imgMatch, scene_corners[0] + cv::Point2f(imgA.cols, 0), scene_corners[1] + cv::Point2f(imgA.cols, 0), cv::Scalar(255, 255, 0), 2); //top of beacon
		cv::line(imgMatch, scene_corners[1] + cv::Point2f(imgA.cols, 0), scene_corners[2] + cv::Point2f(imgA.cols, 0), cv::Scalar(255, 0, 0), 2);
		cv::line(imgMatch, scene_corners[2] + cv::Point2f(imgA.cols, 0), scene_corners[3] + cv::Point2f(imgA.cols, 0), cv::Scalar(255, 0, 0), 2);
		cv::line(imgMatch, scene_corners[3] + cv::Point2f(imgA.cols, 0), scene_corners[0] + cv::Point2f(imgA.cols, 0), cv::Scalar(255, 0, 0), 2);
		cv::imshow("Found Matches (findCenter)", imgMatch);
		cv::waitKey(1);
	}
	
	//Find center of beacon (needs to be moved out of drawing function
	cv::Point2f p1 = scene_corners[0] + cv::Point2f(imgA.cols, 0);
	cv::Point2f p2 = scene_corners[1] + cv::Point2f(imgA.cols, 0);
	cv::Point2f p3 = scene_corners[2] + cv::Point2f(imgA.cols, 0);
	cv::Point2f p4 = scene_corners[3] + cv::Point2f(imgA.cols, 0);
	cv::Point2f center = (p3*0.5 + p1*0.5);
	
	x_=center.x;
	y_=center.y;
}

void findRotation(cv::Mat &homography)
{
	//variable declarations
	arma::mat H(3,3), M(3,3), temp(2,2), R(3,3), R2(3,3);
	arma::mat I(3, 3, arma::fill::eye);
	arma::vec na1(3), na2(3), na3(3);
	arma::vec nb1(3), nb2(3), nb3(3);
	arma::vec ta(3), tb(3), es(3), v(3), u(3);
	
	u << 0 << 1 << 0 << arma::endr;
	v << 0 << 0 << 1 << arma::endr;

	//Convert cv::Mat to arma::mat
	H(0,0) = homography.at<double>(0,0);
	H(0,1) = homography.at<double>(0,1);
	H(0,2) = homography.at<double>(0,2);
	H(1,0) = homography.at<double>(1,0);
	H(1,1) = homography.at<double>(1,1);
	H(1,2) = homography.at<double>(1,2);
	H(2,0) = homography.at<double>(2,0);
	H(2,1) = homography.at<double>(2,1);
	H(2,2) = homography.at<double>(2,2);
	
	//Algorithm
	arma::vec m_val = arma::svd(H);
	double gamma = arma::median(m_val);
	H = H/gamma;
	
	arma::mat S = H.st()*H-I;
	//S.print("S =");

	int size = 3;
	for (int q=0;q<size;q++)
		for (int p=0;p<size;p++)
		{
     		int m=0;
     		int n=0;
     		for (int i=0;i<size;i++)
      			for (int j=0;j<size;j++)
        		{
          			if (i!=q && j!=p)
          			{
            			temp(m,n)=S(i,j);
            			if (n<(size-2)) n++;
            			else
             			{
              				n=0;
               				m++;
               			}

            		}
        		}
        	//temp.print("Temp =");
        	M(q,p)=-arma::det(temp);
        }

	na1 << S(0,0) << S(0,1) + sqrt(M(2,2)) << S(0,2) + sgn(M(1,2))*sqrt(M(1,1)) << arma::endr;
	na1 = na1/arma::norm(na1);
	na2 << S(0,1)+sqrt(M(2,2)) << S(1,1) <<	S(1,2)-sgn(M(0,2))*sqrt(M(0,0)) << arma::endr;
	na2 = na2/arma::norm(na2);
	na3 << S(0,2)+sgn(M(0,1))*sqrt(M(1,1)) << S(1,2)+sqrt(M(0,0)) << S(2,2) << arma::endr;
	na3 = na3/arma::norm(na3);
	nb1 << S(0,0) << S(0,1)-sqrt(M(2,2)) << S(0,2)-sgn(M(1,2))*sqrt(M(1,1)) << arma::endr;
	nb1 = nb1/arma::norm(nb1);
	nb2 << S(0,1)-sqrt(M(2,2)) << S(1,1) <<	S(1,2)+sgn(M(0,2))*sqrt(M(0,0)) << arma::endr;
	nb2 = nb2/arma::norm(nb2);
	nb3 << S(0,2)-sgn(M(0,1))*sqrt(M(1,1)) << S(1,2)-sqrt(M(0,0)) << S(2,2) << arma::endr;
	nb3 = nb3/norm(nb3);
	
	double V = sqrt(2*((1+arma::trace(S))*(1+arma::trace(S))+1-trace(S*S)));
	es << sgn(S(0,0)) << sgn(S(1,1)) << sgn(S(2,2)) << arma::endr;
	double te = sqrt(2+arma::trace(S)-V);
	double p = sqrt(2+arma::trace(S)+V);
	
	
	if(norm(na1-nb1) > 0.0001)
	{
		ta = (te/2)*(es(0)*p*nb1-te*na1);
		tb = (te/2)*(es(0)*p*na1-te*nb1);
		if(na1(2) > nb1(2))
		{
			R = H*(I-(2/V)*ta*na1.t());
			R2 = H*(I-(2/V)*tb*nb1.t());
		}
		else
		{
			R = H*(I-(2/V)*tb*nb1.t());
			R2 = H*(I-(2/V)*ta*na1.t());
		}
	}
	else if (norm(na2-nb2) > 0.0001)
	{
		ta = (te/2)*(es(1)*p*nb2-te*na2);
		tb = (te/2)*(es(1)*p*na2-te*nb2);
		if(na2(2) > nb2(2))
		{
			R = H*(I-(2/V)*ta*na2.t());
			R2 = H*(I-(2/V)*tb*nb2.t());
		}
		else
		{
			R = H*(I-(2/V)*tb*nb2.t());
			R2 = H*(I-(2/V)*ta*na2.t());
		}
	}
	else if (norm(na3-nb3) > 0.0001)
	{
		ta = (te/2)*(es(2)*p*nb3-te*na3);
		tb = (te/2)*(es(2)*p*na3-te*nb3);
		if(na3(2) > nb3(2))
		{
			R = H*(I-(2/V)*ta*na3.t());
			R2 = H*(I-(2/V)*tb*nb3.t());
		}
		else
		{
			R = H*(I-(2/V)*tb*nb3.t());
			R2 = H*(I-(2/V)*ta*na3.t());
		}
	}
	
	if(DEBUG_HOMOGRAPHY==true)
	{
		arma::vec ta1(3), ta2(3), ta3(3);
		arma::vec tb1(3), tb2(3), tb3(3);
		arma::mat Ra1(3,3), Ra2(3,3), Ra3(3,3);
		arma::mat Rb1(3,3), Rb2(3,3), Rb3(3,3);
		
		na1.print("na1 =");
		na2.print("na2 =");
		na3.print("na3 =");
		nb1.print("nb1 =");
		nb2.print("nb2 =");
		nb3.print("nb3 =");
	
		ta1 = (te/2)*(es(0)*p*nb1-te*na1);
		ta2 = (te/2)*(es(1)*p*nb2-te*na2);
		ta3 = (te/2)*(es(2)*p*nb3-te*na3);
		tb1 = (te/2)*(es(0)*p*na1-te*nb1);
		tb2 = (te/2)*(es(1)*p*na2-te*nb2);
		tb3 = (te/2)*(es(2)*p*na3-te*nb3);
	
		Ra1 = H*(I-(2/V)*ta1*na1.t());
		Ra2 = H*(I-(2/V)*ta2*na2.t());
		Ra3 = H*(I-(2/V)*ta3*na3.t());
		Rb1 = H*(I-(2/V)*tb1*nb1.t());
		Rb2 = H*(I-(2/V)*tb2*nb2.t());
		Rb3 = H*(I-(2/V)*tb3*nb3.t());
	
		Ra1.print("Ra1 =");
		Ra2.print("Ra2 =");
		Ra3.print("Ra3 =");
		Rb1.print("Rb1 =");
		Rb2.print("Rb2 =");
		Rb3.print("Rb3 =");
		
		H.print("H = ");
		M.print("M = ");

	}
	ta.print("ta = ");
	tb.print("tb = ");
	R.print("R = ");
	R2.print("R2 = ");
}

//returns 1 for positive and 0 values, and -1 for negative
template <typename T> int sgn(T val) {
	if((T(0) < val) - (val < T(0)) == 0) return 1;
    else return (T(0) < val) - (val < T(0));
}

   
//EXPERIMENTAL
int findNumberHues(cv::Mat &frame, TrackingObject Object, vector <TrackingObject> &Objects)
{
	//Image variables
    cv::Mat imgHSV, imgThresh;
    cv::Mat img = frame;
    
    cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV); //Convert to HSV
    cv::inRange(imgHSV, cv::Scalar(H_MINy, S_MINy, V_MINy), cv::Scalar(H_MAXy, S_MAXy, V_MAXy), imgThresh); //Find threshold image
    morphImg(imgThresh); //Morph threshold
    
	//Find contours in threshold image
    vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
    cv::findContours(imgThresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
    
    int counter = 0;
    if(hierarchy.size()>0 && hierarchy.size() < MAX_NUM_OBJECTS) //if num obj greater than max, too noisy
    {
    	for(int i = 0; i>=0; i=hierarchy[i][0])
    	{
    		cv::Moments moment = moments((cv::Mat)contours[i]);
    		if((moment.m00>MIN_OBJECT_AREA) && (moment.m00<MAX_OBJECT_AREA)) counter++;	
    	}
    }
    if(counter==4) cv::imshow("Cropped Threshold Image", img); cv::waitKey(1);
    return counter;
}   
    
    
/*   
//Complicated and terrible method check if homing beacon is located based on position of hues in region
if(SideOnes.size()>3) 
{
	int idx[SideOnes.size()][SideOnes.size()];
	int dVec[SideOnes.size()][SideOnes.size()];
	//cout << SideOnes.size() << endl;
	for(int i=0; i < SideOnes.size(); i++)
	{
		for (int j=0; j < SideOnes.size(); j++)
		{
			if(i!=j)
			{
				double dh = abs(SideOnes.at(i).getyPos()-SideOnes.at(j).getyPos());
				if(dh<25) //check if horizontally aligned
				{
					idx[i][j]=1; 
					dVec[i][j]=dh;
					cout << "(" << i << ", " << j << ")" << endl;
				}
				else
				{
					idx[i][j]=0;
				}
			}
			else idx[i][j]=0;

		}
	}
    	
	//Check for mismatches
	int errorIdx[SideOnes.size()];
	int errorCount = 0;
	for(int i=0; i < SideOnes.size(); i++)
	{
		int sumN = 0;
    		
		for(int j=0; j<SideOnes.size(); j++) sumN = sumN + idx[i][j]; //sum of row
    	
		if(sumN>1) //if sum of row > 1, then mismatch occurred
		{
 			//remember which row has the error
			errorIdx[i]=1;
			errorCount++;
		}
	}
    	
cout << "num of errors" << errorCount << endl;
}
else cout << "Homing beacon not detected..." << endl;
*/
