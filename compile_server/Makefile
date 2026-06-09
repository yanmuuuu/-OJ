compile_server : compile_server.cc
	g++ $^ -o $@ -std=c++17 -l jsoncpp -l pthread -l mysqlclient
.PHONY : clean
clean : 
	rm -f compile_server