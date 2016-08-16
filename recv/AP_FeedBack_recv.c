#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/inet.h>
#include "/usr/src/linux-4.6.3_new/drivers/net/wireless/ath/ath9k/ath9k.h"

static struct nf_hook_ops nfho;
int rates[16] = {6.5, 13, 19.5, 26, 39, 52, 58.5, 65, 0, 0, 0, 0, 78, 104, 117, 130};
extern struct ath_softc* Pointer;
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*))
{
	struct iphdr *iph;          /* IPv4 header */
    struct udphdr *udph;        /* UDP header */
    u32 saddr, daddr;           /* Source and destination addresses */
    unsigned char *user_data;   /* UDP data begin pointer */
    unsigned char *tail;        /* UDP data end pointer */
    unsigned char *it;          /* UDP data iterator */
    int sequenceStart;
    int sequenceEnd;

    /* Network packet is empty, seems like some problem occurred. Skip it */
    if (!skb)
        return NF_ACCEPT;

    iph = ip_hdr(skb);          /* get IP header */

    if (!(iph->ttl == 99 && (iph->daddr) == in_aton("224.0.67.67")))
    	return NF_ACCEPT;
    /* Skip if it's not UDP packet */
    if (iph->protocol != IPPROTO_UDP)
        return NF_ACCEPT;

    udph = udp_hdr(skb);        /* get UDP header */

    /* Convert network endianness to host endiannes */
    saddr = ntohl(iph->saddr);
    daddr = ntohl(iph->daddr);

    /* Calculate pointers for begin and end of UDP packet data */
    user_data = (unsigned char *)((unsigned char *)udph + (8));
    tail = skb_tail_pointer(skb);
    printk("packed received______\n");
    /* ----- Print all needed information from received UDP packet ------ */
	int array[18]={0};
	int k=0;
	int j=0;
    /* Print UDP packet data (payload) */
    // Old start
    
    printk("Printing data: \n");
    for (it = user_data; it != tail && k < 18; it = it + 4, k++) {
        array[k] = (((int)*it & 0xFF) << 24) + (((int)*(it + 1) & 0xFF) << 16) + (((int)*(it + 2) & 0xFF) << 8) + (((int)*(it + 3) & 0xFF)); 
    }

    for (k = 0; k < 16; k++) printk("%d, ",array[k]); // change this to 16 to see if 16 indexes are getting here correctly // 16th index for startSequence and 17th for endSequence
        printk("\n");
    // //printk("Sequence Start: %d    Sequence End: %d\n", array[16], array[17]);
    sequenceStart = array[16];
    sequenceEnd = array[17];

    // step 2
    int index = 0;
    int endingIndex = 0;
    int sent[16] = {0};
    int tPut[16] = {0};
    for(index = 0; index < Pointer->filled; index++){
        int rt = Pointer->sequenceToRate[index][1];
        if (rt < 16)
            sent[Pointer->sequenceToRate[index][1]]++;
        else 
            printk("Index greater then the array\n");
        if (Pointer->sequenceToRate[index][0] == sequenceEnd) {
            endingIndex = index;
            break;
        }
    }

    // printk("The ending index is: %d\n", endingIndex);
    // printk("The pointer->filled is: %d\n", Pointer->filled);

    for(index = 0; index < Pointer->filled - endingIndex; index++) {
        if (index + endingIndex < 5000){
            Pointer->sequenceToRate[index][0] = Pointer->sequenceToRate[index + endingIndex][0];
            Pointer->sequenceToRate[index][1] = Pointer->sequenceToRate[index + endingIndex][1];
        } else 
            printk("index + endingIndex is greater then 5000 \n" );
    }
    
    Pointer->filled = Pointer->filled - endingIndex;
    //printk("The new pointer->filled is: %d\n", Pointer->filled);
    int max = 0;
    int maxIndex = 0;
    for(index = 0; index < 16; index++) {
        if (array[index] != 0 || sent[index] != 0)
            tPut[index] = rates[index]*array[index]/sent[index];  // Note: Sent[index] could be zero
        else 
            tPut[index] = 0;
        if (max <= tPut[index]) {
            max = tPut[index];
            maxIndex = index;
        }
    }
    printk("\nThe rate selected is FeedBack Module: %d\n",maxIndex);
    Pointer->PacketsSent = 0;
   // printk("SetPacketInKernal\n\n");
    // if (maxIndex < 0 || maxIndex >= 15){
    //     maxIndex = 0;
    // }
    Pointer->DefaultMultiCastRate = maxIndex;


    return NF_ACCEPT;
}



int init_module(void)
{

    nfho.hook = (nf_hookfn*) hook_func;
    nfho.hooknum = 0;  
    nfho.pf = PF_INET;
    //Pointer->filled = 0;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_hook(&nfho);
    printk("Hook registered\n");
    return 0;
}

void cleanup_module(void)
{
    nf_unregister_hook(&nfho);
    printk("Hook Unregistered\n");
}
