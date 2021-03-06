#include "ros/ros.h"
#include <sstream>
#include <stdint.h>
#include "roboteq_interface/motor_speed_control.h"
#include "roboteq_interface/left_motor_encoders.h"
#include "roboteq_interface/right_motor_encoders.h"
#include "computer_vision/Beacon.h"
#include <math.h>

using namespace ros;

int16_t front_left_motor_speed;
int16_t front_right_motor_speed;
int16_t rear_left_motor_speed;
int16_t rear_right_motor_speed;
int16_t heading;
int16_t delta_heading;
uint8_t object_seen;
const int16_t heading_setpoint = 512; // pixels
const double turning_gain = -1.25; // speed / pixels
const int16_t heading_tolerance = 200; // pixels. Within this tolerance, is close enough to be "aimed at" beacon
int16_t turning_speed;
int16_t image_size;
const int16_t stop_threshold = 500;
static int count = 0;

roboteq_interface::motor_speed_control msg_out;

void driveStraight(int speed);
void pivot(int speed);
void callback(const computer_vision::Beacon::ConstPtr& msg_in);
void packMsg();

int main(int argc, char **argv)
{
	ros::init(argc, argv, "homing_test");
	ros::NodeHandle n;
	ros::Publisher pub = n.advertise<roboteq_interface::motor_speed_control>("motor_speed_control",1000);
	ros::Subscriber sub = n.subscribe<computer_vision::Beacon>("det/mis_out_data",1000,callback);
	ros::Rate loop_rate(20);
	while (ros::ok())
	{
		if (object_seen == 1)
		{
			count = count+1;
		}
		else
		{
			count = 0;
		}
		if (count > 3) // If object is seen, aim towards heading
		{
			//ROS_INFO("%i",count);
			delta_heading = heading-heading_setpoint;
			if (abs(delta_heading) <= heading_tolerance) // If delta_heading is within heading_tolerance, drive toward beacon
			{
				//ROS_INFO("%s","Driving Straight");
				driveStraight(200);
			}
			else // If delta_heading is outside heading_tolerance, rotate to face beacon
			{
				
				turning_speed = round(delta_heading*turning_gain);
				if (turning_speed >= 1000)
				{
					//ROS_INFO("%s","Turning CCW");
					turning_speed = 1000;
				}
				else if (turning_speed <=-1000)
				{
					//ROS_INFO("%s","Turning CW");
					turning_speed = -1000;
				}
			pivot(turning_speed);
			}
		}
		else // if object is not seen, pivot CCW continuously to search
		{
			//ROS_INFO("%s","Object Not Seen");
			turning_speed = -200;
			pivot(turning_speed);
		}
		if (image_size > stop_threshold)
		{
			driveStraight(0);
		}
		packMsg();
		pub.publish(msg_out);
		ros::spinOnce();
	}
	return 0;
}

void callback(const computer_vision::Beacon::ConstPtr& msg_in)
{
	//ROS_INFO("%s","Callback");
	object_seen = msg_in->object;
	heading = msg_in->centerx;
	image_size = msg_in->x;
}

void driveStraight(int speed) // Positive speed = forward, negative = backward. Range of speed: [-1000,1000]
{
	front_left_motor_speed = speed;
	front_right_motor_speed = speed;
	rear_left_motor_speed = speed;
	rear_right_motor_speed = speed;
}

void pivot(int speed) // Positive speed = CCW, negative = CW. Range of speed: [-1000,1000]
{
	front_left_motor_speed = -speed;
	front_right_motor_speed = speed;
	rear_left_motor_speed = -speed;
	rear_right_motor_speed = speed;
}

void packMsg()
{
	msg_out.cont_1_motor_1_speed_cmd = front_left_motor_speed;
	msg_out.cont_1_motor_2_speed_cmd = rear_left_motor_speed;
	msg_out.cont_2_motor_1_speed_cmd = front_right_motor_speed;
	msg_out.cont_2_motor_2_speed_cmd = rear_right_motor_speed;
}
