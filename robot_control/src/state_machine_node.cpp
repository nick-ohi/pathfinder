#include <ros/ros.h>
#include <stdint.h>
#include <math.h>
#include "robot_actuators_class.h"
#include "robot_status_class.h"
#include "search_mode.h"
#include "approach_mode.h"
#include "return_to_base_mode.h"
#include <roboteq_interface/motor_commands.h>
#include <serial_comm/pkt_7_msg.h>
#include <serial_comm/pkt_5_msg.h>
#include <navigation/FilterOutput.h>
#include <computer_vision/Beacon.h>

using namespace std;

Robot_Status robot_status;

void packMsg();

int main(int argc, char **argv)
{
    // Node Initialization
    ros::init(argc, argv, "state_machine_node");
    ros::NodeHandle nh;
    ros::Rate loop_rate(20);
    // ****** Need to add pubs and subs*******
    // Instantiate state machine objects
    Actuators actuators;
    Search_Class search;
    Approach_Class approach;
    Return_to_base_Class return_to_base;
    // Instantiate message objects
    roboteq_interface::motor_commands motor_msg_out;
    serial_comm::pkt_7_msg sib_msg_out;
    // State control variables
    int search_ended;
    int approach_end_condition; // 0 = continuing approach, 1 = approach complete, advance to Collect, 2 = need to Re_orient
    int return_to_base_ended;
    enum modes {Pause, Init, Travel_to_search Search, Approach, Re_orient, Collect, Confirm_collect, Return_to_path, Return_to_base, Deposit, Confirm_deposit, Lost, Finish, Avoidance} mode;
    mode = Init;
    static modes previous_mode = mode;
    while(ros::ok())
    {
    	
    	switch(mode)
    	{
    		case Pause:
    			actuators.drive.stop();
    			actuators.gimbal.hold(&robot_status);
    			if(robot_status.pause_switch) mode = Pause;
    			else mode = previous_mode;
    		case Init:
    			actuators.drive.stop();
    			actuators.gimbal.hold(&robot_status);
    			previous_mode = mode;
    			ros::Duration(15).sleep(); // wait for 15 seconds for initialization
    			mode = Search;
    			//if(***STUFF***) mode = Search;
    			//else mode = Init;
    			break;
			/*case Travel_to_search:
				actuators.drive.stop();
    			actuators.gimbal.hold(&robot_status);
    			previous_mode = mode;
				if(***STUFF***) mode = Search;
    			else mode = Travel_to_search;
    			break;*/
    		case Search:
    			search_ended = search.spiral.run(&actuators, &robot_status)
    			previous_mode = mode;
    			if(search_ended==1) mode = Approach;
    			else mode = Search;
    			break;
			case Approach:
				approach_end_condition = approach.run(search.spiral.gimbal_target_yaw_angle, &actuators, &robot_status);
				previous_mode = mode;
				if(approach_end_condition == 1) mode = Collect;
				//else if(approach_end_condition == 2) mode = Re_orient;
				else mode = Approach;
				break;
			case Collect: // Temporarily just passing through this mode until grabber finished
				ros::Duration(5).sleep();
				previous_mode = mode;
				mode = Return_to_base;
				break;
			case Return_to_base:
				return_to_base_ended = return_to_base.run(return_to_base.gimbal_target_yaw_angle, &actuators, &robot_status);
				previous_mode = mode;
				if(return_to_base_ended == 1) mode = Finish; // *** This needs to be changed to Deposit mode later***
				else mode = Return_to_base;
				break;
			case Finish;
				actuators.drive.stop();
				actuators.gimbal.hold(&robot_status);
				previous_mode = mode;
				mode = Finish;
				break;
			default:
				actuators.drive.stop();
				actuators.gimbal.hold(&robot_status);
				previous_mode = Init;
				mode = Pause;
				break;
		}
		packMsg();
		ros::spinOnce();
		loop_rate.sleep();
    }
    return 0;
}

