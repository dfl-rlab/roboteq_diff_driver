#include <roboteq_motor_controller_driver/querylist.h>
#include <roboteq_motor_controller_driver/roboteq_motor_controller_driver_node.h>
#include <ros/ros.h>
#include <serial/serial.h>
#include <std_msgs/Empty.h>
#include <std_msgs/String.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sstream>
#include <typeinfo>

serial::Serial ser;

using namespace roboteq;

Driver::Driver()
{
    readParams();
}

void Driver::connect()
{
    try
    {
        ser.setPort("/dev/ttyACM0");
        ser.setBaudrate(115200);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    }
    catch (serial::IOException &e)
    {
        ROS_ERROR_STREAM("Unable to open port ");
        ROS_INFO_STREAM("Unable to open port");
    }
    if (ser.isOpen())
    {
        ROS_INFO_STREAM("Serial Port initialized\"");
    }
    else
    {
        ROS_INFO_STREAM("Serial Port is not open");
    }
}
/*
Example:
	!G 1 500 : In Open Loop Speed mode, applies 50% power to motor channel 1
	!G 1 500 : In Closed Loop Speed mode, assuming that 3000 is contained in Max RPM pa-rameter (MXRPM), motor will go to 1500 RPM
	!G 1 500 : In Closed Loop Relative or Closed Loop Tracking modes, the motor will move to 75% position of the total -1000 to +1000 motion range
	!G 1 500 : In Torque mode, assuming that Amps Limit is 60A, motor power will rise until 30A are measured.
*/

void Driver::cmd_vel_callback(const geometry_msgs::Twist &msg)
{
    cmd_vr = msg.linear.x + msg.angular.z * wheel_axle_length / 2.0;
    cmd_vl = msg.linear.x - msg.angular.z * wheel_axle_length / 2.0;

    cmd_rpm_r = cmd_vr / double(wheel_circumference) * 60.0 / max_rpm * 1000.0;
    cmd_rpm_l = cmd_vl / double(wheel_circumference) * 60.0 / max_rpm * 1000.0;

    std::stringstream cmd_sub;
    cmd_sub << "!G 1"
            << " " << round(cmd_rpm_r) << "_"
            << "!G 2"
            << " " << -1 * round(cmd_rpm_l) << "_";
    ser.write(cmd_sub.str());
    ser.flush();
    ROS_INFO_STREAM(cmd_sub.str());
}

void Driver::roboteq_subscriber()
{
    ros::NodeHandle n;
    cmd_vel_sub = n.subscribe("/cmd_vel", 10, &Driver::cmd_vel_callback, this);
}

bool Driver::configservice(roboteq_motor_controller_driver::config_srv::Request &request, roboteq_motor_controller_driver::config_srv::Response &response)
{
    std::stringstream str;
    str << "^" << request.userInput << " " << request.channel << " " << request.value << "_ "
        << "%\clsav321654987";
    ser.write(str.str());
    ser.flush();
    response.result = str.str();

    ROS_INFO_STREAM(response.result);
    return true;
}

bool Driver::commandservice(roboteq_motor_controller_driver::command_srv::Request &request, roboteq_motor_controller_driver::command_srv::Response &response)
{
    std::stringstream str;
    str << "!" << request.userInput << " " << request.channel << " " << request.value << "_";
    ser.write(str.str());
    ser.flush();
    response.result = str.str();

    ROS_INFO_STREAM(response.result);
    return true;
}

bool Driver::maintenanceservice(roboteq_motor_controller_driver::maintenance_srv::Request & request,
                                roboteq_motor_controller_driver::maintenance_srv::Response &response)
{
    std::stringstream str;
    str << "%" << request.userInput << " "
        << "_";
    ser.write(str.str());
    ser.flush();
    response.result = ser.read(ser.available());

    ROS_INFO_STREAM(response.result);
    return true;
}

void Driver::roboteq_services()
{
    ros::NodeHandle n;
    configsrv      = n.advertiseService("config_service", &Driver::configservice, this);
    commandsrv     = n.advertiseService("command_service", &Driver::commandservice, this);
    maintenancesrv = n.advertiseService("maintenance_service", &Driver::maintenanceservice, this);
}

