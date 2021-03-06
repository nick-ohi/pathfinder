#include <ros/ros.h>
#include <navigation/FilterOutput.h>
#include <roboteq_interface/motor_commands.h>
#include <armadillo>
#include <stdint.h>
#include <computer_vision/Beacon.h>
#include <math.h>

using namespace std;

static const char DATA_TOPIC1[] = "nav/filter_output";
static const char OUTPUT_TOPIC[] = "mis/motor_commands";
static const char DATA_TOPIC2[] = "det/mis_out_data";

const float PI = 3.14159265;

const float bearing_turning_gain = 10.0; // (delta Motor power)/(degree)
const float vision_turning_gain = 2.0; // (Motor power)/(pixel)
const int vision_turning_setpoint = 320; // pixels
const int vision_turning_threshold = 50; // pixels

int16_t front_left_motor_speed = 0;
int16_t front_right_motor_speed = 0;
int16_t rear_left_motor_speed = 0;
int16_t rear_right_motor_speed = 0;

const int object_area_threshold = 2000; // pixels
const int beacon_area_threshold = 10000; // pixels

void pivot(int speed);
void driveStraight(int speed);

/*motor_commands.msg
int16 cont_1_motor_1_speed_cmd
int16 cont_1_motor_2_speed_cmd
int16 cont_2_motor_1_speed_cmd
int16 cont_2_motor_2_speed_cmd
int16 cont_3_motor_1_speed_cmd
int16 cont_3_motor_2_speed_cmd
*/

class Data //Class containing callback function for data coming into node
{
public:
  ros::Subscriber sub1;
  ros::Subscriber sub2;
  float velocity, deltaD, roll, pitch, yaw, distance, bearing, object_area, beacon_area;
  uint16_t object_x, object_y, beacon_x, beacon_y;
  uint8_t object_seen, beacon_seen;

  Data() : velocity(0), deltaD(0), roll(0), pitch(0), yaw(0), distance(0), bearing(0),
		object_seen(0), object_x(0), object_y(0), beacon_seen(0),
		beacon_x(0), beacon_y(0), beacon_area(0), object_area(0)
  {
    ros::NodeHandle node;
    sub1 = node.subscribe(DATA_TOPIC1, 1, &Data::getData1Callback, this);
    sub2 = node.subscribe(DATA_TOPIC2, 1, &Data::getData2Callback, this);
  }

  void getData1Callback(const navigation::FilterOutput::ConstPtr &msg1) {
    this->velocity = msg1->velocity;
    this->deltaD = msg1->deltaDistance;
    this->roll = msg1->roll;
    this->pitch = msg1->pitch;
    this->yaw = msg1->yaw;
    this->distance = msg1->distance;
    this->bearing = msg1->bearing;
  }

  void getData2Callback(const computer_vision::Beacon::ConstPtr &msg2) {
    this->object_seen = msg2->object;
    this->object_x = msg2->centerx;
    this->object_y = msg2->centery;
    this->object_area = msg2->areaobject;
    this->beacon_seen = msg2->beacon;
    this->beacon_x = msg2->beaconx;
    this->beacon_y = msg2->beacony;
    this->beacon_area = msg2->areabeacon;
  }

};

int main(int argc, char **argv)
{
    //Node Initialization
    ros::init(argc, argv, "nav_testing_node");
    ros::NodeHandle nh;
    ros:: Publisher pub = nh.advertise<roboteq_interface::motor_commands>(OUTPUT_TOPIC,1);
    
    ROS_INFO("nav_testing_node running...");
    
    //Input and Output Data
    roboteq_interface::motor_commands outData;	//structure for output data
    
    //State
    static int state = 0;
    static int object_seen_counter = 0;
    static int beacon_seen_counter = 0;

    Data inData;//structure for input data
    
    while(ros::ok())
    {	
    	if(state==0)//Drive to initial positoin
    	{
    		driveStraight(500);
    		if(inData.distance>=1030) state=1;
    		else state=0;
    	}
    	else if(state==1)//Look for sample
    	{
    		pivot(500);
    		if(inData.object_seen==1) object_seen_counter++;
		else object_seen_counter=0;
		if(object_seen_counter>=5) state=2;
    		else state=1;
    	}
	else if(state==2)//Rotate to sample
	{
		pivot(round((inData.object_x-vision_turning_setpoint)*vision_turning_gain));
		if(abs(inData.object_x-vision_turning_setpoint)<=vision_turning_threshold)
		{
			state=3;
		}
		else state = 2;
	}
    	else if(state==3)//Drive to sample
    	{
    		driveStraight(500);
    		if(inData.object_area>=object_area_threshold) state=4;
    		else state=3;
    	}
    	else if(state==4)//Stop at sample
    	{
    		driveStraight(0);
		ros::Duration(5).sleep();
		state=5;
    	}
	else if(state==5)//Look for beacon
	{
		pivot(300);
    		if(inData.beacon_seen==1) beacon_seen_counter++;
		else beacon_seen_counter=0;
		if(beacon_seen_counter>=3) state=7;
    		else state=5;
    	}
	else if(state==6)//Pivot to beacon
	{
		pivot(round((inData.beacon_x-vision_turning_setpoint)*vision_turning_gain));
		if(abs(inData.beacon_x-vision_turning_setpoint)<=vision_turning_threshold)
		{
			state=7;
		}
		else state = 6;
	}
	else if(state==7)//Drive back to beacon
	{
		driveStraight(500);
    		if(inData.beacon_area>=beacon_area_threshold) state=8;
    		else state=7;
	}
	else//Stop at beacon
	{
		driveStraight(0);
		state=8;
	}
    	
    	
    	outData.cont_1_motor_1_speed_cmd=front_left_motor_speed;
    	outData.cont_1_motor_2_speed_cmd=rear_left_motor_speed;
    	outData.cont_2_motor_1_speed_cmd=front_right_motor_speed;
    	outData.cont_2_motor_2_speed_cmd=rear_right_motor_speed;
    	//cout << front_left_motor_speed << endl;
    	pub.publish(outData);
    	
		ros::spinOnce();
		ros::Duration(0.02).sleep(); //50HZ
	}
}

void driveStraight(int speed) // Positive speed = forward, negative = backward. Range of speed: [-1000,1000]
{
	if(speed > 1000) speed=1000;
	else if(speed < -1000) speed = -1000;
	front_left_motor_speed = speed;
	front_right_motor_speed = speed;
	rear_left_motor_speed = speed;
	rear_right_motor_speed = speed;
}

void pivot(int speed) // Positive speed = CW, negative = CCW. Range of speed: [-1000,1000]
{
	if(speed > 1000) speed=1000;
	else if(speed < -1000) speed = -1000;
	front_left_motor_speed = speed;
	front_right_motor_speed = -speed;
	rear_left_motor_speed = speed;
	rear_right_motor_speed = -speed;
}
/*
void driveBearing(int speed, float bearing_setpoint) // Positive speed = forward, negative = backward. Range of speed: [-1000,1000]; bearing_setpoint is in degrees.
{
	int left_speed;
	int right_speed;

	left_speed = speed+round(bearing_turning_gain*(bearing_setpoint-inData.bearing));
	right_speed = speed+round(bearing_turning_gain*(inData.bearing-bearing_setpoint));

	front_left_motor_speed = left_speed;
	front_right_motor_speed = right_speed;
	rear_left_motor_speed = left_speed;
	rear_right_motor_speed = right_speed;
}
*/	
