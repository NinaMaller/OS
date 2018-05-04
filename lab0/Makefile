
#Nina Maller
#Lab 0
#SID: 604955977 

.SILENT:

default:
	gcc -Wall -Wextra -g lab0.c -o lab0
clean:
	rm -f lab0 *.txt *.o *.gz
dist:
	tar -czf lab0-604955977.tar.gz lab0.c Makefile README backtrace.png breakpoint.png
check:
	echo "test case 1: correct input and output specified. No output is expected" > test_cases.txt;\
	echo "this is a test" > in.txt; \
	./lab0 --input=in.txt --output=out.txt >> test_cases.txt; \

	echo `\n` >> test_cases.txt; \

	echo "test case 2: correct all 4 arguments. Expecting exit code 4" >> test_cases.txt; \
	./lab0 --input=in.txt --output=out.txt --segfault --catch; \
	if [[ $$? -ne 4 ]]; \
	then \
	echo "Test case 2 failed" >> test_cases.txt; \
	else \
	echo "Test case 2 passed" >> test_cases.txt; \
	fi \

	echo `\n` >> test_cases.txt; \
       
	 echo "test case 3: wrong input file. Expecting exit code 2" >> test_cases.txt; \
        ./lab0 --input=inin.txt --output=out.txt; \
        if [[ $$? -ne 2 ]]; \
        then \
        echo "Test case 3 failed" >> test_cases.txt; \
        else \
        echo "Test case 3 passed" >> test_cases.txt; \
        fi \
	
	
	echo `\n` >> test_cases.txt; \
        echo "test case 4: wrong option name. Expecting exit code 1" >> test_cases.txt; \
        ./lab0 --wrong_input; \
        if [[ $$? -ne 1 ]]; \
        then \
        echo "Test case 4 failed" >> test_cases.txt; \
        else \
        echo "Test case 4 passed" >> test_cases.txt; \
        fi \





