#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/inet.h>
#include "/usr/src/linux-4.3/drivers/net/wireless/ath/ath9k/ath9k.h"

static struct nf_hook_ops nfho;
float rates[12] = {1.0, 2.0, 5.5, 11.0, 6.0, 9.0, 12.0, 18.0, 24.0, 36.0, 48.0, 54.0};
int p_recv[4] = {0};
float p_rates[4] = {0};
extern struct ath_softc* Pointer;

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

    /* ----- Print all needed information from received UDP packet ------ */
	int array[18]={0};
	int k=0;
	int j=0;
    /* Print UDP packet data (payload) */
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
    t_puts[0] = p_rates[0]*p_recv[0]/packetsMax;
    for (j=1; j < 4; j++)
	t_puts[j] = p_rates[j]*p_recv[j]/10;
    int max = 0;
    for (j=1; j<4; j++)
    {
	if (t_puts[j]>t_puts[max])
		max = j;
    }
    double best_rate = p_rates[max];

    printk("The rate selected is: %d\n",best_rate);

    Pointer->PacketsSent = 0;
    Pointer->DefaultMultiCastRate = max;
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
