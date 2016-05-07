/* 
   Author:     Marie Faltusová 
   Project:    Simple SNMP manager
   Date:       14.11.2015
                                 */


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

#define MSIZE 512

// Function declaration

int get_parameters(int, char**);
int send(char);
void create_msg(bool, int, int);

// Global variables

int interval = 0;
string com_str;
string agents;
const char* agent;

char send_msg[MSIZE];
unsigned char received_msg[MSIZE];

int main(int argc, char* argv[])
 {

     int interface = 0;
     int val_size = 0;

    	 
     //Timer
     struct timeval t_beg;
     struct timeval t_end;
     int run_time;
     
     //Date
     struct timeval date;
     char date_print[40];


     // Get parameters
     if(get_parameters(argc,argv) == 1)
      {
	cerr<<"Wrong parameters!"<<endl;
	return 0;
      }    

     // Send messages until user press Ctrl+C - SIGINIT
    while(1)
     {

     gettimeofday(&t_beg, NULL);     
     
     
     // Create a first message
      create_msg(true,0,0);	// short message

     //Send a message
     if(send(*send_msg) == 1)
      {
	cerr<<"Error when sending message!"<<endl;
	return 0;
      }

     gettimeofday(&date, NULL);
     strftime(date_print,40,"%Y-%m-%d %T.",localtime(&date.tv_sec));
     printf("%s%03ld",date_print,date.tv_usec/1000);
     cout<<";";
     
     interface = received_msg[37+com_str.size()];
      	
     // Other, long messages
     for(int inter_num=1; inter_num<=interface; inter_num++)    // interfaces and 22 parameters
      {
	for(int par_num=1; par_num<=22; par_num++) 
	{          
          
           // Create and send longer message				
	   create_msg(false,inter_num,par_num);
        
	   if(send(*send_msg) == 1)
      	    {
		cerr<<"Error when sending message!"<<endl;
		return 0;
            }
           val_size=received_msg[38+com_str.size()];
	   uint64_t val_print = 0;

	   //cout<<" val_size>"<<val_size;

	   if(received_msg[16] != 4)  // if received ID is not 4 baits long
	     {
		val_size=received_msg[38+com_str.size() - 4 + received_msg[10+com_str.size()]];
	     }
	//cout<<" val_size>"<<val_size;
           
	   // Get and print values from received message
	   if(val_size == 0)
	     {
		cout<<" ;";
	     }
           else if(par_num == 2)    // Here is interface name, print letter by letter
             {
	  	for(int m=1; m<=val_size; m++)
		 {
		    cout<<received_msg[(38 + com_str.size() - 4 + received_msg[10+com_str.size()]) + m];
		 }
		
		 cout<<";";
	    }	    
	    else                    // Print numbers
	     {
		for(int n=1; n<=val_size; n++)
		{
		   val_print = val_print<< 8;       // bit shift to left
		   val_print += received_msg[(38 + com_str.size() - 4 + received_msg[10+com_str.size()]) + n];	   
		}

		if(par_num == 6)
		{
		   for(int k = 1; k <= val_size; k++)
		    {
			if((received_msg[38 + com_str.size() + k]) < 10)  
			  printf("0%x",received_msg[(38 + com_str.size() - 4 + received_msg[10+com_str.size()]) + k]);
			else
			  printf("%x",received_msg[(38 + com_str.size() - 4 + received_msg[10+com_str.size()]) + k]);

			if(k<val_size)   printf(":");			
		    }

		    printf(";");
		   
		}
		if(par_num == 22)
		{
		   cout<<"."<<val_print;
		}
		
		cout<<val_print<<";";
	     }
        }
       }

     // Interval watchdog
     gettimeofday(&t_end, NULL);
    
     run_time = ((t_end.tv_sec - t_beg.tv_sec) * 1000) + ((t_end.tv_usec - t_beg.tv_usec) / 1000.0) + 0.5;
     
     if(interval <= run_time)
      {
	 cout<<"Interval exceeded...";	 
      }
     else if(interval > run_time)
       {
	 while(interval > run_time)
	 {
	    gettimeofday(&t_end, NULL);
	    run_time = ((t_end.tv_sec - t_beg.tv_sec) * 1000) + ((t_end.tv_usec - t_beg.tv_usec) / 1000.0) + 0.5;            
	 } 
                 
       } 
   
      cout<<endl;  
     }//end while(1)
     
     return 0;
 }

