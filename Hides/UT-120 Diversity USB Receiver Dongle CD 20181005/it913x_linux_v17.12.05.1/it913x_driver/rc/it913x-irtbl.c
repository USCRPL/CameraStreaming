/*
 * This utility creates the "it913xirtbl.bin" file.
 * Afterwards copy it as "/lib/firmware/it913xirtbl.bin".
 *
 * There exist two standards for the "it913xirtbl.bin" file.
 *
 * 1. The "legacy" binary standard - the file size is 356 bytes (= 50 * 7 + 6).
 *    It allows for a maximum of 50 entries (one entry corresponds to a single
 *    "IR remote" key and occupies 7 bytes in the output file).
 *    The last 6 bytes of the output file are:
 *    - 2 bytes for the "Toggle Mask" (RC6-like protocol only, default = 0x7fff)
 *    - 1 byte for the total number of written entries,
 *    - 1 byte for the "Fn Expire Time" (default = 62, the "time unit" is 20ms,
 *      so 62 * 20ms = 1.24s),
 *    - 1 byte for the "Repeat Period" (default = 25, the "time unit" is 20ms,
 *      so 25 * 20ms = 0.5s),
 *    - 1 byte for the "IR Remote Type" (default = 0).
 *
 * 2. The "version 2.0" binary standard - the details are unknown yet.
 */

#include "it913x-irtbl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* global definitions */

#define IT913x_IRTBL_LEGACY_FILE_NAME "it913xirtbl.bin"
#define IT913x_IRTBL_LEGACY_MAX_ENTRIES 50

/* global vriables */

int verbose;

__u16 Toggle_Mask = 0x7fff;
__u8 Fn_Expire_Time = 62;
__u8 Repeat_Period = 25;
__u8 IR_Remote_Type;

/* utility functions and macros */

void write_legacy_it913x_irtbl_entry(rc_code_entry key, FILE *f)
{
	long int l;
	__u8 tmp;

	if (!f)
		return;		/* just a precaution */

	l = ftell(f);
	if ((l < 0) || (((IT913x_IRTBL_LEGACY_MAX_ENTRIES) * 7) < (l + 7)))
		return;

	/* 4 bytes for the IR scancode */
	if (((key.scancode) >> 24) == 0x80) {
		tmp = ((key.scancode) >> 24);
		fwrite(&tmp, 1, 1, f);
		tmp = ((key.scancode) >> 16);
		fwrite(&tmp, 1, 1, f);
		tmp = ((key.scancode) >> 8);
		fwrite(&tmp, 1, 1, f);
		tmp = (key.scancode);
		fwrite(&tmp, 1, 1, f);
	} else {
		tmp = ((key.scancode) >> 16);
		fwrite(&tmp, 1, 1, f);
		tmp = ((key.scancode) >> 8);
		fwrite(&tmp, 1, 1, f);
		tmp = (key.scancode);
		fwrite(&tmp, 1, 1, f);
		tmp = ((key.scancode) ? (tmp ^ 0xff) : 0);
		fwrite(&tmp, 1, 1, f);
	}

	/* two bytes for the HID keycode */
	tmp = (key.keycode);
	fwrite(&tmp, 1, 1, f);
	tmp = ((key.keycode) >> 8);
	fwrite(&tmp, 1, 1, f);

	/* one byte = always zero ? some kind of a "mask" ? */
	tmp = 0;
	fwrite(&tmp, 1, 1, f);

	return;
}

#define WRITE_LEGACY_IT913x_IRTBL_TABLE(TABLE, FILE)			\
	do {								\
		size_t i;						\
		if (!(FILE))					\
			continue; /* just a precaution */		\
		for (i = 0; i < (sizeof((TABLE)) / sizeof(rc_code_entry)); i++)\
			write_legacy_it913x_irtbl_entry((TABLE)[i], (FILE));\
	} while (0)

