/*
 * Nina Maller
 * Lab 0
 * 604955977 */

#include <stdlib.h>
#include <getopt.h> // for getopt_long
#include <errno.h> 
#include <unistd.h> // read
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>


//subroutine that sets a char * pointer to NULL and
//stores through the null pointer
void force_segmentation_fault(){
	char* nullptr = NULL;
	char ch = *nullptr; // assign any value to the null pointer
	if(ch == ' '){
		// do nothing, supress warning about unused variable
	}

}

// handler
// used example from code from discussion
void signal_handler(int signum){
	fprintf(stderr, "caught segmentation fault, signal number is: %d", signum);
	exit(4); // exit with return code of 4
}

int main(int argc, char* argv[])
{
	if(argc == 1){
		fprintf(stderr, "no options were specified, indicate input and output files");
		exit(1);
	}
	// get options using getopt_long
	struct option options[] = {
		{ "input", required_argument, NULL, 'i'},
		{ "output", required_argument, NULL, 'o'},
		{ "segfault", no_argument, NULL, 's'},
		{ "catch", no_argument, NULL, 'c'},
		{0, 0, 0, 0} // know when end of array is reached
	};
	

        // define parameters that are used in the switch statement
        char* inputFile = NULL;
        char* outputFile = NULL;
	int fd0, fd1; // an int - 0:standard input, 1: standard output, 2: stadrard error
	int c = 0; 
	int seg_fault_flag = 0;
                         
	while(1) {
	// get next options:
	c = getopt_long (argc, argv, "i:o:sc", options, NULL);

        // detect the end of the options
        if( c == -1)
       		break;

	// get parameters 
	switch(c){
		case 'i': 
			inputFile = optarg;
			break;
		case 'o': 
			outputFile = optarg;
			break;
		case 's': // do immidiately and exit
			// used example code from discussion section
			//char nullPointer = NULL;
			//force_segmentation_fault();
			seg_fault_flag = 1;
			break;
		case 'c':
			signal(SIGSEGV, signal_handler);
			break;
		default: // an error occured
			fprintf(stderr, "Error in option name. Cannot recognize option name");
			exit(1);  
			break;
	}
	}
	//first check if to make seg fault:
	if(seg_fault_flag == 1)
		force_segmentation_fault();

	//else, continue

	// handle input:
	// used example from web cs ucla
	 fd0 = open(inputFile, O_RDONLY);
	////////////////fprintf(stderr, "this: %c", &inputFile);
	 // if unable to open the file, 
	 // report the failure on stderr
	 if(fd0 == -1){
		 fprintf(stderr, "error opening input file, error number: %d", errno);
		 exit(2);
	 }// else, open was successful
	close(0);
	dup(fd0);
	close(fd0);

	// handle output
	// used example from web cs ucla
	fd1 = creat(outputFile, 0666);
	if(fd1 == -1){
		fprintf(stderr, "error in creating ouput file, error number: %d", errno);
		exit(3); 
	}
	close(1);
	dup(fd1);
	close(fd1);
	
	// copy from input file to output file:
	// ssize_t read(int fd, void *buf, size_t count);
	// read return 0 when EOF
	// on success, the number of bytes read is returned
	int total_bytes_read = 0; // keep track of how many bytes where read
	int total_bytes_written = 0;
	ssize_t ret = 1; // dummy value
	char *buffer[1024]; // read 1024 bytes at a time
	while(ret != 0){ // while there are more bytes to read
		ret = read(0, &buffer, 1024);
		if(ret == -1){ // error reading
			fprintf(stderr, "error in reading from buffer, error number: %d", errno);
			exit(2);
		}
		total_bytes_read += ret; 
		int ret_w = write(1, &buffer, ret); // copy number of bytes read: ret
		if(ret_w == -1){ // error
			fprintf(stderr, "error in writing to output file, error number: %d", errno);
			exit(2);
		}
		total_bytes_written += ret_w;
	}
	// check that number of bytes read and written is the same
	if(total_bytes_read != total_bytes_written){
		fprintf(stderr, "total number of bytes read and written is not the same");
		exit(5);
	}
	
	//done, exit successfuly
	exit(0);
	
}

















