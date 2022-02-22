/*
  This utility creates the "it913xirtbl.bin" file.
  Afterwards copy it as "/lib/firmware/it913xirtbl.bin".
*/

#include <linux/input.h>

/*
  The keys/buttons are modeled after USB HUT 1.12:
  http://www.usb.org/developers/hidpage
  http://en.wikipedia.org/wiki/Human_interface_device
  (See also "/usr/src/linux-source-x.y.z/drivers/hid/hid-input.c")
  The "high byte" contains key modifiers (bit 0 specifies Control,
  bit 1 specifies Shift, bit 2 specifies Alt and bit 3 specifies
  the Windows key).
*/

/* native HID keys */

#define HID_KEY_1 0x001e	/* 1 */
#define HID_KEY_2 0x001f	/* 2 */
#define HID_KEY_3 0x0020	/* 3 */
#define HID_KEY_4 0x0021	/* 4 */
#define HID_KEY_5 0x0022	/* 5 */
#define HID_KEY_6 0x0023	/* 6 */
#define HID_KEY_7 0x0024	/* 7 */
#define HID_KEY_8 0x0025	/* 8 */
#define HID_KEY_9 0x0026	/* 9 */
#define HID_KEY_0 0x0027	/* 0 */

#define HID_KEY_RETURN 0x0028	/* Return (keyboard ENTER) */
#define HID_KEY_ESC 0x0029	/* ESCAPE */
#define HID_KEY_DELETE 0x004C	/* DELETE */

#define HID_KEY_RIGHT 0x004f	/* RightArrow */
#define HID_KEY_LEFT 0x0050	/* LeftArrow */
#define HID_KEY_DOWN 0x0051	/* DownArrow */
#define HID_KEY_UP 0x0052	/* UpArrow */

#define HID_KEY_POWER 0x0066	/* Power */

#define HID_KEY_STOP 0x0078	/* Stop */

#define HID_KEY_MUTE 0x007f	/* Mute */
#define HID_KEY_VOLUMEUP 0x0080	/* VolumeUp */
#define HID_KEY_VOLUMEDOWN 0x081	/* VolumeDown */

/* simulated HID keys */

#define HID_KEY_CHANNELUP 0x004b	/* PageUp */
#define HID_KEY_CHANNELDOWN 0x004e	/* PageDown */

#define HID_KEY_VIDEO 0x0116	/* ctrl-s */
#define HID_KEY_ZOOM 0x0009	/* f */
#define HID_KEY_RECORD 0x004a	/* Home */
#define HID_KEY_PLAY 0x0111	/* ctrl-n */

#define HID_KEY_RED 0x0215	/* shift-r = R */
#define HID_KEY_GREEN 0x020a	/* shift-g = G */
#define HID_KEY_YELLOW 0x021c	/* shift-y = Y */
#define HID_KEY_BLUE 0x0205	/* shift-b = B */

#define HID_KEY_TEXT 0x0004	/* a */
#define HID_KEY_PAUSE 0x0017	/* t */
#define HID_KEY_REWIND 0x000c	/* i */
#define HID_KEY_FORWARD 0x0011	/* n */

#define HID_KEY_POWER2 0x0114	/* ctrl-q */

#define HID_KEY_Q 0x0014
#define HID_KEY_F 0x0009
#define HID_KEY_V 0x0219
#define HID_KEY_PREVIOUS 0x0019

#define HID_KEY_ON_OFF HID_KEY_POWER2
#define HID_KEY_SOURCE HID_KEY_VIDEO
#define HID_KEY_SHUT_DOWN HID_KEY_POWER
#define HID_KEY_ENTER HID_KEY_RETURN

#define HID_KEY_EPG HID_KEY_BLUE
#define HID_KEY_LIVE_TV HID_KEY_YELLOW
#define HID_KEY_STEREO HID_KEY_RED
#define HID_KEY_SNAPSHOT HID_KEY_GREEN