void sibCallback(const serial_comm::pkt_5_msg::ConstPtr& sib_msg_in)
{
	if(sib_msg_in->Stepper < 32768) robot_status.gimbal_yaw_angle = sib_msg_in->Stepper/10.0;
	else robot_status.gimbal_yaw_angle = (sib_msg_in->Stepper - 65536)/10.0;
}

void navCallback(const navigation::FilterOutput::ConstPtr& nav_msg_in)
{
	robot_status.heading = nav_msg_in->yaw;
	robot_status.bearing = nav_msg_in->bearing;
	robot_status.polar_distance = nav_msg_in->distance;
}

void visionCallback(const computer_vision::Beacon::ConstPtr& det_msg_in)
{
	robot_status.object_seen = det_msg_in.object;
	robot_status.beacon_seen = det_msg_in.beacon;
	//robot_status.object_in_range = det_msg_in.object_in_range;
	//robot_status.beacon_in_range = det_msg_in.beacon_in_range;
}

void packMsg()
{
	if(actuators.drive.front_left_motor_speed>1000) actuators.drive.front_left_motor_speed = 1000;
	else if(actuators.drive.front_left_motor_speed<-1000) actuators.drive.front_left_motor_speed = -1000;
	if(actuators.drive.rear_left_motor_speed>1000) actuators.drive.rear_left_motor_speed = 1000;
	else if(actuators.drive.rear_left_motor_speed<-1000) actuators.drive.rear_left_motor_speed = -1000;
	if(actuators.drive.front_right_motor_speed>1000) actuators.drive.front_right_motor_speed = 1000;
	else if(actuators.drive.front_right_motor_speed<-1000) actuators.drive.front_right_motor_speed = -1000;
	if(actuators.drive.rear_right_motor_speed>1000) actuators.drive.rear_right_motor_speed = 1000;
	else if(actuators.drive.rear_right_motor_speed<-1000) actuators.drive.rear_right_motor_speed = -1000;
	motor_msg_out.cont_1_motor_1_speed_cmd = actuators.drive.middle_left_motor_speed;
	motor_msg_out.cont_1_motor_2_speed_cmd = actuators.drive.rear_left_motor_speed;
	motor_msg_out.cont_2_motor_1_speed_cmd = actuators.drive.middle_right_motor_speed;
	motor_msg_out.cont_2_motor_2_speed_cmd = actuators.drive.rear_right_motor_speed;
	motor_msg_out.cont_3_motor_1_speed_cmd = actuators.drive.front_left_motor_speed;
	motor_msg_out.cont_3_motor_2_speed_cmd = actuators.drive.front_right_motor_speed;
	if(actuators.gimbal.pitch_angle > actuators.gimbal.gimbal_max_angle) actuators.gimbal.pitch_angle = actuators.gimbal.gimbal_max_angle;
	else if(actuators.gimbal.pitch_angle < actuators.gimbal.gimbal_min_angle) actuators.gimbal.pitch_angle = actuators.gimbal.gimbal_min_angle;
	if(actuators.gimbal.roll_angle > actuators.gimbal.gimbal_max_angle) actuators.gimbal.roll_angle = actuators.gimbal.gimbal_max_angle;
	else if(actuators.gimbal.roll_angle < actuators.gimbal.gimbal_min_angle) actuators.gimbal.roll_angle = actuators.gimbal.gimbal_min_angle;
	if(actuators.gimbal.yaw_angle > actuators.gimbal.gimbal_max_angle) actuators.gimbal.yaw_angle = actuators.gimbal.gimbal_max_angle;
	else if(actuators.gimbal.yaw_angle < actuators.gimbal.gimbal_min_angle) actuators.gimbal.yaw_angle = actuators.gimbal.gimbal_min_angle;
	sib_msg_out.Desired_Angle_1 = round(actuators.gimbal.pitch_angle*10);
	sib_msg_out.Desired_Angle_2 = round(actuators.gimbal.roll_angle*10);
	sib_msg_out.Desired_Angle_3 = round(actuators.gimbal.yaw_angle*10);
}
