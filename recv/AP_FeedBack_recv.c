#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/inet.h>
#include "/usr/src/linux-4.6.3/drivers/net/wireless/ath/ath9k/ath9k.h"

static struct nf_hook_ops nfho;
int rates[16] = {6.5, 13, 19.5, 26, 39, 52, 58.5, 65, 0, 0, 0, 0, 78, 104, 117, 130};
extern struct ath_softc* Pointer;
extern int** sequenceToRate;
unsigned int hook_func(const struct nf_hook_ops *ops, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*))
{
	struct iphdr *iph;          /* IPv4 header */
    struct udphdr *udph;        /* UDP header */
    u32 saddr, daddr;           /* Source and destination addresses */
    unsigned char *user_data;   /* UDP data begin pointer */
    unsigned char *tail;        /* UDP data end pointer */
    unsigned char *it;          /* UDP data iterator */

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
    printk("Printing data: \n");
    for (it = user_data; it != tail; ++it) {
        int c = (int *)*it;
	if (j==0)
	{
		array[k] = c;
		k++;
	}
	if (j==3)
		j=0;
	else
		j=j+1;
    }
    for (k = 0; k < 12; k++) printk("%d, ",array[k]); // change this to 16 to see if 16 indexes are getting here correctly // 16th index for startSequence and 17th for endSequence
        printk("\n");
    int p_recv[4] = {0};
    int p_rates[4] = {0};

    j=1;
    for (k=0; k<12 && j<3; k++)
    {	
	if (array[k]!=0 && p_recv[0]==0)
	{
		p_rates[0] = rates[k];
		p_recv[0]=array[k];
	}
	else if (array[k]!=0)
	{
		if (array[k]>p_recv[0])
		{
			p_rates[j] = p_rates[0];
			p_recv[j]=p_recv[0];
			j++;
			p_recv[0]=array[k];
			p_rates[0]=rates[k];
		}
		else
		{
			p_rates[j] = rates[k];
			p_recv[j]=array[k];
			j++;			
		}
	}
    }
    int t_puts[4] = {0};
    int packetsMax = Pointer->PacketsSent;
    if (packetsMax == 0)
        t_puts[0] = p_rates[0]*p_recv[0]/70;
    else
        t_puts[0] = p_rates[0]*p_recv[0]/packetsMax;
    for (j=1; j < 4; j++)
	t_puts[j] = p_rates[j]*p_recv[j]/10;
    int max = 0;
    for (j=1; j<4; j++)
    {
	if (t_puts[j]>t_puts[max])
		max = j;
    }
    printk("\nThe rate selected is FeedBack Module: %d\n",max);
    Pointer->PacketsSent = 0;
    if (max < 0 || max > 15) max = 0;
    Pointer->DefaultMultiCastRate = max;

    /* step 2
    // Assuming sequenceStart, sequenceEnd is a variable defined above
    int index = 0;
    int endingIndex = 0;
    int sent[16] = {0};
    int tPut[16] = {0};
    for(index = 0; index <= Pointer->filled; index++){
        sent[sequenceToRate[index][1]]++;
        if (sequenceToRate[index][0] == sequenceEnd) {
            endingIndex = index;
            break;
        }
    }

    for(index = 0; index <= Pointer->filled - endingIndex; index++) {
        sequenceToRate[index][0] = sequenceToRate[index + endingIndex][0];
        sequenceToRate[index][1] = sequenceToRate[index + endingIndex][1];
    }
    Pointer->filled = Pointer->filled - endingIndex;
    max = 0;
    maxIndex = 0;
    for(index = 0; index < 16; index++) {
        tPut[index] = rate[index]*array[index]/sent[index];
        if (max <= tPut[index]) {
            max = tPut[index];
            maxIndex = index;
        }
    }
    printk("\nThe rate selected is FeedBack Module: %d\n",maxIndex);
    Pointer->PacketsSent = 0;
    if (maxIndex < 0 || maxIndex > 15) maxIndex = 0;
    Pointer->DefaultMultiCastRate = maxIndex;

    */

    return NF_ACCEPT;
}



int init_module(void)
{

    nfho.hook = (nf_hookfn*) hook_func;
    nfho.hooknum = 0;  
    nfho.pf = PF_INET;
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
