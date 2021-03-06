#ifndef ROBOTEQ_MOTOR_CONTROLLER_DRIVER_MAIN_H
#define ROBOTEQ_MOTOR_CONTROLLER_DRIVER_MAIN_H

#include <ros/ros.h>
#include <tf/tf.h>

//! ROS standard msgs
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <nav_msgs/Odometry.h>
#include <roboteq_motor_controller_driver/channel_values.h>
#include <roboteq_motor_controller_driver/command_srv.h>
#include <roboteq_motor_controller_driver/config_srv.h>
#include <roboteq_motor_controller_driver/maintenance_srv.h>
#include <roboteq_motor_controller_driver/querylist.h>
#include <sensor_msgs/BatteryState.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/Joy.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/TimeReference.h>
#include <serial/serial.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Int16.h>
#include <std_msgs/String.h>
#include <std_msgs/UInt8.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>

#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

/*
#define WHEEL_AXLE_LEN 0.395
#define WHEEL_RADIUS 0.155
#define WHEEL_CIRCUMFERENCE 2.0 * M_PI* WHEEL_RADIUS
#define MAX_RPM 3000
*/

namespace roboteq
{
class Driver
{
  public:
    ros::Subscriber    cmd_vel_sub;
    ros::Publisher     read_publisher;
    ros::ServiceServer configsrv;
    ros::ServiceServer commandsrv;
    ros::ServiceServer maintenancesrv;

    Driver();
    void connect();
    void readParams();
    void run();
    void roboteq_subscriber();
    void roboteq_publisher();
    void cmd_vel_callback(const geometry_msgs::Twist& msg);

    int channel_number_1;
    int channel_number_2;
    int frequencyH;
    int frequencyL;
    int frequencyG;

    double cmd_vr;
    double cmd_vl;
    double cmd_rpm_r;
    double cmd_rpm_l;

    void roboteq_services();
    bool configservice(roboteq_motor_controller_driver::config_srv::Request& req, roboteq_motor_controller_driver::config_srv::Response& res);
    bool commandservice(roboteq_motor_controller_driver::command_srv::Request& req, roboteq_motor_controller_driver::command_srv::Response& res);
    bool maintenanceservice(roboteq_motor_controller_driver::maintenance_srv::Request& req, roboteq_motor_controller_driver::maintenance_srv::Response& res);

  private:
    ros::NodeHandle nh;

    double wheel_axle_length;
    double wheel_radius;
    double wheel_circumference;

    bool pub_odom;
    bool open_loop;

    std::string odom_frame;
    std::string base_frame;
    std::string cmd_vel_topic;
    std::string serial_port;
    std::string firmware;

    int baud_rate;
    int encoder_ppr;
    int encoder_cpr;
    int max_amp;
    int max_rpm;
    int channel;

    geometry_msgs::TransformStamped tf_msg;
    tf::TransformBroadcaster        odom_broadcaster;
    nav_msgs::Odometry              odom_msg;
    nav_msgs::Odometry              odom;

    enum fault_flag
    {
        NO_FAULT             = 0,
        OVERHEAT             = 1,
        OVERVOLTAGE          = 2,
        UNDERVOLTAGE         = 4,
        SHORT_CIRCUIT        = 8,
        EMERGENCY_STOP       = 16,
        SETUP_FAULT          = 32,
        MOSFET_FAILURE       = 64,
        STARTUP_CONFIG_FAULT = 128,
    };
};
}  // namespace roboteq
#endif  // ROBOTEQ_MOTOR_CONTROLLER_DRIVER_MAIN_H
