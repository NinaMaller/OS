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
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include "zlib.h"                                           

/*                                                                                                                                                                                                          
Nina Maller                                                                                                                                                                                                 
nina.maller@gmail.com                                                                                                                                                                                       
604955977                                                                                                                                                                                                   
*/

/* Use this variable to remember original terminal attributes. */

int shell = 0;
//we have to check pid before exiting                                                                                                                                                                       
// so it must be global:                                                                                                                                                                              
int debug;      
pid_t pid;



// handler                                                                                                                                                                                                  
// used function from warmup                                                                                                                                                                                
void signal_handler(int signum){
  if(debug) fprintf(stderr, "| entered signal handler | ");
  if(signum == SIGPIPE){
    int status;
    waitpid(pid, &status, WNOHANG);                                                                                  
    if (WIFEXITED(status)) //if child terminate normally                       
      {
        int sig = WTERMSIG(status);
        int stat = WEXITSTATUS(status);
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", sig, stat);
	//	 exit(0);
      }
    exit(0);
     }
  //int stat;
  // waitpid(pid, &stat, 0);
}



int main (int argc, char* argv[])
{
  // get options using getopt_long    
  // server only gets port and compress options                                                                                         
 
  struct option options[] = {
    { "port", required_argument, NULL, 'p'},
    { "compress", no_argument, NULL, 'c'},
    { "debug", no_argument, NULL, 'd'},
    {0, 0, 0, 0} // know when end of array is reached                                                                                                                                                       
  };

  // make flags to indicate which options were chosen;                                                                                                                                                      
  // int shell = 0;                                                                                                                                                                                         
  debug = 0;
  int choice;
  int portno = 0;
  int compress_flag = 0;

  while(1){
    choice = getopt_long(argc, argv, "p:c:d", options, NULL);
    if(choice == -1)
      break;
    switch(choice){
    case 'd':
      debug = 1;
      break;
    case 'p':
      //      fprintf(stdout, "s was selsected");                                                                                                                                                           
      portno = atoi(optarg);
      break;
    case 'c':
      compress_flag = 1;
      break;
    default:
      fprintf(stderr, "Error in option name"); // add error number       
      exit(1);
      break;
    }
  }

  // set up signal handlers:
  /* signal(SIGINT, &signal_handler);
  signal(SIGPIPE, &signal_handler);
  signal(SIGTERM, &signal_handler);
  */
  // set up sockets
  int sockfd, newsockfd; //, clilen;
  struct sockaddr_in serv_addr;//  , cli_addr;
  if(debug) fprintf(stderr, "started debugging!");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }
  memset((char*) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR on binding");
    exit(1);

  }
  listen(sockfd,5);
  // clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) NULL, NULL);//  &cli_addr, &clilen);
  if (newsockfd < 0) {
    fprintf(stderr, "ERROR on accept");
    exit(1);
  }

  if(debug) fprintf(stderr, " | finished setting up socket |");


  shell = 1;
   // if the shell option was specified:                                                                           
  if(shell == 1){
    // make two pipes:                                                                                                 // takes in an array of two integers                                                                               // [0] is set on reading and [1] is set on writing   
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
    if(debug) fprintf(stderr, " | made pipes | ");
    // fork to create a new process                                                                                                                                                                         
    pid = fork();

    if(pid < 0){
      fprintf(stderr, "error calling fork(), errno: %d", errno);
      return(1);
    }

    else if(pid == 0){ // child process 
      if(debug == 1)
        fprintf(stderr, "Started child process");
      int ret = close(parent_to_child[1]); // writing                                                                                                                                                       
      if( ret < 0){
        fprintf(stderr, "failed to close pipe parent_to_child[1], errno: %d", errno);
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
      if( ret < 0){
        fprintf(stderr, "failed to close pipe parent_to_child[0], errno: %d", errno);
        exit(1);
      }

      if(debug == 1) fprintf(stderr, "| child process - trying to exec shell |");

      //finally, exec shell bash                                                                                   
      ret = execl("/bin/bash", "/bin/bash", NULL);
      if( ret<0){
        fprintf(stderr, "failed to exec to shell bash, errno: %d", errno);
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


      struct pollfd mypoll[2];
      // first describes the keyboard stdin                                                                                                                                                                 
      mypoll[0].fd = newsockfd; // OR IS IT SOCKFD
   
      mypoll[0].events = POLLIN |POLLHUP| POLLERR;
   
      mypoll[1].fd = child_to_parent[0];
      mypoll[1].events = POLLIN | POLLHUP | POLLERR;
   
      while(1){
        ret = poll(mypoll, 2, 0);
        if(ret < 0){
          fprintf(stderr, "error in poll(), errno: %d", errno);
          exit(1);
        }

        // there is data to read from the client                                                                   
	if(mypoll[0].revents & POLLIN){
	  if(debug) fprintf(stderr, " | there is data to read from client | ");
          unsigned char buf[256]; // value from specs                                                                  
	  unsigned char compbuf[5000];
          char newline_shell = {0x0A};
          int bytes_read = 0;
          if(compress_flag == 0) bytes_read = read(newsockfd, buf, 256);
	  if(compress_flag == 1) bytes_read = read(newsockfd, compbuf, 256); // to be transferred to buf later
          if(bytes_read < 1){ // instead of less than zero
	    // fprintf(stderr, "error reading from STDIN, errno: %d", errno);
	    // exit(1);
	    kill(pid, SIGINT);
	     signal_handler(SIGPIPE);
	    if(debug) fprintf(stderr, " | server: data to read from client < 1 | ");
          }
	  
	  // if data is compressed we need to decompress:
	  if(compress_flag == 1){
	    z_stream fromcli;
	    fromcli.zalloc = Z_NULL;
	    fromcli.zfree = Z_NULL;
	    fromcli.opaque = Z_NULL;

	    fromcli.avail_in = bytes_read; // size of input buffer
	    fromcli.next_in = compbuf;
	    fromcli.avail_out = 5000; // arbitrary ?
	    fromcli.next_out = buf;

	    inflateInit(&fromcli);
	    ret = inflate(&fromcli, Z_SYNC_FLUSH); //Z_NO_FLUSH or Z_SYNC_FLUSH
	    if( ret < 0){
	      fprintf(stderr, "failed to inflate() in server side");
	      exit(1);
	    }
	    inflateEnd(&fromcli);

	    bytes_read = 5000 - fromcli.avail_out;
	  }
	  
          int i = 0;

	   for(i=0; i<bytes_read; i++){
	   char ch = buf[i]; // get one byte at a time  
           if(buf[i] == '\004'){ //cntrl d
	     if(debug) fprintf(stderr, " | recieved ^d from client | ");
	     ret = close(parent_to_child[1]);
	     if(ret < 0){
	       fprintf(stderr, "failef to close pipe to shell, errno: %d" , errno);
	       exit(1);
	     }
	     // write(newsockfd, '\004', 1);
	     //   close(parent_to_child[1]);
	     kill(pid, SIGINT);
	     signal_handler(SIGPIPE);
	      //close(newsockfd);
	   }  
	   if(buf[i] == '\003'){ // cntrl c
	     if(debug) fprintf(stderr, " | recieved ^c from client | ");
	     ret = close(parent_to_child[1]);
             if(ret < 0){
               fprintf(stderr, "failef to close pipe to shell, errno: %d" , errno);
	       exit(1);
             }
	     kill(pid, SIGINT);
	     signal_handler(SIGPIPE);
	     // close(newsockfd);
	     // char *endbuf;
	     //= {'\003'};
	     // endbuf = "\003";
	     // write(newsockfd, endbuf, 1);
	   }
	   if(ch == 0x0D || ch == 0x0A){
              // char newline_shell = {0x0A};                                                           
	      if(debug) fprintf(stderr, "| recieved a new line from the client | ");
              ret = write(parent_to_child[1], &newline_shell, 1);
              if(ret < 0){
                fprintf(stderr, "error writing to shell from parents process, errno: %d", errno);
                exit(1);
              }
	     }
            else {
	      if(debug) fprintf(stderr, " | from client: %s |", &ch);
	      // ret = write(newsockfd, &ch, 1); 
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
        if(mypoll[1].revents & (POLLHUP | POLLERR)){
	  if(debug) fprintf(stderr, " | POLLHUP or POLLERR is set from client | ");
          ret = close(child_to_parent[1]); // WAS 0
	  // ret = close(mypoll[1].fd);
	  // signal_handler(SIGPIPE);
	  kill(pid, SIGINT);
	  signal_handler(SIGPIPE);
	  if(ret < 0){
            fprintf(stderr, "error closing child_to_parent[0] from parents process, errno: %d", errno);
            exit(1);
          }
          exit(0);
        }


        // if there is input from shell                                                                                                                                                                     
	if(mypoll[1].revents & POLLIN){
	  if(debug) fprintf(stderr, " | there is data to read from SHELL | ");
	  char unsigned buf2[256];
	  int bytes_read2 = 0;
	  bytes_read2 = read(child_to_parent[0], buf2, 256); // CHANGE TO 1 -- mypoll[1].fd
	  if(bytes_read2 < 2){
	      fprintf(stderr, "failed to read from shell, errno: %d", errno);
	      exit(1);
	  }
       	  if (compress_flag == 0) ret = write(newsockfd, &buf2, bytes_read2); // (newsockfd, &cha, 1);
	  if(ret < 0){
	      fprintf(stderr, "error writing to STDOUT, &cha, from parents process, errno: %d", errno);
	      exit(1);
	    }
	 
           // if compressed flag is set, we want to compress the data before sending it 
	  if(compress_flag == 1){
	    z_stream tocli;
            tocli.zalloc = Z_NULL;
            tocli.zfree = Z_NULL;
            tocli.opaque = Z_NULL;


            tocli.avail_in = bytes_read2; // number of bytes coming in                                 
            tocli.next_in = buf2; // input buffer                                                      
            tocli.avail_out = 5000; // arbitrary?                                                     
            unsigned char compbuf[5000]; // unsigned ?                                                   
            tocli.next_out = compbuf;


	    deflateInit(&tocli, Z_DEFAULT_COMPRESSION);
	    deflate(&tocli, Z_SYNC_FLUSH); // flush as much as possible                              
	    deflateEnd(&tocli);
	    int bytes_written_tosock = 5000 - tocli.avail_out;
	    if(bytes_written_tosock < 0){
	      fprintf(stderr, "error reading getting byte num in server side");
	      exit(0);
	    }
            ret = write(newsockfd, compbuf, bytes_written_tosock);   
	    if(ret < 0){
              fprintf(stderr, "error writing to socket on server side, errno: %d", errno);
	      exit(0);
            }
	  }
	     
	     
		
	} // close if there is data from shell
      }// close while, main loop                                                                                                                                                                           
    } // close parent                                                                                                                                                                                       
  } // close shell == 1                                                                                                                                                                                     

  if(debug) fprintf(stderr," | server is still running! ");

  return EXIT_SUCCESS;
}



