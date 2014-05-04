#include <iostream>

enum states {Init, Drive2, Scan, Pivot30, DriveN, Search, Pose, Return, Stop, Finished} state;
enum substates {Init_Task, Exec_Task, End_Task} task_state;

bool task_template()
{
	bool task_finished = false;
	
	switch(task_state)
	{
	case Init_Task:
		std::cout << "  Init Task\n";
		task_state =Exec_Task;
		task_finished = false;
		break;
	case Exec_Task:
		std::cout << "  Exec Task\n";
		task_state =End_Task;
		task_finished = false;
		break;
	case End_Task:
		std::cout << "  End Task\n";
		task_state =Init_Task;
		task_finished = true;
		break;
	}
	return task_finished;
}


int main(int argc, char **argv)
{
	state =Init;
	task_state =Init_Task;
	
	while (state != Finished)
	{
		switch(state)
		{
		case Init:
			std::cout << "Init State\n";
			if (task_template()) state = Drive2;
			break;
		case Drive2:
			std::cout << "Drive2 State\n";
			if (task_template()) state = Scan;
			break;
		case Scan:
			std::cout << "Scan State\n";
			if (task_template()) state = Pivot30;
			break;
		case Pivot30:
			std::cout << "Pivot30 State\n";
			if (task_template()) state = DriveN;
			break;
		case DriveN:
			std::cout << "DriveN State\n";
			if (task_template()) state = Search;
			break;
		case Search:
			std::cout << "Search State\n";
			if (task_template()) state = Pose;
			break;
		case Pose:
			std::cout << "Pose State\n";
			if (task_template()) state = Return;
			break;
		case Return:
			std::cout << "Return State\n";
			if (task_template()) state = Stop;
			break;
		case Stop:
			std::cout << "Stop State\n";
			if (task_template()) state = Finished;
			break;
		}
	}
}
