/*
 * mkfs.c - Make Lanyard Filesystem.
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

#include <stddef.h>		/* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>		/* va_*(), vprintf() */
#include <string.h>
#include <unistd.h>		/* getopt() */
#include <time.h>		/* timestamp creation */
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

/* defaults and limitations of mkfs.lanyfs */
#define MKLANYFS_LABEL		"LanyFS Storage"
#define MKLANYFS_BLOCKSIZE	12
#define MKLANYFS_ADDRLEN	4
#define MKLANYFS_MIN_BLOCKS	16
#define MKLANYFS_ROOTDIR	"LANYFSROOT"

/* constants */
const char *progname = "mkfs.lanyfs";
const char *progdate = "December 2012";

/* global variables */
int v = 0;

/* data types */
/**
 * enum mklanyfs_time - Point in time.
 * @TS_NULL:			undefined point in time
 * @TS_NOW:			current time
 */
enum mklanyfs_time {
	TS_NULL,
	TS_NOW,
};

/**
 * struct mklanyfs_cfg - Configuration set.
 * @blocksize:			blocksize in bytes
 * @addrlen:			address length in bytes
 * @vol_label:			volume label
 * @dev_name:			device path
 * @dev_fp:			filepointer for device
 * @dev_bytes:			size of device in bytes
 * @dev_blocks:			number of blocks the device can hold
 * @dev_overhead:		number of unused bytes after last block
 *
 * Configuration set for a target device.
 */
struct mklanyfs_cfg {
	int			blocksize;
	int			addrlen;
	char			*vol_label;
	char			*dev_name;
	FILE			*dev_fp;
	uint64_t		dev_bytes;
	uint64_t		dev_blocks;
	int			dev_overhead;
};

/**
 * struct mklanyfs_b - Allocated block.
 * @addr:			block's on-disk address
 * @b:				the block itself
 *
 * Stores an allocated block in memory.
 */
struct mklanyfs_b {
	uint64_t			addr;
	union {
		struct lanyfs_raw	raw;
		struct lanyfs_sb	sb;
		struct lanyfs_dir	dir;
		struct lanyfs_chain	chain;
		unsigned char		blob[(1 << LANYFS_MAX_BLOCKSIZE)];
	} b;
};

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
 * intlog2() - Simple logarithm check.
 * @n:				value to check
 */