void Driver::readParams()
{
    nh.param("robot/pub_odom", pub_odom, true);
    ROS_INFO_STREAM("pub_odom: " << pub_odom);
    nh.param("drive/open_loop", open_loop, false);
    ROS_INFO_STREAM("open_loop: " << open_loop);

    nh.param<std::string>("drive/serial_port", serial_port, "/dev/ttyACM0");
    ROS_INFO_STREAM("serial_port: " << serial_port);
    nh.param<std::string>("drive/odom_frame", odom_frame, "odom");
    ROS_INFO_STREAM("odom_frame: " << odom_frame);
    nh.param<std::string>("drive/base_frame", base_frame, "base_link");
    ROS_INFO_STREAM("base_frame: " << base_frame);
    nh.param<std::string>("drive/cmd_vel_topic", cmd_vel_topic, "cmd_vel");
    ROS_INFO_STREAM("cmd_vel_topic: " << cmd_vel_topic);

    nh.param("robot/wheel_axle_length", wheel_axle_length, 0.395);
    ROS_INFO_STREAM("wheel_axle_length: " << wheel_axle_length);
    nh.param("robot/wheel_radius", wheel_radius, 0.155);
    ROS_INFO_STREAM("wheel_radius: " << wheel_radius);

    nh.param("dive/baud_rate", baud_rate, 115200);
    ROS_INFO_STREAM("baud_rate: " << baud_rate);
    nh.param("dive/encoder_ppr", encoder_ppr, 900);
    ROS_INFO_STREAM("encoder_ppr: " << encoder_ppr);
    nh.param("dive/encoder_cpr", encoder_cpr, 3600);
    ROS_INFO_STREAM("encoder_cpr: " << encoder_cpr);
    nh.param("dive/max_amp", max_amp, 100);
    ROS_INFO_STREAM("max_amp: " << max_amp);
    nh.param("dive/max_rpm", max_rpm, 3000);
    ROS_INFO_STREAM("max_rpm: " << max_rpm);

    nh.getParam("frequencyH", frequencyH);
    nh.getParam("frequencyL", frequencyL);
    nh.getParam("frequencyG", frequencyG);

    wheel_circumference = 2.0 * M_PI * wheel_radius;
}

