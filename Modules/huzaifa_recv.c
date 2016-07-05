//
//  recv.c
//  NSG-wireless-MultiCast
//
//  Created by Huzaifa Kamran on 6/19/15.

//
#define SHELL "/bin/sh"
#define __KERNEL__
#define MODULE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>    //Provides declarations for ip header
#include <netinet/ip_icmp.h>   //Provides declarations for icmp header
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <time.h>
#include <malloc.h>
#include <netdb.h>

#define FeedBackPacketInterval 100
#define ProbingChance 70

#define MCAST_PORT 1532
#define MCAST_GROUP "224.0.67.67"
#define MSGBUFSIZE 6868


typedef unsigned char byte;
typedef enum { false, true } bool;

static double WEIGHT = 0.5;
static double PROBING_RATE = 100;
/* -------------------- */

/* CONSTANTS */
static  int E_THROUGHPUT = 0;
 static  int PER = 1;
 static  double rates[] = {1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 52};
 static  double rates[] = {6.5,13,19.5,26,39,52,58.5,65,13,26,39,52,78,104,117,130};
/* -------------------- */

 static int current_Rate = 4;
 static int prev_Rate = 0;
 static int leader = 0;

 static int FIXED_RATE = 0;

 static long total_sent = 0;
 static long total_received = 0;

 double rate_table[12][2];
 double PacketsSent[12];
 int Ctable[12];

//static char pkts_Sent[] = "iptables -t mangle --list -v | awk '{if ($3 == \"TTL\") print $ 1}'";
 static  char reset[] = "iptables -t mangle -F";
 static  char mangle[] = "iptables -t mangle -A OUTPUT -j TTL -d 224.0.67.67 --ttl-set ";
 static  char fpath[] = "/sys/kernel/debug/ieee80211/phy0/ath9k/DefaultMultiCastRate";
 static char sshCommand[] = "sshpass -p wireless ssh -o StrictHostKeyChecking=no root@192.168.1.1 ";  
 static char ResetPacketCount[] = "echo 0 > /sys/kernel/debug/ieee80211/phy0/ath9k/PacketsSent";


static  char CommandGetTotalPacketsSentInInterval[] = "cat /sys/kernel/debug/ieee80211/phy0/ath9k/PacketsSent > /home/lion/MultiCast/PacketsSent";
unsigned int PacketsSentInInterval = 0;
unsigned int PacketsSentPerRateInInterval[2] = {0};

static void CalculatePacketsSentPerRateInInterval(void)
{
    FILE* fp;
    system(CommandGetTotalPacketsSentInInterval);

    fp = fopen("/home/lion/MultiCast/PacketsSent","r");
    fscanf(fp,"%u", &PacketsSentInInterval);
    fclose(fp);
    /*fp = fopen("/home/lion/Downloads/MultiCastRate.txt","w");   
    fclose(fp);*/

    PacketsSentPerRateInInterval[0] = ProbingChance*FeedBackPacketInterval/100;
    PacketsSentPerRateInInterval[1] = (FeedBackPacketInterval - PacketsSentPerRateInInterval[0])/3;

        if(PacketsSentPerRateInInterval[0] + 3*PacketsSentPerRateInInterval[1] < FeedBackPacketInterval)
        {
            PacketsSentPerRateInInterval[0] += FeedBackPacketInterval - (PacketsSentPerRateInInterval[0] + 3*PacketsSentPerRateInInterval[1]);
        }

    PacketsSentPerRateInInterval[0] += PacketsSentInInterval - (PacketsSentPerRateInInterval[0] + 3*PacketsSentPerRateInInterval[1]);
}


char* execute_shell_command(char* command,char* output)
{
    
    int status=0;
    pid_t pid;
    
    pid = fork ();
    if (pid == 0)
    {
        execl (SHELL, "bash", "-c", command, NULL);
        size_t bufsize = 0;
        getline(&output, &bufsize, stdin);
        _exit (EXIT_FAILURE);
    }
    else if (pid < 0) status = -1;
    else
            if (waitpid (pid, &status, 0) != pid)
            status = -1;
    
    if (status != -1) return output;
    else return NULL;
}

// THe purpose of this function is to update the mcast_rate debugfs entry
void set_Rate(int rate)
{

    char* t;

        if (FIXED_RATE != 0)
        {
            current_Rate = FIXED_RATE;
            rate = FIXED_RATE;
            char temp[sizeof("echo ") + sizeof(rate) + sizeof(" > ") + sizeof(fpath)+1];
            
            sprintf(temp, "echo %d > %s", rate,fpath);  
            execute_shell_command(temp,t);
        }else{

            char temp[sizeof("echo ") + sizeof(rate) + sizeof(" > ") + sizeof(fpath)+1];
            sprintf(temp, "echo %d > %s", rate,fpath);  
            execute_shell_command(temp,t);
        }    

        execute_shell_command(ResetPacketCount,t);
        printf("The new rate is: %i\n",current_Rate);
}

// THe purpose of this function is to update the TTL entry in the IPTABLES 
void set_TTL(){
        
        printf("Value of ttl (leader): %d \n",leader);
        char *t;
        execute_shell_command(reset,t);

        char mangleUpdated[sizeof(mangle) + sizeof(leader) + 1];
        sprintf(mangleUpdated,"%s%i",mangle,leader);
  
        execute_shell_command((mangleUpdated),t);
}


