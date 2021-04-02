im:im.cpp im.hpp mongoose.c
	g++ -g -std=c++11 $^ -o $@ -L/usr/lib64/mysql -lpthread -lmysqlclient -ljsoncpp