void Driver::run()
{
    typedef std::string Key;
    typedef std::string Val;
    std::map<Key, Val>  map_sH;
    nh.getParam("queryH", map_sH);

    std::stringstream        ss0;
    std::stringstream        ss1;
    std::stringstream        ss2;
    std::stringstream        ss3;
    std::vector<std::string> KH_vector;

    ss0 << "^echof 1_";
    ss1 << "# c_/\"DH?\",\"?\"";
    for (std::map<Key, Val>::iterator iter = map_sH.begin(); iter != map_sH.end(); ++iter)
    {
        Key KH = iter->first;
        KH_vector.push_back(KH);
        Val VH = iter->second;
        ss1 << VH << "_";
    }
    ss1 << "# " << frequencyH << "_";

    std::vector<ros::Publisher> publisherVecH;
    for (int i = 0; i < KH_vector.size(); i++)
    {
        publisherVecH.push_back(nh.advertise<roboteq_motor_controller_driver::channel_values>(KH_vector[i], 100));
    }

    std::map<Key, Val> map_sL;
    nh.getParam("queryL", map_sL);
    std::vector<std::string> KL_vector;
    ss2 << "/\"DL?\",\"?\"";
    for (std::map<Key, Val>::iterator iter = map_sL.begin(); iter != map_sL.end(); ++iter)
    {
        Key KL = iter->first;
        KL_vector.push_back(KL);
        Val VL = iter->second;
        ss2 << VL << "_";
    }
    ss2 << "# " << frequencyL << "_";

    std::vector<ros::Publisher> publisherVecL;
    for (int i = 0; i < KL_vector.size(); ++i)
    {
        publisherVecL.push_back(nh.advertise<roboteq_motor_controller_driver::channel_values>(KL_vector[i], 100));
    }

    std::map<Key, Val> map_sG;
    nh.getParam("queryG", map_sG);
    std::vector<std::string> KG_vector;
    ss3 << "/\"DG?\",\"?\"";
    for (std::map<Key, Val>::iterator iter = map_sG.begin(); iter != map_sG.end(); ++iter)
    {
        Key KG = iter->first;
        KG_vector.push_back(KG);
        Val VG = iter->second;
        ss3 << VG << "_";
    }
    ss3 << "# " << frequencyG << "_";

    std::vector<ros::Publisher> publisherVecG;
    for (int i = 0; i < KG_vector.size(); ++i)
    {
        publisherVecG.push_back(nh.advertise<std_msgs::String>(KG_vector[i], 100));
    }

    ser.write(ss0.str());
    ser.write(ss1.str());
    ser.write(ss2.str());
    ser.write(ss3.str());
    ser.flush();

    read_publisher = nh.advertise<std_msgs::String>("read", 1000);

    ros::Rate loop_rate(50);
    while (ros::ok())
    {
        ros::spinOnce();
        if (ser.available())
        {
            //ROS_INFO_STREAM("Reading from serial port");
            std_msgs::String result;
            result.data = ser.read(ser.available());

            read_publisher.publish(result);
            boost::replace_all(result.data, "\r", "");
            boost::replace_all(result.data, "+", "");
            std::vector<std::string> fields;
            /*
            std::vector<std::string> Field9;
            boost::split(fields, result.data, boost::algorithm::is_any_of("D"));

            std::vector<std::string> fields_H;
            boost::split(fields_H, fields[1], boost::algorithm::is_any_of("?"));
            //	    std::cout << fields_H[1] << std::endl;
            if (fields_H[0] == "H")
            {
                for (int i = 0; i < publisherVecH.size(); ++i)
                {
                    std::vector<std::string> sub_fields_H;
                    boost::split(sub_fields_H, fields_H[i + 1], boost::algorithm::is_any_of(":"));
                    roboteq_motor_controller_driver::channel_values Q1;

                    Q1.value.push_back(0);
                    for (int j = 0; j < sub_fields_H.size(); j++)
                    {
                        Q1.value.push_back(boost::lexical_cast<int>(sub_fields_H[j]));
                    }
                    publisherVecH[i].publish(Q1);
                }
            }
            else
            {
                ROS_INFO_STREAM("Garbage data on Serial");
            }

            std::vector<std::string> fields_L;
            std::vector<std::string> fields_G;
            boost::split(fields_G, fields[3], boost::algorithm::is_any_of("?"));
            boost::split(fields_L, fields[2], boost::algorithm::is_any_of("?"));

            //if (fields_L[0] == "L" || fields_G[0] == "L" )
            if (fields_L[0] == "L")
            {
                for (int i = 0; i < publisherVecL.size(); ++i)
                {
                    std::vector<std::string> sub_fields_L;
                    boost::split(sub_fields_L, fields_L[i + 1], boost::algorithm::is_any_of(":"));
                    roboteq_motor_controller_driver::channel_values Q1;
                    Q1.value.push_back(0);
                    for (int j = 0; j < sub_fields_L.size(); j++)
                    {
                        Q1.value.push_back(boost::lexical_cast<int>(sub_fields_L[j]));
                    }
                    publisherVecL[i].publish(Q1);
                }
            }

            if (fields_G[0] == "G")
            {
                for (int i = 0; i < publisherVecG.size(); ++i)
                {
                    std_msgs::String Q1;
                    Q1.data = fields_G[i + 1];
                    publisherVecG[i].publish(Q1);
                }
            }
            */
            // ROS_INFO_STREAM("success!");
            Driver::roboteq_subscriber();
        }
        loop_rate.sleep();
        //ROS_INFO_STREAM("Type the command - \"rostopic list\" - in new terminal for publishers");
    }
    if (ser.isOpen())
        ser.close();

    ROS_INFO("Exiting");
}
