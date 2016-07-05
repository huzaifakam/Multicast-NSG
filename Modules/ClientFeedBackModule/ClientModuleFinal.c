#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/mm.h>
#include <net/ip.h>
#include <net/udp.h>
#include <linux/init.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <net/sock.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/netpoll.h>
#include <linux/string.h>
#include <asm/unaligned.h>

#define THRESHOLD 100;

unsigned int MultiCastAddress;
int counter;
static bool SendPacket(unsigned char * Message);

static struct nf_hook_ops nfho;   //net filter hook option struct
struct sk_buff *sock_buff;
struct iphdr *ip_header;            //ip header struct

//--------------------------------------------------------------------------------------------------
//                The Following Variables Are Required For SendPacket() Function
//--------------------------------------------------------------------------------------------------

unsigned int SourceAddress;
unsigned int DestinationAddress;

static atomic_t IP_Ident;
bool Status;

static int DataLength, UDPLength, IPLength, TotalLength;
static struct sk_buff *skb;
static struct udphdr *UDPHeader;
static struct iphdr *IPHeader;
static struct ethhdr *ETHHeader;
static struct net_device *OutputDevice;
char * Message = "1\n";
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#define FeedPacketPacketInterval 100
unsigned int TotalPacketsReceivedInCurrentInterval = 0;
int RateTable[17] = {0};

unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb1, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*))
{
    int i;
    sock_buff = skb1;
    ip_header = (struct iphdr *)skb_network_header(sock_buff);
    unsigned int TempSourceAddress = (unsigned int)ip_header->saddr;

        if(!sock_buff) { return NF_ACCEPT;}

        if (ip_header->protocol==17 && TempSourceAddress == in_aton("192.168.8.1")) 
        {
            RateTable[ip_header -> ttl]++;
            counter++;
         //   RateTable[ip_header->id >> 12]++;
            if (counter >= THRESHOLD) {
                counter = 0;
                Message = (char *)RateTable;
                SendPacket(Message); 

                for (i = 0; i < 17; i++) {
                    RateTable[i] = 0;
                }

            }

            printk(KERN_INFO "Fragmentation Offset: %u\n", ip_header->id >> 12);
            return NF_ACCEPT;
        }

    return NF_ACCEPT;
}

int init_module(void)
{

    nfho.hook = (nf_hookfn*) hook_func;
    nfho.hooknum = 0;  
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_hook(&nfho);

    MultiCastAddress = in_aton("224.0.67.67");
    SourceAddress = in_aton("192.168.2.106");
    DestinationAddress = in_aton("224.0.67.67");
    counter = 0;
  //   unsigned char Message_[69];
  //   Message_[68] = '\0';


     int i = 0; 


    for (i = 0; i < 17; i++) {
		RateTable[i] = 97 + i;
    }

  //   for (i = 0; i < 17; i=i+4) {
		// Message_[i] =  RateTable[i] & 0xFF000000;
		// Message_[i + 1] =  RateTable[i] & 0x00FF0000;
		// Message_[i + 2] =  RateTable[i] & 0x0000FF00;
		// Message_[i + 3] =  RateTable[i] & 0x000000FF;
  //   }

  //   for (i = 0; i < 17 * 4; i++) {
	 //    printk("Message: %d, %c\n", i, Message_[i]);
  //   }



    // for (i = 0; i < 10; i++) {
    // 	SendPacket((char*)RateTable);
    // }

	printk("Size of int %d\n",sizeof(int));
    printk("Hook Registered\n");
    return 0;
}

void cleanup_module(void)
{
    nf_unregister_hook(&nfho);
    printk("Hook Unregistered\n");
}

static bool SendPacket(unsigned char * Message)
{

    DataLength = strlen(Message);
    DataLength = 17 * 4 + 1;

    printk("Message Length: %d\n", DataLength);

    UDPLength = DataLength + sizeof(*UDPHeader); 
    IPLength = UDPLength + sizeof(*IPHeader); 
 
    OutputDevice = (struct net_device*)(dev_get_by_name(&init_net,"wlan0"));
    TotalLength = IPLength + LL_RESERVED_SPACE(OutputDevice);

    skb = alloc_skb(TotalLength, GFP_ATOMIC);
    if (!skb) return 0; 

    atomic_set(&skb->users, 1);
    skb_reserve(skb, TotalLength + OutputDevice -> needed_tailroom);

    skb_copy_to_linear_data(skb, Message, DataLength); 
    skb_put(skb, DataLength); 

    skb_push(skb, sizeof(*UDPHeader)); 
    skb_reset_transport_header(skb);
    UDPHeader = udp_hdr(skb);
    UDPHeader->source = SourceAddress;
    UDPHeader->dest = DestinationAddress;
    UDPHeader->len = htons(UDPLength);
    UDPHeader->check = 0;
    UDPHeader->check = csum_tcpudp_magic(SourceAddress, DestinationAddress, UDPLength, IPPROTO_UDP, csum_partial(UDPHeader, UDPLength, 0));
    
        if (UDPHeader->check == 0)
        {
            UDPHeader->check = CSUM_MANGLED_0;
        }
    
    skb_push(skb, sizeof(*IPHeader));
    skb_reset_network_header(skb);

    IPHeader = ip_hdr(skb);
    put_unaligned(0x45, (unsigned char *)IPHeader);
    IPHeader->version = 4;
    IPHeader->ihl = 5;
    IPHeader->tos = 0;
    put_unaligned(htons(IPLength), &(IPHeader->tot_len));
    IPHeader->id = htons(atomic_inc_return(&IP_Ident));
    IPHeader->frag_off = 0;
    IPHeader->ttl = 99;
    IPHeader->protocol = IPPROTO_UDP;
    IPHeader->check = 0;
    put_unaligned(SourceAddress, &(IPHeader->saddr));
    put_unaligned(DestinationAddress, &(IPHeader->daddr));
    IPHeader->check = ip_fast_csum((unsigned char *)IPHeader, IPHeader->ihl);

    ETHHeader = (struct ethhdr *) skb_push(skb, ETH_HLEN);
    skb_reset_mac_header(skb);

    skb->protocol = ETHHeader->h_proto = htons(ETH_P_IP);

    ETHHeader->h_source[0] = 0x48;
    ETHHeader->h_source[1] = 0x51;
    ETHHeader->h_source[2] = 0xb7;
    ETHHeader->h_source[3] = 0x36;
    ETHHeader->h_source[4] = 0xae;
    ETHHeader->h_source[5] = 0x84;

    ETHHeader->h_dest[0] = 0xe8;
    ETHHeader->h_dest[1] = 0xde;
    ETHHeader->h_dest[2] = 0x27;
    ETHHeader->h_dest[3] = 0x13;
    ETHHeader->h_dest[4] = 0x1f;
    ETHHeader->h_dest[5] = 0x0f;

    skb->dev = (struct net_device*)(dev_get_by_name(&init_net,"wlan0"));
    skb->pkt_type = PACKET_OUTGOING;
    dev_queue_xmit(skb);
    return 1;
}
