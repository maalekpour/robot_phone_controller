#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <thread>

class MobileBridge : public rclcpp::Node
{
public:
    MobileBridge()
        : Node("mobile_bridge"),
          running_(true),
          current_command_('S'),
          last_command_ms_(nowMs())
    {
        declare_parameter<int>("port", 5000);
        declare_parameter<std::string>("bind_address", "0.0.0.0");
        declare_parameter<double>("linear_speed", 0.8);
        declare_parameter<double>("angular_speed", 1.5);
        declare_parameter<int>("command_timeout_ms", 800);

        port_ = get_parameter("port").as_int();
        bind_address_ = get_parameter("bind_address").as_string();
        linear_speed_ = get_parameter("linear_speed").as_double();
        angular_speed_ = get_parameter("angular_speed").as_double();
        command_timeout_ms_ = get_parameter("command_timeout_ms").as_int();

        cmd_vel_pub_ = create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

        timer_ = create_wall_timer(
            std::chrono::milliseconds(50),
            std::bind(&MobileBridge::publishCommand, this));

        server_thread_ = std::thread(&MobileBridge::serverLoop, this);

        RCLCPP_INFO(get_logger(),
                    "Mobile bridge TCP %s:%d  commands: F B L R S",
                    bind_address_.c_str(), port_);
        RCLCPP_WARN(get_logger(),
                    "Gazebo Sim does NOT read ROS /cmd_vel directly. "
                    "Run a bridge, e.g.:\n"
                    "  ros2 run ros_gz_bridge parameter_bridge "
                    "/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist");
    }

    ~MobileBridge() override
    {
        running_ = false;

        if (server_fd_ >= 0) {
            shutdown(server_fd_, SHUT_RDWR);
            close(server_fd_);
            server_fd_ = -1;
        }

        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

private:
    static int64_t nowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    void publishCommand()
    {
        geometry_msgs::msg::Twist msg;

        char command = current_command_.load();
        const int64_t elapsed = nowMs() - last_command_ms_.load();

        if (elapsed > command_timeout_ms_) {
            command = 'S';
        }

        switch (command) {
            case 'F':
                msg.linear.x = linear_speed_;
                break;
            case 'B':
                msg.linear.x = -linear_speed_;
                break;
            case 'L':
                msg.angular.z = angular_speed_;
                break;
            case 'R':
                msg.angular.z = -angular_speed_;
                break;
            case 'S':
            default:
                break;
        }

        cmd_vel_pub_->publish(msg);
    }

    void setCommand(char command)
    {
        switch (command) {
            case 'F':
            case 'B':
            case 'L':
            case 'R':
            case 'S':
                current_command_.store(command);
                last_command_ms_.store(nowMs());
                RCLCPP_INFO(get_logger(), "Command: %c", command);
                break;
            case '\n':
            case '\r':
            case '\0':
            case ' ':
                break;
            default:
                RCLCPP_WARN(get_logger(), "Unknown command: %d",
                            static_cast<int>(command));
                break;
        }
    }

    void serverLoop()
    {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            RCLCPP_ERROR(get_logger(), "Failed to create TCP socket");
            return;
        }

        int reuse = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(static_cast<uint16_t>(port_));

        if (bind_address_ == "0.0.0.0" || bind_address_ == "*") {
            server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        } else if (inet_pton(AF_INET, bind_address_.c_str(),
                             &server_address.sin_addr) != 1) {
            RCLCPP_ERROR(get_logger(), "Invalid bind_address: %s",
                         bind_address_.c_str());
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        if (bind(server_fd_, reinterpret_cast<sockaddr *>(&server_address),
                 sizeof(server_address)) < 0) {
            RCLCPP_ERROR(get_logger(), "Failed to bind %s:%d: %s",
                         bind_address_.c_str(), port_, std::strerror(errno));
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        if (listen(server_fd_, 1) < 0) {
            RCLCPP_ERROR(get_logger(), "listen() failed: %s", std::strerror(errno));
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        RCLCPP_INFO(get_logger(), "TCP listening on %s:%d",
                    bind_address_.c_str(), port_);

        while (running_) {
            sockaddr_in client_address{};
            socklen_t client_length = sizeof(client_address);

            int client_fd = accept(
                server_fd_,
                reinterpret_cast<sockaddr *>(&client_address),
                &client_length);

            if (client_fd < 0) {
                if (running_) {
                    RCLCPP_ERROR(get_logger(), "accept() failed: %s",
                                 std::strerror(errno));
                }
                continue;
            }

            char client_ip[INET_ADDRSTRLEN] = {0};
            inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
            RCLCPP_INFO(get_logger(), "Android connected from %s", client_ip);

            int nodelay = 1;
            setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

            setCommand('S');

            while (running_) {
                char buffer = 0;
                ssize_t received = recv(client_fd, &buffer, 1, 0);

                if (received > 0) {
                    setCommand(buffer);
                } else {
                    if (received == 0) {
                        RCLCPP_INFO(get_logger(), "Android disconnected");
                    } else {
                        RCLCPP_WARN(get_logger(), "recv() error: %s",
                                    std::strerror(errno));
                    }
                    break;
                }
            }

            close(client_fd);
            setCommand('S');
        }
    }

    int port_;
    std::string bind_address_;
    double linear_speed_;
    double angular_speed_;
    int command_timeout_ms_;
    int server_fd_ = -1;

    std::atomic<bool> running_;
    std::atomic<char> current_command_;
    std::atomic<int64_t> last_command_ms_;

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::thread server_thread_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MobileBridge>());
    rclcpp::shutdown();
    return 0;
}