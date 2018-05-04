#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h> // error number
#include <getopt.h> // getopt_long
#include <sys/types.h> // fork, waitpid
#include <unistd.h> // fork, exec, pipe, dup, read2
#include <fcntl.h> /* Obtain O_* constant definitions */
#include <poll.h> // use pollfd
#include <signal.h> // also pollfd but Im not sure
#include <sys/wait.h> // waitpid

/*
Nina Maller
nina.maller@gmail.com
604955977
*/

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
int shell = 0; 
//we have to check pid before exiting
// so it must be global:
pid_t pid;


// reseet the terminal mode to the original
void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
  if(shell == 1){
    // give the exit status of shell:                                                                              
    int status = 0;
    // pid, status, options:
    waitpid(pid, &status, 0);
    fprintf(stderr,  "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
  }

}

// handler                                                                                           
// used function from warmup                                                            
void signal_handler(int signum){
  // fprintf(stderr, "caught segmentation fault, signal number is: %d", signum);
  //exit(4); // exit with return code of 4                                                       
  if(signum == SIGPIPE){
    exit(0); // caught signal
  }
}


void set_input_mode (void)
{
  // save the new terminal options
  struct termios new_terminal_state;
  char *name;

  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (EXIT_FAILURE);
    }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);

  /* Set the funny terminal modes. */
   tcgetattr (STDIN_FILENO, &new_terminal_state);
   new_terminal_state.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
   new_terminal_state.c_cc[VMIN] = 1;
   new_terminal_state.c_cc[VTIME] = 0; 

  new_terminal_state.c_iflag = ISTRIP; /* only lower 7 bits*/
  new_terminal_state.c_oflag = 0;/* no processing*/
  new_terminal_state.c_lflag = 0;/* no processing*/
  tcsetattr (STDIN_FILENO, TCSANOW, &new_terminal_state); // change from TCSAFLUCH to NOW
}



