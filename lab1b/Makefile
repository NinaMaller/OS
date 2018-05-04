
#Nina Maller                                                                    
#Lab 1b                                                                         
#SID: 604955977  
#nina.maller@gmail.com                                                               

.SILENT:

default: client server
client:
	gcc -Wall -Wextra lab1b-client.c -o lab1b-client -lz
server:
	gcc -Wall -Wextra lab1b-server.c -o lab1b-server -lz
clean:
	rm -f lab1b-client lab1b-server *.txt *.o *.gz .*~
dist:
	tar -czf lab1b-604955977.tar.gz lab1b-client.c lab1b-server.c Makefile README
