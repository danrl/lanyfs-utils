/*
 * detectfs.c - Detect Lanyard Filesystem.
 *
 * Copyright 2012 Dan Luedtke <mail@danrl.de>
 * Copyright 2012 Philippe Michaud-Boudreault <pitwuu@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU General Public License Version 2 or any later version,
 * in which case the provisions of the GPL are required INSTEAD OF the above
 * restrictions.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* -------------------------------------------------------------------------- */

/**
 * DOC: Caution
 *
 * This programm works well on big and little endian architectures.
 * I have not tried with any other byte sex. However, functions
 * are already there. Just fill in the correct code for mixed endian
 * or whatever you like (have) to use.
 *
 * The functions are named tole16(), fromle16(), tole64() and fromle64().
 * See their documenation for further details.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>		/* va_*(), vprintf() */
#ifdef __FreeBSD__
#include <sys/endian.h>
#define bswap_16(x) __bswap16(x)
#define bswap_64(x) __bswap64(x)
#define off64_t off_t
#define fseeko64 fseeko
#define ftello64 ftello
#elif defined __APPLE__
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_64(x) OSSwapInt64(x)
#define off64_t off_t
#define fseeko64 fseeko
#define ftello64 ftello
#else
#include <byteswap.h>
#endif

#include "lanyfs.h"

 /* gettext support, e.g. for i18n */
#ifdef	HAVE_GETTEXT
#define	_(s)	gettext(s)
#else
#define	_(s)	s
#endif

/* constants */
const char *progname = "detectfs.lanyfs";
const char *progdate = "December 2012";

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
 */
static inline uint16_t tole16 (uint16_t n)
{
	int i = 1;
	if (!*((unsigned char *) &i) == 0x01)
		n =  bswap_16(n);
	return n;
}

/**
 * fromle16() - Convert from little endian to CPU endianess.
 * @n:				unsigned integer to convert
 */
static inline uint16_t fromle16 (uint16_t n)
{
	int i = 1;
	if (!*((unsigned char *) &i) == 0x01)
		n =  bswap_16(n);
	return n;
}

/**
 * tole64() - Convert from CPU endianess to little endian.
 * @n:				unsigned integer to convert
 */

static inline uint64_t tole64 (uint64_t n)
{
	int i = 1;
	if (!*((unsigned char *) &i) == 0x01)
		n =  bswap_64(n);
	return n;
}

/**
 * fromle64() - Convert from little endian to CPU endianess.
 * @n:				unsigned integer to convert
 */
static inline uint64_t fromle64 (uint64_t n)
{
	int i = 1;
	if (!*((unsigned char *) &i) == 0x01)
		n =  bswap_64(n);
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
	printf(_("root dir: %"PRIu64"\n"), sb->rootdir);
	printf(_("total blocks: %"PRIu64"\n"), sb->blocks);
	printf(_("free head: %"PRIu64"\n"), sb->freehead);
	printf(_("free tail: %"PRIu64"\n"), sb->freetail);
	printf(_("free blocks: %"PRIu64"\n"), sb->freeblocks);
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
	printf(_("badblocks%"PRIu64"\n"), sb->badblocks);
	printf(_("volume label: %s\n"), sb->label);

	free_null(sb);
	return EXIT_SUCCESS;
}
