//'Hello World' netfilter hooks example
//For any packet, we drop it, and log fact to /var/log/messages

#include <linux/netpoll.h>
#include <linux/dma-mapping.h>
#include <linux/netpoll.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <asm/unaligned.h>
#include "/usr/src/linux-3.13.2/drivers/net/wireless/ath/ath9k/ath9k.h"
#include <net/checksum.h>
#include <net/ip.h>

// MultiCastRate
// PacketsSent (Probing)
// DefaultMultiCastRate

static void CalculateMaxPacketPerInterval(void);
static void SendMultiCastPacket(void);
static void ResetRateTableAndPacketCount(void);
static void PopulateRateTable(void);
static bool IsRateAlreadyPresent(int Rate, int SizeOfRateTable);
static void GetRandomRate(int Counter);
static int GetRateToSendPacket(void);
static void ModifyTTLfield(void);
static void UpdateDebugFsSendingRateAndPacketCount(void);
static void ReCalculateIPCheckSum(void);

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *));


static struct nf_hook_ops nfho;   //net filter hook option struct
struct sk_buff *sock_buff;
struct udphdr *udp_header;          //udp header struct (not used)
struct iphdr *ip_header;            //ip header struct

static int RandomMultiCastRatePool[16] = {0,1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

#define FeedBackPacketInterval 100
#define ProbingChance 70
#define MaxMCSIndex 15

int MaxPacketPerInterval[2] = {0};
int CurrentSendingRates[4] = {0};
int StatusPacketsSentPerRate[4]= {0};
bool HighRateOrLowRate = true;

unsigned int DestinationAddress;
unsigned int PacketCountInModule = 0;
unsigned int PacketCountInKernel = 0;
int CurrentPacketSendingRate = 0;
int DefaultMultiCastRate = 0;

extern struct ath_softc *Pointer;

static void SendMultiCastPacket(void)
{
	PacketCountInKernel = Pointer->PacketsSent; //probing interval (0 when we get the feedback packet)

		if(PacketCountInKernel == 0)
		{
			ResetRateTableAndPacketCount();
			PopulateRateTable();
			PacketCountInModule = 0;
			printk("%d, %d\n", MaxPacketPerInterval[0], MaxPacketPerInterval[1]);
			printk("%d, %d, %d, %d \n", CurrentSendingRates[0], CurrentSendingRates[1], CurrentSendingRates[2], CurrentSendingRates[3]);
		}

	CurrentPacketSendingRate = GetRateToSendPacket();
	//ModifyTTLfield();*/
	UpdateDebugFsSendingRateAndPacketCount();
	ReCalculateIPCheckSum();
}

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
        sock_buff = skb;

        ip_header = (struct iphdr *)skb_network_header(sock_buff);    //grab network header using accessor
        DestinationAddress = (unsigned int)ip_header->daddr;
        if(!sock_buff) { return NF_ACCEPT;}
        
        if ((ip_header->protocol==17) && (DestinationAddress == in_aton("192.168.8.76")))
        {     
          	SendMultiCastPacket();
          	return NF_ACCEPT;
        }
       
        return NF_ACCEPT;
}
 
int init_module()
{
        nfho.hook = hook_func;
        nfho.hooknum = NF_INET_LOCAL_OUT;
        nfho.pf = PF_INET;
        nfho.priority = NF_IP_PRI_FIRST;
 		nf_register_hook(&nfho);
 		CalculateMaxPacketPerInterval();
        printk(KERN_INFO "init_module() called\n");

        return 0;
}
 
void cleanup_module()
{
        printk(KERN_INFO "cleanup_module() called\n");

        Pointer->PacketsSent = 0;
        Pointer->MultiCastRate = 0;
        Pointer->DefaultMultiCastRate = 0;

        nf_unregister_hook(&nfho);     
}

static void CalculateMaxPacketPerInterval(void)
{
	MaxPacketPerInterval[0] = ProbingChance*FeedBackPacketInterval/100;
	MaxPacketPerInterval[1] = (FeedBackPacketInterval - MaxPacketPerInterval[0])/3;

		if(MaxPacketPerInterval[0] + 3*MaxPacketPerInterval[1] < FeedBackPacketInterval)
		{
			MaxPacketPerInterval[0] += FeedBackPacketInterval - (MaxPacketPerInterval[0] + 3*MaxPacketPerInterval[1]);
		}
}

