# CloudMeetingServer

基于C++11编写的服务端，底层使用线程池+IO多路复用的Reactor处理模式，上层基于自定义数据头中消息类型进行处理

## System Requires:

  Linux kernel version >= 4.15.0
  编写环境于Ubuntu 18.04 g++ 7.5.0

## Packages Required:

  `g++ >= 7.5.0`
  `cmake`
  `make`

  **Ubuntu, etc.**
  `sudo apt install g++ cmake make`
  
## To Build

	./build.sh

## To Run

	./CloudMeetingServer
	
## Notice
  默认端口号：1234