#define HID_KEY_RECALL HID_KEY_ESC
#define HID_KEY_TELETEXT HID_KEY_TEXT

/*
  Note: all "IR scan codes" are entered in row-wise order (i.e. the keys
  from the "IR remote" top row first, then the keys from the next row,
  finally the keys from the "IR remote" bottom row). For each row, "IR
  scan codes" are entered in the left to right order.
*/

typedef struct {
	__u32 scancode;		/* IR scan code */
	__u32 keycode;		/* HID key code */
} rc_code_entry;

rc_code_entry it913x_rc6[] = {

	{0x800f0401, HID_KEY_1},
	{0x800f0402, HID_KEY_2},
	{0x800f0403, HID_KEY_3},
	{0x800f0404, HID_KEY_4},
	{0x800f0405, HID_KEY_5},
	{0x800f0406, HID_KEY_6},
	{0x800f0407, HID_KEY_7},
	{0x800f0408, HID_KEY_8},
	{0x800f0409, HID_KEY_9},
	{0x800f0400, HID_KEY_0},

	{0x800f040a, HID_KEY_DELETE},
	{0x800f040b, HID_KEY_RETURN},
	{0x800f040c, HID_KEY_POWER},
	{0x800f040d, HID_KEY_LEFT},
	{0x800f040e, HID_KEY_MUTE},

	{0x800f0410, HID_KEY_VOLUMEUP},
	{0x800f0411, HID_KEY_VOLUMEDOWN},
	{0x800f0412, HID_KEY_CHANNELUP},
	{0x800f0413, HID_KEY_CHANNELDOWN},

	{0x800f0414, HID_KEY_FORWARD},
	{0x800f0415, HID_KEY_REWIND},
	{0x800f0416, HID_KEY_PLAY},
	{0x800f0417, HID_KEY_RECORD},
	{0x800f0418, HID_KEY_PAUSE},
	{0x800f046e, HID_KEY_TEXT},
	{0x800f0419, HID_KEY_STOP},

	{0x800f041e, HID_KEY_UP},
	{0x800f041f, HID_KEY_DOWN},
	{0x800f0420, HID_KEY_LEFT},
	{0x800f0421, HID_KEY_RIGHT},
	{0x800f0423, HID_KEY_ESC},
	{0x800f0424, HID_KEY_VIDEO},
	{0x800f0426, HID_KEY_EPG},
	{0x800f0427, HID_KEY_ZOOM},
	{0x800f0432, HID_KEY_SOURCE},
	{0x800f0446, HID_KEY_LIVE_TV},
	{0x800f0447, HID_KEY_STEREO},
	{0x800f045b, HID_KEY_RED},
	{0x800f045c, HID_KEY_GREEN},
	{0x800f045d, HID_KEY_YELLOW},
	{0x800f045e, HID_KEY_BLUE},
	{0x800f0465, HID_KEY_POWER2},
	{0x800f046e, HID_KEY_PLAY},
	{0x800f0481, HID_KEY_PAUSE},

};

rc_code_entry it913x_nec[] = {

	{0x20cf12, HID_KEY_Q},
	{0x20cf1a, HID_KEY_MUTE},
	{0x20cf0d, HID_KEY_F},
	{0x20cf1f, HID_KEY_CHANNELUP},
	{0x20cf0e, HID_KEY_CHANNELDOWN},
	{0x20cf0a, HID_KEY_V},
	{0x20cf0c, HID_KEY_PREVIOUS},
	{0x20cf01, HID_KEY_1},
	{0x20cf02, HID_KEY_2},
	{0x20cf03, HID_KEY_3},
	{0x20cf04, HID_KEY_4},
	{0x20cf05, HID_KEY_5},
	{0x20cf06, HID_KEY_6},
	{0x20cf07, HID_KEY_7},
	{0x20cf08, HID_KEY_8},
	{0x20cf09, HID_KEY_9},
	{0x20cf1b, HID_KEY_0},

};