void finalize_legacy_it913x_irtbl_file(FILE *f)
{
	long int l, n;
	__u8 tmp;

	if (!f)
		return;		/* just a precaution */

	l = ftell(f);
	if ((l < 0) || (l > ((IT913x_IRTBL_LEGACY_MAX_ENTRIES) * 7))) {
		fprintf(stderr, "Error: incorrect file size : %ld\n", l);
		return;		/* just a precaution */
	}

	n = l / 7;		/* the total number of written entries */

	if (l != (n * 7)) {
		fprintf(stderr, "Error: incorrect file size : %ld\n", l);
		return;		/* just a precaution */
	}

	/* fill to IT913x_IRTBL_LEGACY_MAX_ENTRIES entries */
	tmp = 0;
	for (l = l; l < ((IT913x_IRTBL_LEGACY_MAX_ENTRIES) * 7); l++)
		fwrite(&tmp, 1, 1, f);

	/* write the last 6 bytes */
	tmp = (Toggle_Mask >> 8);
	fwrite(&tmp, 1, 1, f);
	tmp = Toggle_Mask;
	fwrite(&tmp, 1, 1, f);
	tmp = n;
	fwrite(&tmp, 1, 1, f);
	tmp = Fn_Expire_Time;
	fwrite(&tmp, 1, 1, f);
	tmp = Repeat_Period;
	fwrite(&tmp, 1, 1, f);
	tmp = IR_Remote_Type;
	fwrite(&tmp, 1, 1, f);

	return;
}

void print_help(void)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "it913xirtbl [options] [IR_scancode_type ...]\n");

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-m NUM (default %#x)\n", Toggle_Mask);
	fprintf(stderr, "       Toggle Mask (RC6-like protocol only)\n");
	fprintf(stderr, "-t NUM (default %u)\n", Fn_Expire_Time);
	fprintf(stderr, "       Fn Expire Time (in 20ms units)\n");
	fprintf(stderr, "-p NUM (default %u)\n", Repeat_Period);
	fprintf(stderr, "       Repeat Period (in 20ms units)\n");
	fprintf(stderr, "-r NUM (default %u)\n", IR_Remote_Type);
	fprintf(stderr, "       IR Remote Type (does it really matter?)\n");
	fprintf(stderr, "-v     verbose screen output\n");
	fprintf(stderr, "-h     print this help\n");

	fprintf(stderr, "Known `IR scancode types':\n");
	fprintf(stderr, "1 = RC6 protocol remote controller\n");
	fprintf(stderr, "2 = NEC protocol remote controller\n");

	return;
}

/* main program */

int main(int argc, char *argv[])
{
	FILE *f = ((FILE *) 0);
	int c, i, itmp;

	/* parse options */
	opterr = 0;
	while ((c = getopt(argc, argv, "hvm:t:p:r:")) != -1)
		switch (c) {
		case 'h':
			print_help();
			return 0;
		case 'v':
			verbose = 1;
			break;
		case 'm':
			sscanf(optarg, "%x", &itmp);
			Toggle_Mask = itmp;
			break;
		case 't':
			sscanf(optarg, "%d", &itmp);
			Fn_Expire_Time = itmp;
			break;
		case 'p':
			sscanf(optarg, "%d", &itmp);
			Repeat_Period = itmp;
			break;
		case 'r':
			sscanf(optarg, "%d", &itmp);
			IR_Remote_Type = itmp;
			break;
		case '?':
			if ((optopt == 'm') || (optopt == 't') ||
			    (optopt == 'p') || (optopt == 'r'))
				fprintf(stderr,
					"Option -%c requires an argument.\n",
					optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n",
					optopt);
			else
				fprintf(stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			print_help();
			return 1;
		default:
			abort();
		}

	if (verbose)
		fprintf(stderr,
			"Toggle_Mask = %#x,\nFn_Expire_Time = %u,\nRepeat_Period = %u,\nIR_Remote_Type = %u\n",
			Toggle_Mask, Fn_Expire_Time, Repeat_Period,
			IR_Remote_Type);

	/* open the output file */
	f = fopen((IT913x_IRTBL_LEGACY_FILE_NAME), "w");
	if (!f) {
		fprintf(stderr,
			"Error: cannot create new %s file!\n",
			(IT913x_IRTBL_LEGACY_FILE_NAME));
		return 2;
	}

	/* loop over IR scancode types that the user wants to use */
	for (i = optind; i < argc; i++) {
		if (!(sscanf(argv[i], "%d", &itmp))) {
			fprintf(stderr, "Error: wrong IR scancode type : %s\n",
				argv[i]);
			continue;
		}

		switch (itmp) {
		case 1:
			WRITE_LEGACY_IT913x_IRTBL_TABLE(it913x_rc6, f);
			break;
		case 2:
			WRITE_LEGACY_IT913x_IRTBL_TABLE(it913x_nec, f);
			break;
		default:
			fprintf(stderr,
				"Error: IR scancode type %d not implemented.\n",
				itmp);
			break;
		}
	}

	/* store "the last 6 bytes" */
	finalize_legacy_it913x_irtbl_file(f);

	/* close the output file */
	if (f) {
		fclose(f);
		f = ((FILE *) 0);
	}

	return 0;
}
