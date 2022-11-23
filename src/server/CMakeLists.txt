# 定义了一个SRC_LIST变量，包含了当前目录下所有cpp源文件;定义一个DB_LIST变量包含db文件夹下所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 指定链接时依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread hiredis)