static int intlog2 (unsigned int n)
{
	int b = 0;
	while (n) {
		if (n & 1) {
			if (n > 1)
				return -1;
			return b;
		}
		n >>= 1;
		b++;
	}
	return -1;
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
	fprintf(stderr,
		_("usage: %s"
		  " [-v] [-l label] [-b blocksize] [-a address length]"
		  " device\n"),
		progname);
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

/**
 * verbose() - Prints verbose message.
 * @fmt:			verbose message format string
 * @...:			format string arguments
 */
static void verbose (const char *fmt, ...)
{
	va_list arg;
	if (v) {
		printf(_("info: "));
		va_start(arg, fmt);
		vprintf(fmt, arg);
		va_end(arg);
		printf("\n");
	}
}

/**
 * make_timestamp() - Crafts a timestamp in LanyFS format.
 * @t:				point in time of requested timestamp
 */
static struct lanyfs_ts make_timestamp (enum mklanyfs_time t) {
	struct lanyfs_ts ts;
	time_t tnow;
	struct tm *tm;

	ts.year = 0;
	ts.mon = 0;
	ts.day = 0;
	ts.hour = 0;
	ts.min = 0;
	ts.sec = 0;
	ts.nsec = 0;
	ts.offset = 0;
	if (t == TS_NOW) {
		tnow = time(NULL);
		tm = gmtime(&tnow);
		ts.year = tm->tm_year + 1900;
		ts.mon = tm->tm_mon + 1;
		ts.day = tm->tm_mday;
		ts.hour = tm->tm_hour;
		ts.min = tm->tm_min;
		ts.sec = tm->tm_sec;
		tm = localtime(&tnow);
		ts.offset = (tm->tm_gmtoff / 60);
	}
	return ts;
}

/**
 * open_device() - Opens a device (or file) for formatting.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 */
static void open_device (struct mklanyfs_cfg *cfg)
{
	cfg->dev_fp = fopen(cfg->dev_name, "rb+");
	if (cfg->dev_fp) {
		fflush(cfg->dev_fp);
		rewind(cfg->dev_fp);
		fseeko64(cfg->dev_fp, 0LL, SEEK_END);
		cfg->dev_bytes = (uint64_t) ftello64(cfg->dev_fp);
		rewind(cfg->dev_fp);
		cfg->dev_overhead = cfg->dev_bytes % (1 << cfg->blocksize);
		cfg->dev_blocks = cfg->dev_bytes / (1 << cfg->blocksize);
	}
}

/**
 * close_device() - Closes a device (or file).
 * @cfg:			configuration set containing device
 * 				information and user defined options
 */
static void close_device (struct mklanyfs_cfg *cfg)
{
	fclose(cfg->dev_fp);
	cfg->dev_fp = NULL;
}

/**
 * flush_block() - Writes a block from memory to target device.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @b:				block to be written
 */
static int flush_block (struct mklanyfs_cfg *cfg, struct mklanyfs_b *b)
{
	off64_t pos;
	if (!cfg || !b || !cfg->dev_fp)
		return EXIT_FAILURE;
	verbose("write block addr=%"PRIu64" type=0x%x", b->addr, b->b.raw.type);
	pos = b->addr * (1 << cfg->blocksize);
	if (fseeko(cfg->dev_fp, pos, SEEK_SET) != 0) {
		show_error("seek error at block %"PRIu64, b->addr);
	}
	b->b.raw.wrcnt = tole16(fromle16(b->b.raw.wrcnt) + 1);
	if (fwrite(b->b.blob, 1 << cfg->blocksize, 1, cfg->dev_fp) != 1) {
		show_error("write error at block %"PRIu64, b->addr);
	}
	return EXIT_SUCCESS;
}

/**
 * allocate_superblock() - Allocates a superblock in memory.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @addr:			on-disk target address of block
 */
static struct mklanyfs_b *allocate_superblock (struct mklanyfs_cfg *cfg,
					       uint64_t addr)
{
	struct mklanyfs_b *b;
	if (!cfg)
		return NULL;
	verbose("allocating superblock at addr=%"PRIu64, addr);
	b = calloc(1, sizeof(*b));
	if (b) {
		b->addr = addr;
		b->b.sb.type = LANYFS_TYPE_SB;
		b->b.sb.wrcnt = 0;
		b->b.sb.major = LANYFS_MAJOR_VERSION;
		b->b.sb.minor = LANYFS_MINOR_VERSION;
		b->b.sb.magic = LANYFS_SUPER_MAGIC;
		b->b.sb.blocksize = cfg->blocksize;
		b->b.sb.addrlen = cfg->addrlen;
		b->b.sb.rootdir = 0;
		b->b.sb.blocks = cfg->dev_blocks;
		b->b.sb.freehead = 0;
		b->b.sb.freetail = 0;
		b->b.sb.freeblocks = 0;
		b->b.sb.created = b->b.sb.updated = make_timestamp(TS_NOW);
		b->b.sb.checked = make_timestamp(TS_NULL);
		b->b.sb.badblocks = 0;
		strncpy(b->b.sb.label, cfg->vol_label, LANYFS_NAME_LENGTH);
	}
	return b;
}

/**
 * allocate_rootdir() - Allocates the root directory block in memory.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @addr:			on-disk target address of block
 */
static struct mklanyfs_b *allocate_rootdir (struct mklanyfs_cfg *cfg,
					    uint64_t addr)
{
	struct mklanyfs_b *b;
	if (!cfg)
		return NULL;
	verbose("allocating root directory at addr=%"PRIu64, addr);
	b = calloc(1, sizeof(*b));
	if (b) {
		b->addr = addr;
		b->b.dir.type = LANYFS_TYPE_DIR;
		b->b.dir.wrcnt = 0;
		b->b.dir.btree.left = 0;
		b->b.dir.btree.right = 0;
		b->b.dir.subtree = 0;
		b->b.dir.meta.created = b->b.dir.meta.modified \
				      = make_timestamp(TS_NOW);
		b->b.dir.meta.attr = 0;
		strncpy(b->b.dir.meta.name, MKLANYFS_ROOTDIR,
			LANYFS_NAME_LENGTH);
	}
	return b;
}

/**
 * allocate_chain() - Allocates a chain block in memory.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @addr:			on-disk target address of block
 */
static struct mklanyfs_b *allocate_chain (struct mklanyfs_cfg *cfg,
					  uint64_t addr)
{
	struct mklanyfs_b *b;
	if (!cfg)
		return NULL;
	verbose("allocating chain block at addr=%"PRIu64, addr);
	b = calloc(1, sizeof(*b));
	if (b) {
		b->addr = addr;
		b->b.chain.type = LANYFS_TYPE_EXT;
		b->b.chain.wrcnt = 0;
		b->b.chain.next = 0;
	}
	return b;
}

/**
 * chain_count_slots() - Returns the number of slots of extender blocks.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 */
static int chain_count_slots (struct mklanyfs_cfg *cfg)
{
	int slots = 0;
	if (cfg) {
		slots = (1 << cfg->blocksize);
		slots -= offsetof(struct lanyfs_chain, stream);
		slots /= cfg->addrlen;
	}
	return slots;
}

/**
 * chain_get_slot() - Returns the address stored in slot.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @b:				extender block
 * @slot:			slot to be read
 */
static uint64_t chain_get_slot (struct mklanyfs_cfg *cfg, struct mklanyfs_b *b,
				unsigned int slot)
{
	uint64_t addr;
	if (!cfg || !b)
		return 0;
	if (slot >= chain_count_slots(cfg))
		return 0;
	/*
	 * this works fine on little endian machines,
	 * remember to swap bytes before running this
	 * code on big endian ones!
	 */
	addr = 0;
	memcpy(&addr, &b->b.chain.stream + (slot * cfg->addrlen), cfg->addrlen);
	return fromle64(addr);
}

/**
 * chain_get_free_slot() - Returns next free slot in an extender.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @b:				extender block
 */
static int chain_get_free_slot (struct mklanyfs_cfg *cfg, struct mklanyfs_b *b)
{
	for (int i = 0; i < chain_count_slots(cfg); i++) {
		if (!chain_get_slot(cfg, b, i))
			return i;
	}
	verbose("chain block at addr=%"PRIu64" no free slot", b->addr);
	return -1;
}

/**
 * chain_set_slot() - Sets next free slot of extender to given address.
 * @cfg:			configuration set containing device
 * 				information and user defined options
 * @b:				extender block
 * @addr:			target address
 */
static int chain_set_slot (struct mklanyfs_cfg *cfg, struct mklanyfs_b *b,
			   uint64_t addr)
{
	int slot;
	if (!cfg || !b)
		return -1;
	slot = chain_get_free_slot(cfg, b);
	if (slot < 0)
		return -1;
	verbose("chain block at addr=%"PRIu64" slot=%d target=%"PRIu64, b->addr, slot, addr);
	addr = tole64(addr);
	memcpy(&b->b.chain.stream + (slot * cfg->addrlen), &addr, cfg->addrlen);
	return 0;
}

/* -------------------------------------------------------------------------- */

int main (int argc, char *argv[])
{
	/* essentials */
	struct mklanyfs_b *super;
	struct mklanyfs_b *root;
	struct mklanyfs_b *chain;
	uint64_t current = LANYFS_SUPERBLOCK + 1;
	uint64_t freeblocks = 0;

	/* fill configuration set with defaults */
	struct mklanyfs_cfg cfg;
	cfg.blocksize = MKLANYFS_BLOCKSIZE;
	cfg.addrlen = MKLANYFS_ADDRLEN;
	cfg.vol_label = MKLANYFS_LABEL;

	show_version();
	/* parse command line options */
	int c;
	while ((c = getopt(argc, argv, "a:b:l:v")) != -1) {
		int tmp;
		switch (c) {
		case 'a':
			tmp = atoi(optarg);
			if (tmp >= (LANYFS_MIN_ADDRLEN * 8) &&
			    tmp <= (LANYFS_MAX_ADDRLEN * 8) &&
			    !(tmp % 8)) {
				cfg.addrlen = tmp / 8;
			} else {
				show_error(_("invalid address length"));
			}
			break;
		case 'b':
			tmp = intlog2(atoi(optarg));
			if (tmp >= LANYFS_MIN_BLOCKSIZE &&
			    tmp <= LANYFS_MAX_BLOCKSIZE) {
				cfg.blocksize = tmp;
			} else {
				show_error(_("invalid blocksize"));
				exit(EXIT_FAILURE);
			}
			break;
		case 'l':
			cfg.vol_label = optarg;
			break;
		case 'v':
			v = 1;
			break;
		default:
			show_usage();
			break;
		}
	}
	if (optind < argc) {
		cfg.dev_name = argv[optind];
	} else {
		show_usage();
	}

	/* open device */
	open_device(&cfg);
	if (cfg.dev_fp == NULL) {
		show_error(_("error opening device %s"), cfg.dev_name);
	}
	if (cfg.dev_blocks < MKLANYFS_MIN_BLOCKS) {
		show_error(_("device %s fits less than %d blocks"),
			   cfg.dev_name, MKLANYFS_MIN_BLOCKS);
	}

	/* show configuration */
	printf(_("address length: %d bit\n"), cfg.addrlen * 8);
	printf(_("blocksize: %d bytes\n"), 1 << cfg.blocksize);
	printf(_("volume label: %s\n"), cfg.vol_label);
	if (cfg.dev_blocks > (uint64_t) 1 << (cfg.addrlen * 8)) {
		printf(_("warning: address length not sufficient!\n"));
		cfg.dev_blocks = (uint64_t) 1 << (cfg.addrlen * 8);
	}
	if (cfg.dev_overhead) {
		printf(_("info: device has %d bytes overhead\n"),
		       cfg.dev_overhead);
	}

	/* write superblock */
	super = allocate_superblock(&cfg, LANYFS_SUPERBLOCK);
	if (!super) {
		show_error("error allocating superblock");
	}
	printf(_("writing superblock\n"));
	flush_block(&cfg, super);

	/* TODO: write badblocks chain */
	super->b.sb.badblocks = 0;

	/* create root directory */
	root = allocate_rootdir(&cfg, current++);
	if (!root) {
		show_error("error allocating root directory");
	}
	printf(_("creating root directory\n"));
	flush_block(&cfg, root);
	super->b.sb.rootdir = root->addr;
	free_null(root);

	/* write first chain block for free blocks */
	chain = allocate_chain(&cfg, current++);
	if (!chain) {
		show_error("error allocating chain");
	}
	super->b.sb.freehead = chain->addr;
	freeblocks++;

	/* map remaining free space */
	printf(_("mapping free space\n"));
	printf(_("\r\t%"PRIu64"/%"PRIu64), current, cfg.dev_blocks);
	fflush(stdout);
	while (current < cfg.dev_blocks) {
		if (chain_get_free_slot(&cfg, chain) < 0) {
			chain->b.chain.next = current;
			flush_block(&cfg, chain);
			free_null(chain);
			chain = allocate_chain(&cfg, current++);
			if (!chain)
				show_error("error allocating chain");
			freeblocks++;
			printf(_("\r\t%"PRIu64"/%"PRIu64), current, cfg.dev_blocks);
			if (v)
				printf("\n");
			continue;
		}
		chain_set_slot(&cfg, chain, current++);
		freeblocks++;
		if (v)
			printf(_("\r\t%"PRIu64"/%"PRIu64"\n"), current, cfg.dev_blocks);
	}
	printf(_("\r\t%"PRIu64"/%"PRIu64"\n"), current, cfg.dev_blocks);
	super->b.sb.freetail = chain->addr;
	super->b.sb.freeblocks = freeblocks;
	flush_block(&cfg, chain);
	free_null(chain);

	/* update superblock */
	printf(_("updating superblock\n"));
	super->b.sb.updated = make_timestamp(TS_NOW);
	flush_block(&cfg, super);
	free_null(super);

	/* close device */
	close_device(&cfg);

	printf(_("all done\n"));
	return EXIT_SUCCESS;
}
