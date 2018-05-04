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
#include <sys/socket.h> // sockets                                                                        
#include <sys/poll.h>
#include <netdb.h>
#include <sys/stat.h>                                        
#include <string.h> //memset
#include <netinet/in.h>
#include <netdb.h>
#include "zlib.h"

/*                                                                                                                                                                                                          
Nina Maller                                                                                                                                                                                                 
nina.maller@gmail.com                                                                                                                                                                                       
604955977                                                                                                                                                                                                   
*/

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
//pid_t pid;
// reseet the terminal mode to the original                                                                                                                                                                 
void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);

    // give the exit status of shell:                                                                                                                                                                       
  // int status = 0;
    // pid, status, options:                                                                                                                                                                                
    //    waitpid(pid, &status, 0);
    //fprintf(stderr,  "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}

// handler                                                                                                                                                                                                  
// used function from warmup                                                                                                                      
 /*                                                         
void signal_handler(int signum){
  // fprintf(stderr, "caught segmentation fault, signal number is: %d", signum);                                                                                                                            
  //exit(4); // exit with return code of 4                                                                                                                                                                  
  if(signum == SIGPIPE){
    exit(0); // caught signal                                                                                                                                                                               
  }
}
*/

void set_input_mode (void)
{
  // save the new terminal options                                                                                                                                                                          
  struct termios new_terminal_state;
  //  char *name;                            
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
    { "port", required_argument, NULL, 'p'},
    { "log", required_argument, NULL, 'l'},
    { "compress", no_argument, NULL, 'c'},
    { "debug", no_argument, NULL, 'd'},
    {0, 0, 0, 0} // know when end of array is reached                                                                                                                                                       
  };

  // make flags to indicate which options were chosen;                                                                                                                                                      
  // int shell = 0;                                                                                                                                                                                         
  int debug = 0;
  int choice;
  int portno = 0;
  int log_flag = 0;
  char *log_filename;
  int compress_flag = 0;

  while(1){
    choice = getopt_long(argc, argv, "p:l:cd", options, NULL);
    if(choice == -1)
      break;
    switch(choice){
    case 'p':
      //      fprintf(stdout, "s was selsected");                                                                                
      portno = atoi(optarg);
      break;
    case 'd':
      // fprintf(stderr, "d was selected");                                                                                                                                                                 
      debug = 1;
      break;
    case 'c':
      compress_flag = 1;
      break;
    case 'l':
      // log            
        log_filename = optarg; // atoi(optarg);
	log_flag = 1;
	break;
    default:
      fprintf(stderr, "Error in option name"); // add error number       
      exit(1);
      break;
    }
  }

  //set up log
  int logfd = 0;
  if(log_flag == 1){
    logfd = creat(log_filename,S_IRWXU );// 0666); // write to file 
       if(logfd < 0){
	 fprintf(stderr, "failed to create log file desciptor, errno: %d", errno);
	 exit(1);
       }
  }

  // set new mode before getting characters                                                                                                                                                                 
  //  set_input_mode();

  // set up sockets
  // from client side:

  // create a socket with the socket() system call
  int sockfd; // portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "error opening socket");
    exit(1);
  }
  if(debug) fprintf(stderr, " | fd sock is = %d |", sockfd);

  // connect the socket to the address
  server = gethostbyname("localhost");
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(1);
  }
  // note: change bzero to memset, because the tutorial has an old version
  memset((char*) &serv_addr,0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  // change bcopy to memcpy
  memcpy((char *) &serv_addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  // connect
  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting");
    exit(1);
  }

  // send and recieve data

  set_input_mode();

  char newline[2]= {0x0D, 0x0A};
  

      struct pollfd mypoll[2];

      mypoll[0].fd = STDIN_FILENO;
      mypoll[0].events = POLLIN |POLLHUP| POLLERR;
      mypoll[1].fd = sockfd;
      mypoll[1].events = POLLIN | POLLHUP | POLLERR;
     
      /* if(mypoll[1].revents & POLLHUP){
	exit(0);
      }      
	if(mypoll[1].revents & POLLERR){
	    exit(0);
	    }*/
    
      int ret;                                            
      while(1){
        ret = poll(mypoll, 2, 0);
        if(ret < 0){
          fprintf(stderr, "error in poll(), errno: %d", errno);
          exit(1);
        }

        if(mypoll[0].revents & POLLIN){
	  // there is input from STDIN
	  unsigned char buf[256];
	  int bytes_read;
          bytes_read = read(STDIN_FILENO, buf, 256);
	  if(bytes_read == 0) exit(0);
          if(bytes_read < 0){
            fprintf(stderr, "error reading from STDIN, errno: %d", errno);
            exit(1);
          }
	  ret = write(STDOUT_FILENO, buf, bytes_read);
	  if(ret < 0){
	    fprintf(stderr, "error writing to STDOUT from parents process, errno: %d", errno);
	    exit(1);
	  }
	  if(compress_flag == 0){  
	  // write the message to the socket
	  ret = write(sockfd, buf, bytes_read);                                                                       
	  if(ret < 0){
	    fprintf(stderr, "error writing to shell from parents process, errno: %d", errno);                           	    
	    exit(1);
	  } 
	  }

	  // write to log
	  if(log_flag == 1 && compress_flag == 0){
	    ret = write(logfd, "SENT ", 5);
	    char str[4];
	    sprintf(str, "%d", bytes_read);
	    if(bytes_read<10) ret = write(logfd, str, 1);
	    if(bytes_read >9 && bytes_read < 100) ret = write(logfd, str, 2);
	    if(bytes_read>99 && bytes_read < 1000) ret = write(logfd, str, 3);
	    if(bytes_read>999) ret = write(logfd, str, 4);
	    if(ret < 0){
	      fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
	      exit(1);
	    }
	    ret = write(logfd, " bytes: ",8);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	    ret = write(logfd, buf, bytes_read);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	    ret = write(logfd, "\n", 1);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	  }
	  
	  
	  //DELETE
	  // exit(0);

	  // write compressed version to shell
	  if(compress_flag == 1){
	    z_stream toserver;
	    toserver.zalloc = Z_NULL;
	    toserver.zfree = Z_NULL;
	    toserver.opaque = Z_NULL;
	    
	    
	    toserver.avail_in = bytes_read; // number of bytes coming in
	    toserver.next_in = buf; // input buffer
	    toserver.avail_out = 5000;
	    unsigned char compbuf[5000];
	    toserver.next_out = compbuf; 

	   
	     deflateInit(&toserver, Z_DEFAULT_COMPRESSION);
	    ret =  deflate(&toserver, Z_SYNC_FLUSH); // flush as much as possible
	    if(ret < 0){
              fprintf(stderr, "error in deflate() in toserver");
              exit(1);
            } 
	    deflateEnd(&toserver);
	     int bytes_written_tosock = 5000 - toserver.avail_out; 
	    ret = write(sockfd, compbuf, bytes_written_tosock);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes to sockfd, errno: %d", errno);
              exit(1);
            }
	  	    
	    if(log_flag == 1){
	      // write to log bytes written
	       ret = write(logfd, "SENT ", 5);
	      char str[4];
	      sprintf(str, "%d", bytes_written_tosock);
	      if(bytes_written_tosock<10) ret = write(logfd, str, 1);
	      if(bytes_written_tosock >9 && bytes_written_tosock  < 100) ret = write(logfd, str, 2);
	      if(bytes_written_tosock>100) ret = write(logfd, str, 3);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, " bytes: ",8);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, compbuf, bytes_written_tosock);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, "\n", 1);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	    } 
	    
	  }
	  
	  
	  
	  int i;
         
          for(i=0; i<bytes_read; i++){
            char ch = buf[i]; // get one byte at a time                                                                                                                                                     
	    // if(ch == 0x03){
              // cntrl c, then use kill(2) to send SIGINT to the shell process                                                                                                                              
	      // kill(pid, SIGINT); // SHELL will not necessarily die!                                                                          
	       // exit(0);                                                               
	      // }
	    /*  if(ch == '\004' || ch == '\003'){ // cntrl d = EOF       
	      // close only from the terminal so we can still process input from SHELL                                                                                                                      
	        exit(0);
	      
		}*/
            if(ch == 0x0D) { // || ch == 0x0A){
              // char newline_shell = {0x0A};                                                                                                                                                               
	       ret = write(STDOUT_FILENO, &newline, 2);
              if(ret < 0){
                fprintf(stderr, "error writing to STDOUT from parents process, errno: %d", errno);
                exit(1);
		}

            }
           
	  } 
	}
        

        if(mypoll[1].revents & (POLLHUP | POLLERR)){
          //exit(0);
	  ret = close(sockfd);
	  return(0);
        }


        // if there is input from server                                                                                                            
	if(mypoll[1].revents & POLLIN){
	  unsigned char buf2[256];
	  int bytes_read2 = 0;
	  unsigned char compbuf[5000];
	  if(compress_flag == 0) bytes_read2 = read(mypoll[1].fd, buf2, 256);
	  if(compress_flag == 1) bytes_read2 = read(mypoll[1].fd, compbuf, 256); // to be trasferred to buf2 later
	  if(bytes_read2 < 0){
	    fprintf(stderr, "error reading bytes from server, errno: %d", errno);
	    exit(1);
	  }
	  if(bytes_read2 == 0) {
	    close(sockfd);
	    exit(0);
	  }
	  int i;
	  char cha;
	  
	  // check if compressed, and if so decompress
	  if(compress_flag == 1){
	    z_stream fromsrv;
            fromsrv.zalloc = Z_NULL;
            fromsrv.zfree = Z_NULL;
            fromsrv.opaque = Z_NULL;

            fromsrv.avail_in = bytes_read2; // size of input buffer                                                                       
            fromsrv.next_in = compbuf;
            fromsrv.avail_out = 5000; // arbitrary ?                                                                                     
            fromsrv.next_out = buf2;

            inflateInit(&fromsrv);
            ret = inflate(&fromsrv, Z_SYNC_FLUSH); //Z_NO_FLUSH or Z_SYNC_FLUSH                                                    
            if(ret < 0){
              //fprintf(stderr, "error in inflate() on client side");
              //exit(1);
	      // exit(0);
            }
	    inflateEnd(&fromsrv);
	    
            bytes_read2 = 5000 - fromsrv.avail_out;

	    // check if log flag is set, and if so, write to the log
	    if(log_flag == 1){
	      ret = write(logfd, "RECIEVED ", 9);
	      char str[4];
	      sprintf(str, "%d", bytes_read2);
	      if(bytes_read2<10) ret = write(logfd, str, 1);
	      if(bytes_read2 >9 && bytes_read2  < 100) ret = write(logfd, str, 2);
	      if(bytes_read2>100) ret = write(logfd, str, 3);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, " bytes: ",8);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, compbuf, bytes_read2);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }
	      ret = write(logfd, "\n", 1);
	      if(ret < 0){
		fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
		exit(1);
	      }  
	    }
	  }
	  
          // write to log                                                                               
          if(log_flag == 1 && compress_flag ==0){
            ret = write(logfd, "RECIEVED ", 9);
            char str[4];
            sprintf(str, "%d", bytes_read2);
            if(bytes_read2<10) ret = write(logfd, str, 1);
            if(bytes_read2 >9 && bytes_read2 < 100) ret = write(logfd, str, 2);
            if(bytes_read2>100) ret = write(logfd, str, 3);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	    ret = write(logfd, " bytes: ",8);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	    ret = write(logfd, buf2, bytes_read2);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	    ret = write(logfd, "\n", 1);
	    if(ret < 0){
              fprintf(stderr, "error writing bytes number to log fd, errno: %d", errno);
              exit(1);
            }
	  }


	  for(i=0; i<bytes_read2; i++){
	    // we have to analyze byte by byte                                                                                     

	    if(buf2[i] == '\004') exit(0);
	    if(buf2[i] == '\003') exit(0);
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

      if(debug) fprintf(stderr, " | Client is still running! ");
  return EXIT_SUCCESS;
}



