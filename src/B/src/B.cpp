#include <ros/ros.h>
#include <std_msgs/String.h>
#include <cmath>
#include <string.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <geometry_msgs/PoseStamped.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <serial/serial.h>

enum mode
{
	mission = 0,
	offboard = 1,
	choose = 2,
	bombing = 3,
};
mode mission_mode = bombing;

mavros_msgs::State current_state;
geometry_msgs::PoseStamped local_pos;

void state_sb(const mavros_msgs::State::ConstPtr& msg) {
	current_state = *msg;
	ROS_INFO_STREAM_THROTTLE(1, "\033[1;32m \033[0m" << current_state);
}
void local_pos_sb(const geometry_msgs::PoseStamped::ConstPtr& msg) {
	local_pos = *msg;
	
}

std::string port = "/dev/ttyUSB1"; // 根据实际情况修改串口设备号
int baudrate = 115200; // 根据实际情况修改波特率
serial::Serial serialPort(port, baudrate, serial::Timeout::simpleTimeout(1000));



int main(int argc, char** argv) {
    ros::init(argc, argv, "mission_bombing");
	ros::NodeHandle nh;
    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>("/mavros/state", 10, state_sb);
	ros::Subscriber plane_local_pos_sub = nh.subscribe<geometry_msgs::PoseStamped>("/mavros/local_position/pose", 1, local_pos_sb);
	ros::Publisher target_pos_pub = nh.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_position/local", 10);
	ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>("/mavros/cmd/arming");
	ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>("/mavros/set_mode");
	ros::Rate rate(60.0);
	ros::spinOnce();
	rate.sleep();
	
	geometry_msgs::PoseStamped pose;
	pose.pose.position.x = 138;
	pose.pose.position.y = 145;
	pose.pose.position.z = 20;
    while (ros::ok() && !current_state.connected)
	{
		ROS_INFO("not connected");
		ros::spinOnce();
		rate.sleep();
	}
	for (int i = 100; ros::ok() && i > 0; --i) {
		pose.header.stamp = ros::Time::now();
		target_pos_pub.publish(pose);
		ros::spinOnce();
		rate.sleep();
	}

	mavros_msgs::SetMode offb_set_mode;
	offb_set_mode.request.custom_mode = "AUTO.MISSION";
	mavros_msgs::CommandBool arm_cmd;
	arm_cmd.request.value = true;
	ros::Time last_request = ros::Time::now();

    std_msgs::String msg;
	msg.data = '1';//舵机控制

    int ok=0;
	


    while (ros::ok()) {
        if (current_state.mode == "STABILIZED") {
			ROS_INFO("switch to STABILIZED");
			std::exit(0);
		}
        switch (mission_mode) {
            case 0: 
			if (!current_state.armed) {
				pose.header.stamp = ros::Time::now();
				
				if (arming_client.call(arm_cmd) && arm_cmd.response.success) {
					ROS_INFO("Vehicle armed");
					
				}
				last_request = ros::Time::now();
			}
            else if (current_state.armed&&ok<5) {
				if (current_state.mode != "AUTO.MISSION") {
					pose.header.stamp = ros::Time::now();
					target_pos_pub.publish(pose);
					if (set_mode_client.call(offb_set_mode) && offb_set_mode.response.mode_sent) {
						ROS_INFO("Mission enabled");
						ok++;
					}
				}}
			else if(ok==5){
				mode mission_mode = bombing;
			}
			break;

			case 3:
				
				float distance =sqrt((local_pos.pose.position.x-pose.pose.position.x)*(local_pos.pose.position.x-pose.pose.position.x)+(local_pos.pose.position.y-pose.pose.position.y)*(local_pos.pose.position.y-pose.pose.position.y) );
				ROS_INFO_STREAM_THROTTLE(1, "\033[1;32m \033[0m" << distance);
				
				
				if(distance<10){
					serialPort.write(msg.data);
					std::exit(0);
				}
			break;
}
		ros::spinOnce();
		rate.sleep();
		}}