#include <ros/ros.h>
#include <stdint.h>
#include <math.h>
#include <serial_comm/pkt_7_msg.h>
#include <serial_comm/pkt_5_msg.h>

using namespace std;

int16_t gimbal_desired_angle_output = 0; // Degrees*10
float gimbal_current_angle = 0.0;

void rotateGimbal(double desired_angle);
void callback(const serial_comm::pkt_5_msg::ConstPtr& msg_in);

int main(int argc, char **argv)
{
	float gimbal_desired_angle = 0.0; //degrees
    static unsigned int step = 0;
    const float gimbal_angle_deadband = 1.0;
    //Node Initialization
    ros::init(argc, argv, "gimbal_test_node");
    ros::NodeHandle nh;
    ros::Publisher pub = nh.advertise<serial_comm::pkt_7_msg>("mis/sib_out_data",1);
    ros::Subscriber sub = nh.subscribe<serial_comm::pkt_5_msg>("mis/sib_in_data",1,callback);
    ros::Rate loop_rate(50);
    serial_comm::pkt_7_msg msg_out;
    while(ros::ok())
   	{
   		switch (step)
   		{
   			case 0:
   				gimbal_desired_angle = 90.0; // Degrees
   				rotateGimbal(gimbal_desired_angle);
   				//ROS_INFO("Current Angle in main %f", gimbal_current_angle);
   				if(fabs(gimbal_current_angle - gimbal_desired_angle) <= gimbal_angle_deadband) step = 1;
   				else step = 0;
   				break;
   			case 1:
   				gimbal_desired_angle = 0.0; // Degrees
   				rotateGimbal(gimbal_desired_angle);
   				if(fabs(gimbal_current_angle - gimbal_desired_angle) <= gimbal_angle_deadband) step = 2;
   				else step = 1;
   				break;
   			case 2:
   				gimbal_desired_angle = -90.0; // Degrees
   				rotateGimbal(gimbal_desired_angle);
   				if(fabs(gimbal_current_angle - gimbal_desired_angle) <= gimbal_angle_deadband) step = 3;
   				else step = 2;
   				break;
   			case 3:
   				gimbal_desired_angle = 0.0; // Degrees
   				rotateGimbal(gimbal_desired_angle);
				step = 3;
   				break;
   		}
   		ROS_INFO("%i %f", step, gimbal_current_angle);
   		msg_out.Desired_Angle_3 = gimbal_desired_angle_output;
   		pub.publish(msg_out);
   		ros::spinOnce();
   		loop_rate.sleep();
   	}
   	return 0;
}

void rotateGimbal(double desired_angle)
{
	gimbal_desired_angle_output = round(desired_angle*10);
}

void callback(const serial_comm::pkt_5_msg::ConstPtr& msg_in)
{
	//ROS_INFO("%s","Callback");
	if(msg_in->Stepper < 32768) gimbal_current_angle = msg_in->Stepper/10.0;
	else gimbal_current_angle = (msg_in->Stepper - 65536)/10.0;
	//ROS_INFO("Current Angle in Callback %f", gimbal_current_angle);
}
	