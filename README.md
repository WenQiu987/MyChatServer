# MyChatServer
This project uses C++ to implement a cluster chat server based on the muduo network library. Use nginx that supports tcp to achieve cluster load balancing. Using redis as a message queue enables effective communication between multiple servers.

Dependent libraries and environments:

1. CMake version 3.0 and above
2. Install boost and muduo network development environment
3. MySQL database environment

4. Support nginx1.12.2 and above versions of tcp load balancing

5. Redis, installation command: sudo apt-get install redis-server; and hiredis, which supports C++ for redis client programming, source code download: git clone https://github.com/redis/hiredis



Compile method:

There are already compiled executable files of the server and client in the bin/ directory.

If you compile it yourself, you can execute the following command:

cd build
rm-rf *
cmake..
make