static void ResetRateTableAndPacketCount(void)
{
	memset(StatusPacketsSentPerRate, 0, sizeof(StatusPacketsSentPerRate));
	StatusPacketsSentPerRate[0] = MaxPacketPerInterval[0];
	StatusPacketsSentPerRate[1] = MaxPacketPerInterval[1];
	StatusPacketsSentPerRate[2] = MaxPacketPerInterval[1];
	StatusPacketsSentPerRate[3] = MaxPacketPerInterval[1];
	
	memset(CurrentSendingRates, 0, sizeof(CurrentSendingRates));
}

static void PopulateRateTable(void)
{
	int Counter = 0;
	
	CurrentSendingRates[Counter] = Pointer->DefaultMultiCastRate; //current multicast rate

		if(CurrentSendingRates[0] == 0)
		{
			CurrentSendingRates[++Counter] = 1;
			GetRandomRate(++Counter);
			GetRandomRate(++Counter);			
		}
		else if(CurrentSendingRates[0] == MaxMCSIndex)
		{
			CurrentSendingRates[++Counter] = MaxMCSIndex - 1;
			GetRandomRate(++Counter);
			GetRandomRate(++Counter);		
		}
		else
		{
			CurrentSendingRates[++Counter] = Pointer->DefaultMultiCastRate + 1;
			CurrentSendingRates[++Counter] = Pointer->DefaultMultiCastRate - 1;
			GetRandomRate(++Counter);

		}	
}

static bool IsRateAlreadyPresent(int Rate, int SizeOfRateTable)
{
	int Index;

		for(Index = 0; Index < SizeOfRateTable; Index++)
		{
			if(CurrentSendingRates[Index] == Rate)
			{
				return true;
			}
		}

	return false;
}

static void GetRandomRate(int Counter)
{
	int RandomNumber, RandomRate;
	bool FoundRandomRate = false;

	if(Pointer->DefaultMultiCastRate == 0 || Pointer->DefaultMultiCastRate == MaxMCSIndex || Pointer->DefaultMultiCastRate == 1 || Pointer->DefaultMultiCastRate == MaxMCSIndex -1)
	{
		while(!FoundRandomRate)
		{
			get_random_bytes(&RandomNumber, sizeof(RandomNumber));
			RandomNumber = RandomNumber % (sizeof(RandomMultiCastRatePool)/sizeof(RandomMultiCastRatePool[0]));
			RandomRate = RandomMultiCastRatePool[RandomNumber];

				if(IsRateAlreadyPresent(RandomRate, Counter))
				{
					continue;
				}

			CurrentSendingRates[Counter] = RandomRate;
			FoundRandomRate = true;
		}
	}
	else
	{
		while(!FoundRandomRate)
		{
			get_random_bytes(&RandomNumber, sizeof(RandomNumber));
			RandomNumber = RandomNumber % (sizeof(RandomMultiCastRatePool)/sizeof(RandomMultiCastRatePool[0]));
			RandomRate = RandomMultiCastRatePool[RandomNumber];

				if(IsRateAlreadyPresent(RandomRate, Counter))
				{
					continue;
				}

				if(HighRateOrLowRate)
				{
					if(RandomRate < CurrentSendingRates[1])
					{
						continue;
					}
				}
				else
				{
					if(RandomRate > CurrentSendingRates[1])
					{
						continue;
					}
				}

			CurrentSendingRates[Counter] = RandomRate;
			HighRateOrLowRate = !(HighRateOrLowRate);
			FoundRandomRate = true;
		}
	}
}

static int GetRateToSendPacket(void)
{

	unsigned int RandomNumber, RateIndex;

		while(1)
		{
				if(PacketCountInModule >= FeedBackPacketInterval)
				{
					return CurrentSendingRates[0];
				}

			get_random_bytes(&RandomNumber, sizeof(RandomNumber));
			RateIndex = RandomNumber % 4;

				if(StatusPacketsSentPerRate[RateIndex] != 0)
				{
					StatusPacketsSentPerRate[RateIndex]--;
					return CurrentSendingRates[RateIndex];
				}
		}
}

static void ModifyTTLfield(void)
{
	ip_header->ttl = CurrentPacketSendingRate;
	return;
}

static void UpdateDebugFsSendingRateAndPacketCount(void)
{
	Pointer->MultiCastRate = CurrentPacketSendingRate;
	ip_header->id = CurrentPacketSendingRate << 12;
	
	PacketCountInModule++;
	Pointer->PacketsSent = PacketCountInModule;
}

static void ReCalculateIPCheckSum(void)
{
	ip_header->check = 0;
    ip_send_check(ip_header);
}
