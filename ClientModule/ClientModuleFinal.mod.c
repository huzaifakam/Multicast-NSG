#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xb915c50, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x837850eb, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0x675964e6, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0xe49c4d35, __VMLINUX_SYMBOL_STR(dev_queue_xmit) },
	{ 0x7d50a24, __VMLINUX_SYMBOL_STR(csum_partial) },
	{ 0x88e1be1f, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0xacbc0123, __VMLINUX_SYMBOL_STR(skb_put) },
	{ 0x2e60bace, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xc2e89bd1, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0xb4e26f28, __VMLINUX_SYMBOL_STR(dev_get_by_name) },
	{ 0x5373df5c, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0xd0d8621b, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "39F4E8430BB8C7397CA8556");
