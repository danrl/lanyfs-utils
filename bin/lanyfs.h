/*
 * lanyfs.h - Userspace Headers for Lanyard Filesystem.
 *
 * Copyright 2012 Dan Luedtke <mail@danrl.de>
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

#ifndef __LANYFS_H_
#define __LANYFS_H_

#include <inttypes.h>

/* filesystem magic bytes */
#define LANYFS_SUPER_MAGIC	0x594E414C

/* filesystem version */
#define LANYFS_MAJOR_VERSION	1
#define LANYFS_MINOR_VERSION	4

/* important numbers */
#define LANYFS_SUPERBLOCK	0	/* address of on-disk superblock */
#define LANYFS_MIN_ADDRLEN	1	/* mimimun address length (in bytes) */
#define LANYFS_MAX_ADDRLEN	8	/* maximum address length (in bytes) */
#define LANYFS_MIN_BLOCKSIZE	9	/* mimimun blocksize 2**9 */
#define LANYFS_MAX_BLOCKSIZE	12	/* maximum blocksize 2**12 */
#define LANYFS_NAME_LENGTH	256	/* maximum length of label/name */

/* block type identifiers */
#define LANYFS_TYPE_FREE	0x00	/* optional */
#define LANYFS_TYPE_DIR		0x10
#define LANYFS_TYPE_FILE	0x20
#define LANYFS_TYPE_CHAIN	0x70
#define LANYFS_TYPE_EXT		0x80
#define LANYFS_TYPE_DATA	0xA0	/* dor internal use only */
#define LANYFS_TYPE_SB		0xD0
#define LANYFS_TYPE_BAD		0xE0	/* optional */

/* directory and file attributes */
#define LANYFS_ATTR_NOWRITE	(1<<0)
#define LANYFS_ATTR_NOEXEC	(1<<1)
#define LANYFS_ATTR_HIDDEN	(1<<2)
#define LANYFS_ATTR_ARCHIVE	(1<<3)


/**
 * struct lanyfs_ts - ISO8601-like LanyFS timestamp.
 * @year:			gregorian year (0 to 9999)
 * @mon:			month of year (1 to 12)
 * @day:			day of month (1 to 31)
 * @hour:			hour of day (0 to 23)
 * @min:			minute of hour (0 to 59)
 * @sec:			second of minute (0 to 59 normal, 0 to 60 if
 * 				leap second)
 * @__reserved_0:		reserved
 * @nsec:			nanosecond (0 to 10^9)
 * @offset:			signed UTC offset in minutes
 * @__reserved_1:		reserved
 */
struct lanyfs_ts {
	uint16_t		year;
	uint8_t			mon;
	uint8_t			day;
	uint8_t			hour;
	uint8_t			min;
	uint8_t			sec;
	unsigned char		__reserved_0[1];
	uint32_t		nsec;
	int16_t			offset;
	unsigned char		__reserved_1[2];
};

/**
 * struct lanyfs_raw - LanyFS raw block.
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @data:			first byte of data
 */
struct lanyfs_raw {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	unsigned char		data;
};

/**
 * struct lanyfs_sb - LanyFS superblock.
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @major:			major version of filesystem
 * @__reserved_1:		reserved
 * @minor:			minor version of filesystem
 * @__reserved_2:		reserved
 * @magic:			identifies the filesystem
 * @blocksize:			blocksize (exponent to base 2)
 * @__reserved_3:		reserved
 * @addrlen:			length of block addresses in bytes
 * @__reserved_4:		reserved
 * @rootdir:			address of root directory block
 * @blocks:			number of blocks on the device
 * @freehead:			start of free blocks chain
 * @freetail:			end of free blocks chain
 * @freeblocks:			number of free blocks
 * @created:			date and time of filesystem creation
 * @updated:			date and time of last superblock field change
 * @checked:			date and time of last successful filesystem
 * 				check
 * @badblocks:			start of bad blocks chain
 * @__reserved_5:		reserved
 * @label:			optional label for the filesystem
 */
struct lanyfs_sb {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	uint32_t		magic;
	uint8_t			major;
	unsigned char		__reserved_1;
	uint8_t			minor;
	unsigned char		__reserved_2;
	uint8_t			blocksize;
	unsigned char		__reserved_3;
	uint8_t			addrlen;
	unsigned char		__reserved_4;
	uint64_t		rootdir;
	uint64_t		blocks;
	uint64_t		freehead;
	uint64_t		freetail;
	uint64_t		freeblocks;
	struct lanyfs_ts	created;
	struct lanyfs_ts	updated;
	struct lanyfs_ts	checked;
	uint64_t		badblocks;
	unsigned char		__reserved_5[8];
	char			label[LANYFS_NAME_LENGTH];
};