// to set frag offset
void set_FOffset(){

    long fragOff = current_Rate;
    fragOff = fragOff << 8;
    fragOff += Ctable[current_Rate];
    // Here we need to execute the IPtables command to update the frag_off

}        

// call this whenever we receive feedback so rate_table could be updated
void updateRateTable(int* mcs_feedback)
{
    
    double per = 0;
    double lost = 0;
    int i;


    
       // total_received += mcs_feedback[i];
        lost = PacketsSentPerRateInInterval[0] - mcs_feedback[current_Rate];
        per = lost/PacketsSentPerRateInInterval[0];
        rate_table[current_Rate][PER] = WEIGHT*rate_table[current_Rate][PER] + ((double)(1 - WEIGHT))*(per);
        rate_table[current_Rate][E_THROUGHPUT] = rates[current_Rate]*(1 - rate_table[current_Rate][PER]);

        if (current_Rate > 0){

        lost = PacketsSentPerRateInInterval[1] - mcs_feedback[current_Rate-1];
        per = lost/PacketsSentPerRateInInterval[1];
        rate_table[current_Rate-1][PER] = WEIGHT*rate_table[current_Rate-1][PER] + ((double)(1 - WEIGHT))*(per);
        rate_table[current_Rate-1][E_THROUGHPUT] = rates[current_Rate-1]*(1 - rate_table[current_Rate-1][PER]);

        }

        if (current_Rate < 11){

        lost = PacketsSentPerRateInInterval[1] - mcs_feedback[current_Rate+1];
        per = lost/PacketsSentPerRateInInterval[1];
        rate_table[current_Rate+1][PER] = WEIGHT*rate_table[current_Rate+1][PER] + ((double)(1 - WEIGHT))*(per);
        rate_table[current_Rate+1][E_THROUGHPUT] = rates[current_Rate+1]*(1 - rate_table[current_Rate+1][PER]);

        }

        int j;
        for (j = 0; j <12; j++)
        Ctable[i]=0;
       
}

// Assuming that we will have a feedback after every ______
// we need to call this after 80% of the data has been sent 
void Probe_Minstrel(){



}


// check and update the current rate
void find_Rate(){
    int i;
    for (i=0; i < 12; i++)
        {
            if (rate_table[i][E_THROUGHPUT] > rate_table[current_Rate][E_THROUGHPUT])
            {
                current_Rate = i;
            }
        }
    set_Rate(current_Rate);
}

int main(int argc, char *argv[])
{

    if (argc > 0)
    {
        FIXED_RATE = atoi(argv[1]);
        printf("%s%i","Rate Fixed to ",FIXED_RATE);
    }else {
        FIXED_RATE = 0;
    }
    
    
    struct sockaddr_in addr;
    int fd,addrlen;
    ssize_t nbytes;
    int i;
    int j;
    unsigned long myID;
    unsigned int myTTL;
    char* msgbuf;
    char *message;
    struct ip_mreq mreq;
    struct iphdr *iph;
    int PacketCounter = 0;
    int FeedBackCounter = 0;
    int PacketBytes = 0;
    

    if ((fd=socket(AF_INET,SOCK_RAW,IPPROTO_UDP)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr("224.0.67.67");
    addr.sin_port=htons(MCAST_PORT);

    mreq.imr_multiaddr.s_addr = inet_addr("224.0.67.67");
    mreq.imr_interface.s_addr = inet_addr("192.168.8.1"); 
    
    
    if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    if(setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0){
    perror("setsockopt");
    exit(1);
    }
    msgbuf = (char*)malloc(MSGBUFSIZE);
    addrlen=sizeof(addr);
    unsigned int* number = (unsigned int*) &addrlen;
    while(true)
    {

        if ((nbytes=recvfrom(fd,msgbuf,MSGBUFSIZE,0,(struct sockaddr *) &addr,number)) < 0) {

                perror("recvfrom");
                exit(1);
            }

        iph = (struct iphdr*)msgbuf;
        myID = iph->id;
        myTTL = iph->ttl;

        struct udphdr *udph = (struct udphdr *) (msgbuf + sizeof (iph));
        msgbuf = (char*)(msgbuf + sizeof(iph) + sizeof(udph));

        // convert the data into an int array
        int myArray[nbytes/4];
            
        if (nbytes%4 == 0) {
                for (i = 0; i < nbytes; i+=4) {
                    
                    char tempArray[5];
                    tempArray[4]='\0';
                    
                    for (j = 0; j < 4; j++) {
                        tempArray[j] = msgbuf[i+j];
                    }
                    
                    myArray[i/4] = atoi(tempArray);
                    
                }
        }else{
                
                printf("%s\n%s%zd\n","nbytes is NOT a multiple of 4","Size of NBYTES is: ",nbytes);
        }

        int* mcs_feedback = myArray;

        if(mcs_feedback[12] == 99){ // This is a takeover message packet

            // 1) clear out the records 
                for(i = 0; i < 12; i++){
                    PacketsSent[i]=0;
                    Ctable[i]=0;
                }

            //  2) Now check the ttl field and save it in the leader field
                leader = myTTL;

            //  3)  Now update the leader infomration 
                  set_TTL();
                  set_FOffset(); // just to be sure

        }else if(mcs_feedback[12] == 66){ // This is a feedback message packet

                // 1) extract all the information from the feedback packet
                 updateRateTable(mcs_feedback);

                // 2) Use the information to find a better rate
                 find_Rate();
        }


    }
}






