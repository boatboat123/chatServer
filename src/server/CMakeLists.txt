#定义了一个SRC_LIST变量 包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model DB_MODEL_LIST)

#指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${DB_MODEL_LIST})
#指定可执行文件链接时需要依赖的文件
target_link_libraries(ChatServer mymuduo mysqlclient pthread)