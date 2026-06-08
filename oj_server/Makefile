oj_server : oj_server.cc
	g++ $^ -o $@ -std=c++17 -l pthread -l ctemplate -l jsoncpp -l mysqlclient
.PHONT : clean
clean :
	rm -f oj_server;