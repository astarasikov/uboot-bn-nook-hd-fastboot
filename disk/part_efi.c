/*
 * Copyright (c) 2012, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <fastboot.h>

/* Windows Basic data partition GUID */
/* EBD0A0A2-B9E5-4433-87C0-68B6B72699C7 */
static const u8 windows_partition_type[16] = {
	0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
	0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7
};

/* Linux data partition GUID */
/* 0FC63DAF-8483-4772-8E79-3D69D8477DE4 */
static const u8 linux_partition_type[16] = {
	0xaf, 0x3d, 0xc6, 0x0f, 0x83, 0x84, 0x72, 0x47,
	0x8e, 0x79, 0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4
};

/* Empty partition */
static const u8 empty_partition_type[16] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Use Windows data GUID by default */
#define partition_type windows_partition_type

static const u8 random_uuid[16] = {
	0xff, 0x1f, 0xf2, 0xf9, 0xd4, 0xa8, 0x0e, 0x5f,
	0x97, 0x46, 0x59, 0x48, 0x69, 0xae, 0xc3, 0x4e,
};

static void efi_init_mbr(u8 *mbr, u32 blocks)
{
	mbr[0x1be] = 0x00; // nonbootable
	mbr[0x1bf] = 0xFF; // bogus CHS
	mbr[0x1c0] = 0xFF;
	mbr[0x1c1] = 0xFF;

	mbr[0x1c2] = 0xEE; // GPT partition
	mbr[0x1c3] = 0xFF; // bogus CHS
	mbr[0x1c4] = 0xFF;
	mbr[0x1c5] = 0xFF;

	mbr[0x1c6] = 0x01; // start
	mbr[0x1c7] = 0x00;
	mbr[0x1c8] = 0x00;
	mbr[0x1c9] = 0x00;

	memcpy(mbr + 0x1ca, &blocks, sizeof(u32));

	mbr[0x1fe] = 0x55;
	mbr[0x1ff] = 0xaa;
}

