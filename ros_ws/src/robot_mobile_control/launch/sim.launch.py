from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():

    package_share = get_package_share_directory(
        "robot_mobile_control"
    )

    world_file = os.path.join(
        package_share,
        "description",
        "rover.sdf"
    )

    bridge_config = os.path.join(
        package_share,
        "config",
        "bridge.yaml"
    )

    gazebo = ExecuteProcess(
        cmd=[
            "gz",
            "sim",
            "-r",
            world_file
        ],
        output="screen"
    )

    bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        name="ros_gz_bridge",
        output="screen",
        arguments=[
            "--ros-args",
            "-p",
            f"config_file:={bridge_config}"
        ]
    )

    mobile_bridge = Node(
        package="robot_mobile_control",
        executable="mobile_bridge",
        name="mobile_bridge",
        output="screen",
        parameters=[
            {
                "port": 5000,
                "linear_speed": 0.8,
                "angular_speed": 1.5,
                "command_timeout_ms": 500
            }
        ]
    )

    return LaunchDescription([
        gazebo,
        bridge,
        mobile_bridge
    ])