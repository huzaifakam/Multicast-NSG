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
#include <linux/igmp.h>
#include "/usr/src/linux-3.13.2/drivers/net/wireless/ath/ath9k/ath9k.h"

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *));
static void CheckAndAddMemberToGroup(void);
static void CheckAndRemoveMemberFromGroup(void);

extern struct ath_softc *Pointer;

static struct nf_hook_ops nfho;   //net filter hook option struct
struct sk_buff *sock_buff;
struct udphdr *udp_header;          //udp header struct (not used)
struct iphdr *ip_header;            //ip header struct
struct igmpv3_report *IGMPReportHeader;
static struct igmpv3_grec TempStruct;

unsigned int SourceAddress;

__be32 CurrentMembersOfGroup[255] = {0};
int CurrentRSSIOfMembers[255] = {0};
int MemberCount = 0;

extern struct ath_softc *Pointer;

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *))
{
        sock_buff = skb;

        ip_header = (struct iphdr *)skb_network_header(sock_buff);    //grab network header using accessor
        SourceAddress = (unsigned int)ip_header->saddr;
       
        if(!sock_buff) { return NF_ACCEPT;}
        
        if ((ip_header->protocol == 2))
        {        
    		IGMPReportHeader = (struct igmpv3_report *)skb_transport_header(skb);
    		TempStruct = IGMPReportHeader->grec[0];
    			
    			if(TempStruct.grec_mca == in_aton("224.0.67.67"))
    			{
    				if(TempStruct.grec_type == 4)
    				{
    					CheckAndAddMemberToGroup();
    				}
    				else if(TempStruct.grec_type == 3)
    				{
    					CheckAndRemoveMemberFromGroup();
    				}	
    				printk("%d\n", TempStruct.grec_type);
    					
    			}
    		
    		return NF_ACCEPT;
        }
       
        return NF_ACCEPT;
}
 
int init_module()
{
        nfho.hook = hook_func;
        nfho.hooknum = NF_INET_PRE_ROUTING;
        nfho.pf = PF_INET;
        nfho.priority = NF_IP_PRI_FIRST;
 		nf_register_hook(&nfho);
        printk(KERN_INFO "init_module() called\n");

        return 0;
}
 
void cleanup_module()
{
        printk(KERN_INFO "cleanup_module() called\n");
        nf_unregister_hook(&nfho);     
}

static void CheckAndAddMemberToGroup(void)
{
		if(MemberCount == 0)
		{
			CurrentMembersOfGroup[0] = SourceAddress;
			MemberCount++;
			return;	
		}

	int Index;

		for(Index = 0; Index < MemberCount; Index++)
		{
			if(CurrentMembersOfGroup[Index] == SourceAddress)
			{
				return;
			}
		}

	CurrentMembersOfGroup[MemberCount] = SourceAddress;
	MemberCount++;
}

static void CheckAndRemoveMemberFromGroup(void)
{
		if(MemberCount == 0)
		{
			return;	
		}

	int Index;

		for(Index = 0; Index < MemberCount; Index++)
		{
			if(CurrentMembersOfGroup[Index] == SourceAddress)
			{
				CurrentMembersOfGroup[Index] = CurrentMembersOfGroup[MemberCount-1];
				CurrentMembersOfGroup[MemberCount-1] = 0;
				MemberCount--;
				return;
			}
		}
}