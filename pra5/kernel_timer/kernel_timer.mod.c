#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x8a2e525e, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x37a0cba, "kfree" },
	{ 0x80dd3a3, "kmem_cache_alloc_trace" },
	{ 0xf6aa025b, "kmalloc_caches" },
	{ 0x86cb7b28, "init_timer_key" },
	{ 0xec95baea, "__register_chrdev" },
	{ 0x37e74642, "get_jiffies_64" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x22d88e1d, "add_timer" },
	{ 0x7d11c268, "jiffies" },
	{ 0xf7b574c2, "del_timer_sync" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x27e1a049, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

