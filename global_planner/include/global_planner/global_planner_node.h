#ifndef GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H
#define GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H

#include <math.h>
#include <stdio.h>
#include <boost/bind.hpp>
#include <mutex>
#include <set>
#include <string>
#include "rclcpp/rclcpp.hpp"

#include <px4_msgs/msg/vehicle_local_position.hpp>
#include <px4_msgs/msg/vehicle_attitude.hpp>
#include <px4_msgs/msg/vehicle_trajectory_waypoint.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/common/transforms.h>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <std_msgs/msg/color_rgba.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/create_timer_ros.h>
#include <tf2_ros/buffer.h>
#include <tf2/convert.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <octomap/octomap.h>
#include <octomap_msgs/msg/octomap.h>
#include <octomap_msgs/conversions.h>

#include "avoidance/avoidance_node.h"
#include "global_planner/global_planner.h"

#ifndef DISABLE_SIMULATION
#include <avoidance/rviz_world_loader.h>
#endif

namespace global_planner {

using std::placeholders::_1;
using namespace std::chrono_literals;

struct cameraData {
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_sub_;
};

class GlobalPlannerNode  : public rclcpp::Node {
 public:
  // TODO: Deque instead of vector
  GlobalPlanner global_planner_;
  std::vector<GoalCell> waypoints_;  // Intermediate goals, from file, mavros
                                     // mission or intermediate goals
  GlobalPlannerNode();
  ~GlobalPlannerNode();

 private:
  std::mutex mutex_;

  // Subscribers
  // ros::Subscriber octomap_sub_;
  rclcpp::Subscription<octomap_msgs::msg::Octomap>::SharedPtr octomap_full_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr position_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleAttitude>::SharedPtr attitude_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr clicked_point_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr move_base_simple_sub_;
  
  // Publishers
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr global_temp_path_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr smooth_path_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr actual_path_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr explored_cells_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PointStamped>::SharedPtr global_goal_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PointStamped>::SharedPtr global_temp_goal_pub_;
  rclcpp::Publisher<px4_msgs::msg::VehicleTrajectoryWaypoint>::SharedPtr mavros_obstacle_free_path_pub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr current_waypoint_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;

  rclcpp::Time start_time_;
  rclcpp::Time last_wp_time_;

  rclcpp::TimerBase::SharedPtr gp_cmdloop_timer_;
  rclcpp::TimerBase::SharedPtr gp_plannerloop_timer_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;  
  std::shared_ptr<tf2_ros::TransformBroadcaster> transform_broadcaster_;
  
  nav_msgs::msg::Path actual_path_;
  geometry_msgs::msg::Point start_pos_;
  geometry_msgs::msg::PoseStamped current_goal_;
  geometry_msgs::msg::PoseStamped last_goal_;
  geometry_msgs::msg::PoseStamped last_pos_;
  sensor_msgs::msg::PointCloud2 pointcloud2_;
  
  std::vector<geometry_msgs::msg::PoseStamped> last_clicked_points;
  std::vector<geometry_msgs::msg::PoseStamped> path_;
  std::vector<cameraData> cameras_;

  int num_octomap_msg_ = 0;
  int num_pos_msg_ = 0;
  std::chrono::milliseconds gp_cmdloop_dt_;
  std::chrono::milliseconds gp_plannerloop_dt_;
  double mapupdate_dt_;
  double min_speed_;
  double speed_ = min_speed_;
  double start_yaw_;
  bool position_received_;
  std::string frame_id_;
  std::string camera_frame_id_;

  double clicked_goal_alt_;
  double clicked_goal_radius_;
  bool hover_;
  int simplify_iterations_;
  double simplify_margin_;

  avoidance::AvoidanceNode avoidance_node_;
// #ifndef DISABLE_SIMULATION
  // std::unique_ptr<avoidance::WorldVisualizer> world_visualizer_;
  avoidance::WorldVisualizer::SharedPtr world_visualizer_;
  rclcpp::executors::MultiThreadedExecutor world_visualizer_executor_;
  rclcpp::TimerBase::SharedPtr world_visualizer_timer_;
  std::thread* world_visualizer_thread;
// #endif
  void readParams();
  void initializeCameraSubscribers(std::vector<std::string>& camera_topics);
  void setNewGoal(const GoalCell& goal);
  void popNextGoal();
  void planPath();
  void setIntermediateGoal();
  bool isCloseToGoal();
  void setCurrentPath(const std::vector<geometry_msgs::msg::PoseStamped>& poses);
  void positionCallback(const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg);
  void clickedPointCallback(const geometry_msgs::msg::PointStamped::SharedPtr msg);
  void moveBaseSimpleCallback(const geometry_msgs::msg::PoseStamped::SharedPtr msg);
  void octomapFullCallback(const octomap_msgs::msg::Octomap::SharedPtr msg);
  void depthCameraCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg);
  void tf2Callback(const std::shared_future<geometry_msgs::msg::TransformStamped>& tf);
  void attitudeCallback(const px4_msgs::msg::VehicleAttitude::SharedPtr msg);
  // void fcuInputGoalCallback(const mavros_msgs::Trajectory& msg);
  void cmdLoopCallback();
  void plannerLoopCallback();
  void publishGoal(const GoalCell& goal);
  void publishPath();
  void publishSetpoint();
  void printPointInfo(double x, double y, double z);
};

}  // namespace global_planner

#endif  // GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H
