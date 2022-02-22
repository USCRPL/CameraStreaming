
#include <media/rc-map.h>

static struct rc_map_table nec[] = {

	{0x20cf1b, KEY_0},
	{0x20cf01, KEY_1},
	{0x20cf02, KEY_2},
	{0x20cf03, KEY_3},
	{0x20cf04, KEY_4},
	{0x20cf05, KEY_5},
	{0x20cf06, KEY_6},
	{0x20cf07, KEY_7},
	{0x20cf08, KEY_8},
	{0x20cf09, KEY_9},

	{0x20cf12, KEY_Q},
	{0x20cf1a, KEY_M},
	{0x20cf0d, KEY_F},
	{0x20cf1f, KEY_PAGEUP},
	{0x20cf0e, KEY_PAGEDOWN},
	{0x20cf0a, KEY_V},
	{0x20cf0c, KEY_PREVIOUS},

};

static struct rc_map_list nec_map = {
	.map = {
		.scan = nec,
		.size = ARRAY_SIZE(nec),
		.rc_type = RC_TYPE_NEC,
		.name = "NEC",
		}
};

static int __init init_rc_map_nec(void)
{
	return rc_map_register(&nec_map);
}

static void __exit exit_rc_map_nec(void)
{
	rc_map_unregister(&nec_map);
}

module_init(init_rc_map_nec)
module_exit(exit_rc_map_nec)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ITE");