/**
 * struct lanyfs_btree - Binary tree components.
 * @left:			address of left node of binary tree
 * @right:			address of right node of binary tree
 */
struct lanyfs_btree {
	uint64_t		left;
	uint64_t		right;
};

/**
 * struct lanyfs_vi_btree - Aligned binary tree components.
 * @__padding_0:		padding
 * @left:			address of left node of binary tree
 * @right:			address of right node of binary tree
 *
 * Used to access binary tree components independent from underlying block type.
 * This creates a virtual block.
 */
struct lanyfs_vi_btree	{
	unsigned char		__padding_0[8];
	uint64_t		left;
	uint64_t		right;
};

/**
 * struct lanyfs_meta - LanyFS metadata.
 * @created:			date and time of creation
 * @modified:			date and time of last modification
 * @__reserved_0:		reserved
 * @attr:			directory or file attributes
 * @name:			name of file or directory
 */
struct lanyfs_meta {
	struct lanyfs_ts	created;
	struct lanyfs_ts	modified;
	unsigned char		__reserved_0[14];
	uint16_t		attr;
	char			name[LANYFS_NAME_LENGTH];
};

/**
 * struct lanyfs_vi_meta - Aligned LanyFS metadata.
 * @__padding_0:		padding
 * @created:			date and time of creation
 * @modified:			date and time of last modification
 * @__reserved_0:		reserved
 * @attr:			directory or file attributes
 * @name:			name of file or directory
 *
 * Used to access meta data independent from underlying block type.
 * This creates a virtual block.
 */
struct lanyfs_vi_meta	{
	unsigned char		__padding_0[56];
	struct lanyfs_ts	created;
	struct lanyfs_ts	modified;
	unsigned char		__reserved_0[14];
	uint16_t		attr;
	unsigned char		name[LANYFS_NAME_LENGTH];
};

/**
 * struct lanyfs_dir - LanyFS directory block.
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @__reserved_1:		reserved
 * @btree:			binary tree components
 * @subtree:			binary tree root of directory's contents
 * @__reserved_2:		reserved
 * @meta:			directory metadata
 */
struct lanyfs_dir {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	unsigned char		__reserved_1[4];
	struct lanyfs_btree	btree;
	uint64_t		subtree;
	unsigned char		__reserved_2[24];
	struct lanyfs_meta	meta;
};

/**
 * struct lanyfs_file - LanyFS file block.
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @__reserved_1:		reserved
 * @btree:			binary tree components
 * @data:			address of extender for data blocks
  * @size:			size of file in bytes
 * @__reserved_2:		reserved
 * @meta:			file metadata
 */
struct lanyfs_file {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	unsigned char		__reserved_1[4];
	struct lanyfs_btree	btree;
	uint64_t		data;
	uint64_t		size;
	unsigned char		__reserved_2[16];
	struct lanyfs_meta	meta;
};

/**
 * struct lanyfs_chain - LanyFS chain block (size-independent).
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @__reserved_1:		reserved
 * @next:			address of next chain block
 * @stream:			start of block address stream
 */
struct lanyfs_chain {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	unsigned char		__reserved_1[4];
	uint64_t		next;
	unsigned char		stream;
};

/**
 * struct lanyfs_ext - LanyFS extender block (size-independent).
 * @type:			identifies the blocks purpose
 * @__reserved_0:		reserved
 * @wrcnt:			write counter
 * @level:			depth of indirection
 * @stream:			start of block address stream
 */
struct lanyfs_ext {
	unsigned char		type;
	unsigned char		__reserved_0;
	uint16_t		wrcnt;
	uint8_t			level;
	unsigned char		stream;
};

/**
 * struct lanyfs_data - LanyFS data block.
 * @stream:			start of data stream
 */
struct lanyfs_data {
	unsigned char		stream;
};

/** union lanyfs_b - Lanyfs block.
 * @raw:			raw block
 * @sb:				superblock
 * @dir:			directory block
 * @file:			file block
 * @ext:			extender block
 */
union lanyfs_b {
	struct lanyfs_raw	raw;
	struct lanyfs_sb	sb;
	struct lanyfs_dir	dir;
	struct lanyfs_file	file;
	struct lanyfs_chain	chain;
	struct lanyfs_ext	ext;
	struct lanyfs_data	data;
	struct lanyfs_vi_btree	vi_btree;
	struct lanyfs_vi_meta	vi_meta;
};

#endif /* __LANYFS_H */
