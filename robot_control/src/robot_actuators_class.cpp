#include "robot_actuators_class.h"
#include <stdint.h>
#include <math.h>
#include <iostream>

using namespace std;

int Drive::stop() // Stop all drive motors
{
	front_left_motor_speed = 0;
	middle_left_motor_speed = 0;
	rear_left_motor_speed = 0;
	front_right_motor_speed = 0;
	middle_left_motor_speed = 0;
	rear_right_motor_speed = 0;
	return 1;
}

// Drive straight a distance specified in "desired_distance" while maintaining the initial heading. Positive distance is forward. Speed Range: [0,1000]	
int Drive::driveStraightDist(int speed, float desired_distance, Robot_Status* robot_status)
{
	int left_speed;
	int right_speed;
	static int drive_sign;
	const float heading_turning_gain = 100.0;
	static substates task_state = Init_Task;
	static float initial_distance;
	static float desired_heading;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			initial_distance = robot_status->absolute_distance;
			desired_heading = robot_status->heading;
			if (desired_distance < 0) drive_sign = -1;
			else drive_sign  = 1;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			left_speed = speed+round(heading_turning_gain*(desired_heading-robot_status->heading));
			right_speed = speed+round(heading_turning_gain*(robot_status->heading-desired_heading));
			front_left_motor_speed = left_speed*drive_sign;
			middle_left_motor_speed = left_speed*drive_sign;
			rear_left_motor_speed = left_speed*drive_sign;
			front_right_motor_speed = right_speed*drive_sign;
			middle_right_motor_speed = right_speed*drive_sign;
			rear_right_motor_speed = right_speed*drive_sign;
			task_ended = 0;
			if(fabs(robot_status->absolute_distance-initial_distance)>=fabs(desired_distance)) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

// Drive straight until stop = 1. Continue while stop = 0. Maintain the initial heading. reverse = 1, drive in reverse. else, drive forward. Speed Range: [0,1000]
int Drive::driveStraightTrigger(int speed, int reverse, int stop, Robot_Status* robot_status)
{
	int left_speed;
	int right_speed;
	static int drive_sign;
	const float heading_turning_gain = 100.0;
	static substates task_state = Init_Task;
	static float desired_heading;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			desired_heading = robot_status->heading;
			if (reverse == 1) drive_sign = -1;
			else drive_sign  = 1;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			left_speed = speed+round(heading_turning_gain*(desired_heading-robot_status->heading));
			right_speed = speed+round(heading_turning_gain*(robot_status->heading-desired_heading));
			front_left_motor_speed = left_speed*drive_sign;
			middle_left_motor_speed = left_speed*drive_sign;
			rear_left_motor_speed = left_speed*drive_sign;
			front_right_motor_speed = right_speed*drive_sign;
			middle_right_motor_speed = right_speed*drive_sign;
			rear_right_motor_speed = right_speed*drive_sign;
			task_ended = 0;
			if(stop == 1) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}
// Drive straight until stop = 1. Continue while stop = 0. Maintain the specified heading. reverse = 1, drive in reverse. else, drive forward. Speed Range: [0,1000]
int Drive::driveStraightTriggerHeading(int speed, int reverse, int stop, float desired_heading Robot_Status* robot_status)
{
	int left_speed;
	int right_speed;
	static int drive_sign;
	const float heading_turning_gain = 100.0;
	static substates task_state = Init_Task;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			if (reverse == 1) drive_sign = -1;
			else drive_sign  = 1;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			left_speed = speed+round(heading_turning_gain*(desired_heading*PI/180-robot_status->heading));
			right_speed = speed+round(heading_turning_gain*(robot_status->heading-desired_heading*PI/180));
			front_left_motor_speed = left_speed*drive_sign;
			middle_left_motor_speed = left_speed*drive_sign;
			rear_left_motor_speed = left_speed*drive_sign;
			front_right_motor_speed = right_speed*drive_sign;
			middle_right_motor_speed = right_speed*drive_sign;
			rear_right_motor_speed = right_speed*drive_sign;
			task_ended = 0;
			if(stop == 1) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

// Pivot to a desired absolute heading angle. Positive heading angles are positive. Speed Range: [0,1000]
int Drive::pivotAbsolute(int speed, float desired_angle, Robot_Status* robot_status)
{
	static int pivot_sign = 1;
	static substates task_state = Init_Task;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			if((desired_angle*PI/180-robot_status->heading)>0) pivot_sign = 1;
			else pivot_sign = -1;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			front_left_motor_speed = speed*pivot_sign;
			middle_left_motor_speed = speed*pivot_sign;
			rear_left_motor_speed = speed*pivot_sign;
			front_right_motor_speed = -speed*pivot_sign;
			middle_right_motor_speed = -speed*pivot_sign;
			rear_right_motor_speed = -speed*pivot_sign;
			task_ended = 0;
			if(((pivot_sign>=0)&&(desired_angle*PI/180-robot_status->heading)<=0.0)||((pivot_sign<0)&&(desired_angle*PI/180-robot_status->heading)>=0.0)) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

// Pivot to a desired delta heading angle. Positive heading angles are positive. Speed Range: [0,1000]
int Drive::pivotDelta(int speed, float desired_delta_angle, Robot_Status* robot_status)
{
	static float initial_angle;
	static int pivot_sign = 1;
	static substates task_state = Init_Task;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			if(desired_delta_angle>0.0) pivot_sign = 1;
			else pivot_sign = -1;
			initial_angle = robot_status->heading;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			front_left_motor_speed = speed*pivot_sign;
			middle_left_motor_speed = speed*pivot_sign;
			rear_left_motor_speed = speed*pivot_sign;
			front_right_motor_speed = -speed*pivot_sign;
			middle_right_motor_speed = -speed*pivot_sign;
			rear_right_motor_speed = -speed*pivot_sign;
			task_ended = 0;
			if(((pivot_sign>=0)&&(robot_status->heading-initial_angle)>=desired_delta_angle*PI/180)||((pivot_sign<0)&&(robot_status->heading-initial_angle)<=desired_delta_angle*PI/180)) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

// Pivot until stop = 1. Continue while stop = 0. Speed Range: [0,1000]. CCW = 1, roate counterclockwise. Else, rotate clockwise
int Drive::pivotTrigger(int speed, int CCW, int stop, Robot_Status* robot_status)
{
	static int pivot_sign = 1;
	static substates task_state = Init_Task;
	int task_ended = 0;
	
	switch(task_state)
	{
		case Init_Task:
			if(CCW==0) pivot_sign = -1;
			else pivot_sign = 1;
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			front_left_motor_speed = speed*pivot_sign;
			middle_left_motor_speed = speed*pivot_sign;
			rear_left_motor_speed = speed*pivot_sign;
			front_right_motor_speed = -speed*pivot_sign;
			middle_right_motor_speed = -speed*pivot_sign;
			rear_right_motor_speed = -speed*pivot_sign;
			task_ended = 0;
			if(stop == 1) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			front_left_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_left_motor_speed = 0;
			front_right_motor_speed = 0;
			middle_left_motor_speed = 0;
			rear_right_motor_speed = 0;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

// Insert grabber functions here****

//***

//Gimbal::gimbal_max_angle = 135.0; // Degrees
//Gimbal::gimbal_min_angle = -135.0; // Degrees

int Gimbal::rotateGimbal(float desired_pitch_angle, float desired_roll_angle, float desired_yaw_angle, Robot_Status* robot_status)
{
	static substates task_state = Init_Task;
	int task_ended = 0;
	const float gimbal_angle_tolerance = 1.0; // Degrees
	switch(task_state)
	{
		case Init_Task:
			pitch_angle = robot_status->gimbal_pitch_angle;
			roll_angle = robot_status->gimbal_roll_angle;
			yaw_angle = robot_status->gimbal_yaw_angle;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			pitch_angle = desired_pitch_angle;
			roll_angle = desired_roll_angle;
			yaw_angle = desired_yaw_angle;
			task_ended = 0;
			if((fabs(desired_pitch_angle-robot_status->gimbal_pitch_angle)<=gimbal_angle_tolerance)&&(fabs(desired_roll_angle-robot_status->gimbal_roll_angle)<=gimbal_angle_tolerance)&&(fabs(desired_yaw_angle-robot_status->gimbal_yaw_angle)<=gimbal_angle_tolerance)) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			pitch_angle = robot_status->gimbal_pitch_angle;
			roll_angle = robot_status->gimbal_roll_angle;
			yaw_angle = robot_status->gimbal_yaw_angle;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}

int Gimbal::hold(Robot_Status* robot_status)
{
	int task_ended = 1;
	pitch_angle = robot_status->gimbal_pitch_angle;
	roll_angle = robot_status->gimbal_roll_angle;
	yaw_angle = robot_status->gimbal_yaw_angle;
	return task_ended;
}

// Scan around for a target or the homing beacon. if homing_beacon = 1, look for homing beacon, else look for object
int Gimbal::scan(int homing_beacon, Robot_Status* robot_status)
{
	static substates task_state = Init_Task;
	static int scan_step = 1;
	int task_ended = 0;
	int target_seen;
	const float gimbal_angle_increment = 45.0; // Degrees
	static float increment_sign = 1.0;
	const float gimbal_angle_tolerance = 2.0; // Degrees
	static float desired_yaw_angle = 0.0; // Degrees
	switch(task_state)
	{
		case Init_Task:
			pitch_angle = robot_status->gimbal_pitch_angle;
			roll_angle = robot_status->gimbal_roll_angle;
			yaw_angle = robot_status->gimbal_yaw_angle;
			task_ended = 0;
			task_state = Exec_Task;
			break;
		case Exec_Task:
			if(homing_beacon==1) target_seen = robot_status->beacon_seen;
			else target_seen = robot_status->object_seen;
			pitch_angle = 0.0;
			roll_angle = 0.0;
			yaw_angle = desired_yaw_angle;
			task_ended = 0;
			if((robot_status->gimbal_yaw_angle>=(gimbal_max_angle-gimbal_angle_tolerance))&&(scan_step==1)) scan_step = 2;
			else if((robot_status->gimbal_yaw_angle<=(gimbal_min_angle+gimbal_angle_tolerance))&&(scan_step==2)) scan_step = 3;
			if((scan_step==1)||(scan_step==3)) increment_sign = 1.0;
			else increment_sign = -1.0;
			if(fabs(desired_yaw_angle-robot_status->gimbal_yaw_angle)<=gimbal_angle_tolerance) desired_yaw_angle = desired_yaw_angle + gimbal_angle_increment*increment_sign;
			else desired_yaw_angle = desired_yaw_angle;
			if((scan_step==3)||(target_seen==1)) task_state = End_Task;
			else task_state = Exec_Task;
			break;
		case End_Task:
			pitch_angle = robot_status->gimbal_pitch_angle;
			roll_angle = robot_status->gimbal_roll_angle;
			yaw_angle = robot_status->gimbal_yaw_angle;
			task_ended = 1;
			task_state = Init_Task;
			break;
	}
	return task_ended;
}