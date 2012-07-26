/*
 * detectfs.c - Lanyard Filesystem detectfs
 *
 * Copyright (C) 2012  Dan Luedtke <mail@danrl.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* -------------------------------------------------------------------------- */

/**
 * DOC: Caution
 *
 * This programm works well on little endian architecture x86_64.
 * I have not tried with any other byte sex. However, function
 * protoypes for $your_architecture are already there. Just fill in
 * the correct code for big endian or mixed endian or whatever you
 * like (have) to use.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>		/* va_*(), vprintf() */

#include "lanyfs.h"

 /* gettext support, e.g. for i18n */
#ifdef	HAVE_GETTEXT
#define	_(s)	gettext(s)
#else
#define	_(s)	s
#endif

/* constants */
const char *progname = "detectfs.lanyfs";
const char *progdate = "July 2012";

/* -------------------------------------------------------------------------- */

/**
 * free_null() - Frees memory and sets pointer to NULL.
 * @p:				pointer to dynamically allocated memory
 *
 * This is a preprocessor macro-function.
 */
#define free_null(p)		\
	do {			\
		free(p);	\
		p = NULL;	\
	} while (0)

/**
 * tole16() - Convert from CPU endianess to little endian.
 * @n:				unsigned integer to convert
 *
 * Insert the corresponding code into this empty function when porting! This
 * function works fine on little endian machines, e.g. x86_64 without any
 * changes. Only 16 bit unsigned integers are passed to this function, but
 * signedness should not really matter.
 */
static inline uint16_t tole16 (uint16_t n)
{
	/*
	 * this works fine on little endian machines,
	 * remember to swap bytes before running this
	 * code on other machines!
	 */
	return n;
}

/**
 * fromle16() - Convert from little endian to CPU endianess.
 * @n:				unsigned integer to convert
 *
 * Insert the corresponding code into this empty function when porting! This
 * function works fine on little endian machines, e.g. x86_64 without any
 * changes. Only 16 bit unsigned integers are passed to this function, but
 * signedness should not really matter.
 */
static inline uint16_t fromle16 (uint16_t n)
{
	/*
	 * this works fine on little endian machines,
	 * remember to swap bytes before running this
	 * code on other machines!
	 */
	return n;
}

/**
 * tole64() - Convert from CPU endianess to little endian.
 * @n:				unsigned integer to convert
 *
 * Insert the corresponding code into this empty function when porting! This
 * function works fine on little endian machines, e.g. x86_64 without any
 * changes. Only 64 bit unsigned integers are passed to this function, but
 * signedness should not really matter.
 */

static inline uint64_t tole64 (uint64_t n)
{
	/*
	 * this works fine on little endian machines,
	 * remember to swap bytes before running this
	 * code on other machines!
	 */
	return n;
}

/**
 * fromle64() - Convert from little endian to CPU endianess.
 * @n:				unsigned integer to convert
 *
 * Insert the corresponding code into this empty function when porting! This
 * function works fine on little endian machines, e.g. x86_64 without any
 * changes. Only 64 bit unsigned integers are passed to this function, but
 * signedness should not really matter.
 */
static inline uint64_t fromle64 (uint64_t n)
{
	/*
	 * this works fine on little endian machines,
	 * remember to swap bytes before running this
	 * code on other machines!
	 */
	return n;
}

/**
 * show_version() - Prints the program's name and version.
 */
static void show_version (void)
{
	fprintf(stderr, "%s v%d.%d (%s)\n", progname, LANYFS_MAJOR_VERSION,
		LANYFS_MINOR_VERSION, _(progdate));
}

/**
 * show_usage() - Prints usage hints.
 */
static void show_usage (void)
{
	fprintf(stderr, _("usage: %s device\n"), progname);
	exit(EXIT_FAILURE);
}

/**
 * show_error() - Prints error message to stderr and exits program.
 * @fmt:			error message format string
 * @...:			format string arguments
 */
static void show_error (const char *fmt, ...)
{
	va_list arg;
	fprintf(stderr, "%s: ", progname);
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------- */

#define DETECTFS_SB_SIZE	512

int main (int argc, char *argv[])
{
	/* variables */
	FILE *fp;
	int i;
	struct lanyfs_sb *sb;

	show_version();
	/* parse command line options */
	if (argc < 2)
		show_usage();

	/* open device */
	fp = fopen(argv[1], "rb");
	if (!fp)
		show_error(_("error opening device %s"), argv);

	/* read superblock */
	sb = malloc(DETECTFS_SB_SIZE);
	for (i = 0; i < DETECTFS_SB_SIZE; i++)
		((unsigned char *) sb)[i] = fgetc(fp);

	/* close device */
	fclose(fp);

	/* show configuration */
	printf(_("blocktype: 0x%x\n"), sb->type);
	if (sb->type != LANYFS_TYPE_SB)
		show_error(_("block type mismatch"));
	printf(_("write counter: %u\n"), sb->wrcnt);
	printf(_("magic: 0x%x\n"), sb->magic);
	if (sb->magic != LANYFS_SUPER_MAGIC)
		show_error(_("magic mismatch"));
	printf(_("version: %u.%u\n"), sb->major, sb->minor);
	printf(_("address length: %d bit\n"), sb->addrlen * 8);
	printf(_("blocksize: %d bytes\n"), 1 << sb->blocksize);
	printf(_("root dir: %lu\n"), sb->rootdir);
	printf(_("total blocks: %lu\n"), sb->blocks);
	printf(_("free head: %lu\n"), sb->freehead);
	printf(_("free tail: %lu\n"), sb->freetail);
	printf(_("free blocks: %lu\n"), sb->freeblocks);
	printf(_("created: %04u-%02u-%02uT%02u:%02u:%02u.%u%+03d:%02d\n"),
		sb->created.year, sb->created.mon, sb->created.day,
		sb->created.hour, sb->created.min, sb->created.sec,
		sb->created.nsec, sb->created.offset / 60,
		abs(sb->created.offset % 60));
	printf(_("updated: %04u-%02u-%02uT%02u:%02u:%02u.%u%+03d:%02d\n"),
		sb->updated.year, sb->updated.mon, sb->updated.day,
		sb->updated.hour, sb->updated.min, sb->updated.sec,
		sb->updated.nsec, sb->updated.offset / 60,
		abs(sb->updated.offset % 60));
	printf(_("checked: %04u-%02u-%02uT%02u:%02u:%02u.%u%+03d:%02d\n"),
		sb->checked.year, sb->checked.mon, sb->checked.day,
		sb->checked.hour, sb->checked.min, sb->checked.sec,
		sb->checked.nsec, sb->checked.offset / 60,
		abs(sb->checked.offset % 60));
	printf(_("badblocks: %lu\n"), sb->badblocks);
	printf(_("volume label: %s\n"), sb->label);

	free_null(sb);
	return EXIT_SUCCESS;
}