// Get the parameters from user
int get_parameters(int argc,char* argv[])
 {
     if(argc == 4 || argc == 6)
      {
	if(strcmp(argv[1],"-i") == 0)
	{
	   interval = atoi(argv[2]);
	   if(interval <= 0) return 1;
	   if(strcmp(argv[3],"-c") != 0) return 1;
	   com_str = argv[4];
	   agents = argv[5];	   
	}
	else if(strcmp(argv[1],"-c") == 0)
	{
	   interval = 100;
	   com_str = argv[2];
	   agents = argv[3];	  
	}
	else return 1;
      }
      else if((argc == 2) && (strcmp(argv[1],"--help") == 0))
      {
	cout<< "**************************************************************************\n"
	       "	This is a simple SNMP manager for connecting you to agent, \n"
               "	please, give parameters in this format:\n"
               "	./isaSnmpIfLog [-i interval] -c community_string AGENT \n"
               "	where INTERVAL is optional parameter for waiting for next value\n"
	       "	      COM. STRING is password to connect\n"
	       "	      AGENT is domain name or IP\n"
	       "**************************************************************************\n"
        <<endl;      	
      }
      else return 1;

     agent = agents.c_str();
     return 0;
 }

// Send a message, UDP
int send(char packet)
 {
     int sckt; 
     int port = 161;  // SNMP port
     
     // Socket variables
     struct sockaddr_in sin;
     struct hostent *hptr;
     struct sockaddr_in si_other;
     int sckt_len = sizeof(si_other);
     
     // Set a timeout for waiting for response, we won't wait forever! (1 sec)
     struct timeval timeout={5,0};     

     // Create a socket
     if ((sckt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
      {
       	cerr<<"Socket error - "; 
	return 1;
      }     

     // Set connection
     memset((char *) &si_other, 0, sizeof(si_other));
     si_other.sin_family = AF_INET;
     si_other.sin_port = htons(port);

     // Get IP from domain name (parameter agent)
     if ((hptr = gethostbyname(agent)) == NULL)
      {
        struct in_addr addr;
	inet_aton(agent, &addr);
        	
	if ((hptr = gethostbyaddr(&addr, sizeof(addr), AF_INET)) == NULL)
         {
	   cerr<<"Address error - "; 
           return 1; 
	 }
      }
     memcpy(&si_other.sin_addr, hptr->h_addr, hptr->h_length);

     // Send a socket
     if (sendto(sckt, send_msg, 39+com_str.size() , 0 ,(struct sockaddr *)&si_other,  sckt_len) < 0) 
      {
        cerr<<"Send error - ";    
	return 1;
      }

     // Settings before receiving message from server
     memset(received_msg,'\0', MSIZE); 
     setsockopt(sckt,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

     // Receive a message     
     if ((recvfrom(sckt, received_msg, MSIZE, 0, (struct sockaddr *) &si_other, (socklen_t*)&sckt_len)) >= 0) 
      {
	     // Received messages                                     
      }
      else
      {	
	cerr<<"Agent not responding - ";    
	return 1;
      }

     // Close & clean a socket
      if (close(sckt) < 0) 
      { 
	cerr<<"Close error - ";  
	return 1;
      }			
     	
     return 0; 
 }

// Create short or long message
void create_msg(bool short_true,int interface, int par_num)
 {
     int comstr_size = com_str.size();
     int msg_ptr = 0;

     send_msg[msg_ptr] = 48;
     msg_ptr++;

     if(short_true)                          
      {
	send_msg[msg_ptr] = 40 + (comstr_size - 6);
	msg_ptr++;
      }
      else
      {
	send_msg[msg_ptr] = 43 + (comstr_size - 6);
	msg_ptr++;
      }
     
     send_msg[msg_ptr] = 2;
     msg_ptr++;
     send_msg[msg_ptr] = 1;
     msg_ptr++;
     send_msg[msg_ptr] = 0;
     msg_ptr++;
     send_msg[msg_ptr] = 4;
     msg_ptr++;
     send_msg[msg_ptr] = comstr_size;
     msg_ptr++;

     for(int x=0; x<=comstr_size-1; x++) 
      {
	send_msg[msg_ptr] = com_str[x];
	msg_ptr++;
      }

     if(short_true)
      {
	send_msg[msg_ptr] = 161; 
	msg_ptr++;
	send_msg[msg_ptr] = 27;  
	msg_ptr++;
      }
      else
      {
	send_msg[msg_ptr] = 160; 
	msg_ptr++;
	send_msg[msg_ptr] = 30;  
	msg_ptr++;				
      }

      send_msg[msg_ptr] = 2;  
      msg_ptr++;
      send_msg[msg_ptr] = 4;  
      msg_ptr++;

      // Random unique ID
      send_msg[msg_ptr] = rand(); 
      msg_ptr++;
      send_msg[msg_ptr] = rand(); 
      msg_ptr++;
      send_msg[msg_ptr] = rand(); 
      msg_ptr++;
      send_msg[msg_ptr] = rand(); 
      msg_ptr++;
					
      send_msg[msg_ptr] = 2;  
      msg_ptr++;
      send_msg[msg_ptr] = 1;  
      msg_ptr++;
      send_msg[msg_ptr] = 0;  
      msg_ptr++;
					
      send_msg[msg_ptr] = 2;  
      msg_ptr++;
      send_msg[msg_ptr] = 1;  
      msg_ptr++;
      send_msg[msg_ptr] = 0;  
      msg_ptr++;
					
      send_msg[msg_ptr] = 48; 
      msg_ptr++;
	
     if(short_true)
      {
	send_msg[msg_ptr] = 13;  
	msg_ptr++; 
	send_msg[msg_ptr] = 48;  
	msg_ptr++;
	send_msg[msg_ptr] = 11;  
	msg_ptr++; 
	send_msg[msg_ptr] = 6;   
	msg_ptr++;
	send_msg[msg_ptr] = 7;   
	msg_ptr++; 
      }
      else
      {
	send_msg[msg_ptr] = 16;  
	msg_ptr++; 
	send_msg[msg_ptr] = 48;  
	msg_ptr++;
	send_msg[msg_ptr] = 14;  
	msg_ptr++; 
	send_msg[msg_ptr] = 6;   
	msg_ptr++;
	send_msg[msg_ptr] = 10;  
	msg_ptr++; 
      }
					
      send_msg[msg_ptr] = 43;  
      msg_ptr++;
      send_msg[msg_ptr] = 6;   
      msg_ptr++;
      send_msg[msg_ptr] = 1;   
      msg_ptr++;
      send_msg[msg_ptr] = 2;   
      msg_ptr++;
      send_msg[msg_ptr] = 1;   
      msg_ptr++;
      send_msg[msg_ptr] = 2;   
      msg_ptr++;					
					
     if(short_true)
      {
	send_msg[msg_ptr] = 1;  
        msg_ptr++;
      }
      else
      {
	send_msg[msg_ptr] = 2;  
	msg_ptr++;
	send_msg[msg_ptr] = 1;  
	msg_ptr++;
	// Parameter 1-22
	send_msg[msg_ptr] = par_num;  
	msg_ptr++;
	// Interface
	send_msg[msg_ptr] = interface;  
	msg_ptr++;
      }
		
      send_msg[msg_ptr] = 5;  
      msg_ptr++;
      send_msg[msg_ptr] = 0;

     return;
 }





