static void efi_start_ptbl(struct efi_ptable *ptbl, unsigned blocks)
{
	struct efi_header *hdr = &ptbl->header;

	memset(ptbl, 0, sizeof(*ptbl));

	efi_init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba  = hdr->backup_lba - hdr->first_lba + hdr->header_lba;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void efi_end_ptbl(struct efi_ptable *ptbl)
{
	struct efi_header *hdr = &ptbl->header;
	u32 n;

	n = crc32(0, 0, 0);
	n = crc32(n, (void*) ptbl->entry, sizeof(ptbl->entry));
	hdr->entries_crc32 = n;

	n = crc32(0, 0, 0);
	n = crc32(0, (void*) &ptbl->header, sizeof(ptbl->header));
	hdr->crc32 = n;
}

static int efi_add_ptn(struct efi_ptable *ptbl, u64 first, u64 last, const char *name)
{
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n;

	if (first < hdr->first_lba) {
		printf("partition '%s' overlaps partition table\n", name);
		return -1;
	}

	if (last > hdr->last_lba) {
		printf("partition '%s' does not fit\n", name);
		return -1;
	}
	for (n = 0; n < hdr->entries_count; n++, entry++) {
		if (entry->last_lba)
			continue;
		memcpy(entry->type_uuid, partition_type, 16);
		memcpy(entry->uniq_uuid, random_uuid, 16);
		entry->uniq_uuid[0] = n;
		entry->first_lba = first;
		entry->last_lba = last;
		for (n = 0; (n < EFI_NAMELEN) && *name; n++)
			entry->name[n] = *name++;
		return 0;
	}
	printf("out of partition table entries\n");
	return -1;
}

static void efi_import_partition(struct efi_entry *entry)
{
	struct fastboot_ptentry e;
	int n;
	unsigned int blocks;
#ifdef USE_ONLY_GPT_DATA_PARTITIONS
	if (memcmp(entry->type_uuid, windows_partition_type, sizeof(windows_partition_type)) &&
	    memcmp(entry->type_uuid, linux_partition_type, sizeof(linux_partition_type)))
		return;
#else
	if (!memcmp(entry->type_uuid, empty_partition_type, sizeof(empty_partition_type)))
		return;
#endif
	for (n = 0; n < (sizeof(e.name)-1); n++)
		e.name[n] = entry->name[n];
	e.name[n] = 0;
	e.start = entry->first_lba;
	blocks = entry->last_lba - entry->first_lba + 1;
	e.length = (entry->last_lba - entry->first_lba + 1) * 512;
	e.flags = 0;

	if (!strcmp(e.name, "environment"))
		e.flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_ENV;
	fastboot_flash_add_ptn(&e);

	if (blocks >= 2048)
		printf("%8d %8d %7dM %s\n", e.start, (u32)entry->last_lba, blocks/2048, e.name);
	else
		printf("%8d %8d %7dK %s\n", e.start, (u32)entry->last_lba, blocks/2,    e.name);
}

int efi_load_ptbl(int dev)
{
	static unsigned char data[512];
	static struct efi_entry entry[4];
	int n, m, r;

	fastboot_flash_reset_ptn();
	r = mmc_read(dev, 1, data, 512);
	if (r != 1) {
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		printf("efi partition table not found\n");
		return -1;
	}
	printf("\nefi partition table for mmc %d:\n", dev);
	for (n = 0; n < (128/4); n++) {
		r = mmc_read(dev, 2 + n, (void*) entry, 512);
		if (r != 1) {
			printf("partition read failed\n");
			return -1;
		}
		for (m = 0; m < 4; m ++)
			efi_import_partition(entry + m);
	}
	return 0;
}

int efi_do_format(struct efi_partition_info * partitions, int dev)
{
	struct efi_ptable ptable;
	struct efi_ptable * ptbl = &ptable;
	int ret;
	unsigned sector_sz, blocks;
	unsigned next;
	int n;

	if (mmc_init(dev)) {
		printf("mmc init %d failed?\n", dev);
		return -1;
	}

	mmc_info(dev, &sector_sz, &blocks);
	printf("mmc %d has %d blocks\n", dev, blocks);

	efi_start_ptbl(ptbl, blocks);
	for (n = 0, next = 0; partitions[n].name; n++) {
		unsigned sz = partitions[n].size_kb * 2;
		if (!strcmp(partitions[n].name, "-")) {
			next += sz;
			continue;
		}
		if (partitions[n].start_kb > 0) {
		    next = partitions[n].start_kb * 2;
		}
		if (sz == 0) {
			sz = ptbl->header.last_lba + 1 - next;
			/* for slightly different eMMC chips formatted size should not differ */
			sz &= ~(16*1024*2-1); /* align to 16MB granularity */
		}
		if (efi_add_ptn(ptbl, next, next + sz - 1, partitions[n].name))
			return -1;
		next += sz;
	}
	efi_end_ptbl(ptbl);

	fastboot_flash_reset_ptn();
	ret = mmc_write(dev, (void*) ptbl, 0, sizeof(struct efi_ptable));
	if (ret != 1) {
		printf("ERROR: cannot write partition table to mmc %d\n", dev);
		return -1;
	}

	printf("\nnew partition table for mmc %d: \n", dev);
	efi_load_ptbl(dev);

	return 0;
}

/**
*  efi_get_partition_sz: Returns size of requested paritition in eMMC
* @buf: Caller buffer pointer the size will be returned in
* @partname: partittion name being requested
*/
char * efi_get_partition_sz(char *buf, const char *partname, int dev)
{
	struct efi_ptable ptable;
	struct efi_ptable *ptbl = &ptable;
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n, i;
	char curr_partname[EFI_NAMELEN];
	u64 sz;
	u32 crc_orig;
	u32 crc;
	u32 *szptr = (u32 *) &sz;

	if (mmc_read(dev, 0,  (void *)ptbl, sizeof(struct efi_ptable)) != 1){
		printf("\n ERROR Reading Partition Table \n");
		return buf;
	}

	/*Make sure there is a legit partition table*/
	if (efi_load_ptbl(dev)) {
		printf("\n INVALID PARTITION TABLE \n");
		return buf;
	}

	/*crc needs to be computed with crc zeroed out.*/
	crc_orig = hdr->crc32;
	hdr->crc32 = 0;
	crc = crc32(0,0,0);
	crc = crc32(0, (void *) &ptbl->header, sizeof(ptbl->header));
	if (crc != crc_orig){
		printf("\n INVALID HEADER CRC!!\n");
		return buf;
	}

	for (n=0; n < EFI_ENTRIES; n++, entry++) {
		for (i = 0; i < EFI_NAMELEN; i++)
			curr_partname[i] = (char) entry->name[i];
		if (!strcmp(curr_partname, partname)){
			if (entry->last_lba < entry->first_lba){
				printf("\n RIDICULOUS LENGTH!!\n");
				break;
			}
			sz = (entry->last_lba - entry->first_lba)/2;
			if (sz >= 0xFFFFFFFF)
				sprintf(buf, "0x%08x , %08x KB", szptr[1], szptr[0]);
			else
				sprintf(buf, "%d KB", szptr[0]);

			break;
		}
	}
	return buf;
}