int main (int argc, char* argv[])
{
  // get options using getopt_long                                                                         
  struct option options[] = {
    { "shell", no_argument, NULL, 's'},
    { "debug", no_argument, NULL, 'd'},
    {0, 0, 0, 0} // know when end of array is reached                                           
  };

  // make flags to indicate which options were chosen;
  // int shell = 0;
  int debug = 0;
  int choice;

  while(1){
    choice = getopt_long(argc, argv, "sd", options, NULL);
    if(choice == -1)
      break;
    switch(choice){
    case 's':
      //      fprintf(stdout, "s was selsected");
      shell = 1;
      break;
    case 'd':
      // fprintf(stderr, "d was selected");
      debug = 1;
      break;
    default:
      fprintf(stderr, "Error in option name"); // add error number
      exit(1);
      break;
    }
  }

  char c;

  // set new mode before getting characters 
  set_input_mode();
  char d = ' ';
  char newline[2]= {0x0D, 0x0A};
  if(shell == 0) {
  while (1)
    {
      read(STDIN_FILENO, &c, 1);
      if (c == 0x04) // ^D
	break;
      if( c == 0x0D || c == 0x0A)
	write(STDOUT_FILENO, newline, 2);
     else {
	write(STDOUT_FILENO, &c, 1);
        }
   }
  } // close if shell == 0

  // if the shell option was specified:
  if(shell == 1){
    // make two pipes:
    // takes in an array of two integers
    // [0] is set on reading and [1] is set on writing
    int child_to_parent[2];
    int parent_to_child[2];
    if(pipe(child_to_parent) < 0){
      fprintf(stderr, "error creating child_to_parent pipe, errno: %d", errno);
      return(1);
    }
    if(pipe(parent_to_child) < 0){
      fprintf(stderr, "error creating parent_to_child pipe, errno: %d", errno);
      return(1);
    }

    // fork to create a new process
   
    int i;
     pid = fork();
    // pid = getpid(); // get the process if
    if(pid < 0){
      fprintf(stderr, "error calling fork(), errno: %d", errno);
      return(1);
    }

    else if(pid == 0){ // child process
      // in child process, we want to close all the pipes we do not need
      if(debug == 1)
	fprintf(stderr, "Started child process");
      int ret = close(parent_to_child[1]); // writing
      if( ret < 0){
	fprintf(stderr, "failed to close pipe parent_to_child[1], errno: %d", errno);
	exit(1);
      }
      ret = close(child_to_parent[0]); // reading
      if( ret < 0){
        fprintf(stderr, "failed to close pipe parent_to_child[1], errno: %n", errno);
	exit(1);
      }
      
      // use dup2 to duplicate the file descriptors 
      // child will be shell so we need stdin from terminal
      // and stdout and stderr go back 
      ret = dup2(child_to_parent[1], STDOUT_FILENO);
      if(ret < 0){
	fprintf(stderr, "failed to dup2 child_to_parent[1] to STDOUT. errno: %d", errno);
	exit(1);
      }
      ret = dup2(child_to_parent[1], STDERR_FILENO);
      if(ret < 0){
	fprintf(stderr,"failed to dup2 child_to_parent[1] to STDERR. errno: %d", errno);
	exit(1);
      } 
      ret = dup2(parent_to_child[0], STDIN_FILENO);
      if(ret < 0){
	fprintf(stderr,"failed to dup2 parent_to_child[0] to STDIN. errno: %d", errno);
	exit(1);
      }
     
      // after using dup2, we can now close the pipes:
      ret = close(child_to_parent[1]);                                                   
      if( ret < 0){
        fprintf(stderr, "failed to close pipe child_to_parent[1], errno: %d", errno);
        exit(1);
      }
      ret = close(parent_to_child[0]);                                                    
      //      fprintf(stderr, " || %n ||", ret);
      //ret = -1;
      if( ret < 0){
        fprintf(stderr, "failed to close pipe parent_to_child[0], errno: %d", errno);
        exit(1);
      }
      //      if(debug == 1)
      //  fprintf(stderr, "| child process - last close: %n |", ret);      
      if(debug == 1)
	fprintf(stderr, "| child process - trying to exec shell |");
      
      //finally, exec shell bash
      ret = execvp("/bin/bash", NULL);
      if( ret<0){
	fprintf(stderr, "failed to exec to shell bash, errno: %n", errno);
	exit(1);
      }

      if(debug == 1)
	fprintf(stderr, " finished child process ");


    } // close child

    else if(pid > 0){ // parent process!
      if(debug == 1)
	fprintf(stderr, "started parent proces ");
      // when using shell, there can be 2 
      // close the 2 sides we are not going to use
      int ret = close(child_to_parent[1]);
      if( ret < 0){
        fprintf(stderr, "failed to close pipe child_to_parent[1], errno: %d", errno);
        exit(1);
      }
      ret = close(parent_to_child[0]);
      if( ret < 0){
        fprintf(stderr, "failed to close pipe parent_to_child[0], errno: %d", errno);
        exit(1);
      }
      
      // poll waits for one of a set of fds to become ready to perform I/O
      // it is controlled by pollfd (int fd, short events, short revents)
      // create an array of two pollfd structures
      struct pollfd mypoll[2];
      // first describes the keyboard stdin
      mypoll[0].fd = STDIN_FILENO;
      // POLLIN = there is data to be read
      // POLLHUP = closed connection
      // POLLERR = error condition
      mypoll[0].events = POLLIN |POLLHUP| POLLERR;
      //next one describes the pipe that return from the shell
      mypoll[1].fd = child_to_parent[0];
      mypoll[1].events = POLLIN | POLLHUP | POLLERR;
      // revents parameter will be filled by the kernel, indicating that the
      // events have happened 
      //write a main loop that call poll2 and only then read from a fd 
      // if it has pending input
      // intret;
      while(1){
	ret = poll(mypoll, 2, 0);
	if(ret < 0){
	  fprintf(stderr, "error in poll(), errno: %d", errno);
	  exit(1);
	}
	// there is data to read from the keyboard
	if(mypoll[0].revents & POLLIN){
	  char buf[256]; // value from specs
	  char newline_shell = {0x0A};
	  int bytes_read = 0;
	  bytes_read = read(STDIN_FILENO, buf, 256);
	  if(bytes_read < 0){
	    fprintf(stderr, "error reading from STDIN, errno: %d", errno);
	    exit(1);
	  }
	  // we want to analyze one byte at a time to see if 
	  // some special character was put in
	  int i;
	  if(debug == 1)
	    {// fprintf(stderr, " || got herer || ");
	      //	      return(1);
	    }
	  for(i=0; i<bytes_read; i++){
	    char ch = buf[i]; // get one byte at a time
	    if(ch == 0x03){
	      // cntrl c, then use kill(2) to send SIGINT to the shell process
	      kill(pid, SIGINT); // SHELL will not necessarily die!
	    }
	    if(ch == 0x04){ // cntrl d = EOF
	      // close only from the terminal so we can still process input from SHELL
	     ret =  close(parent_to_child[1]);
	     if(ret < 0){
	       fprintf(stderr, "error closing parent_to_child[0] from parents process, errno: %d", errno);
	       exit(1);
	    }
	     //exit(0);
	    }
	    else if(ch == 0x0D || ch == 0x0A){
	      // char newline_shell = {0x0A};
	      ret = write(STDOUT_FILENO, &newline, 2);
	      if(ret < 0){
		fprintf(stderr, "error writing to STDOUT from parents process, errno: %d", errno);
		exit(1);
	      }
	      ret = write(parent_to_child[1], &newline_shell, 1);
	      if(ret < 0){
		fprintf(stderr, "error writing to shell from parents process, errno: %d", errno);
		exit(1);
	      }
	    }
	    else {
	    ret = write(STDOUT_FILENO, &ch, 1);
	    if(ret < 0){
	      fprintf(stderr, "error writing to STDOUT from parents process, errno: %d", errno);
	      exit(1);
            }
	    ret = write(parent_to_child[1], &ch, 1);
	    if(ret < 0){
	      fprintf(stderr, "error writing to shell from parents process, errno: %d", errno);
	      exit(1);
	    }
	    }
	    }
	} 
	if(mypoll[1].revents & POLLHUP+POLLERR){
	  ret = close(child_to_parent[0]);
	  if(ret < 0){
	    fprintf(stderr, "error closing child_to_parent[0] from parents process, errno: %d", errno);
	    exit(1);
	  }
	  exit(0);
	}
	

	// if there is input from shell 
	  if(mypoll[1].revents & POLLIN){
	    char buf2[256];
	    int bytes_read2 = 0;
	    bytes_read2 = read(mypoll[1].fd, buf2, 256);
	    int i;
	    char cha;
	    for(i=0; i<bytes_read2; i++){
	      // we have to analyze byte by byte
	      cha = buf2[i];
	      if(cha == 0x0D || cha == 0x0A){
		ret = write(STDOUT_FILENO, newline, 2);
                 if(ret < 0){
		   fprintf(stderr, "error writing to STDOUT from shell from parents process, errno: %d", errno);
		   exit(1);
              }
	      }
	      else
		ret = write(STDOUT_FILENO, &cha, 1);
	      if(ret < 0){
		fprintf(stderr, "error writing to STDOUT, &cha, from parents process, errno: %d", errno);
		exit(1);
	      }
	    }
	  }
	
        
      } // close while, main loop


    } // close parent
  
  } // close shell == 1



  return EXIT_SUCCESS;
}

