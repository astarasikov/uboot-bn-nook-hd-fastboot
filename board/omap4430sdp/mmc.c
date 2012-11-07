/*
 * Copyright (c) 2010, The Android Open Source Project.
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
#include <asm/io.h>

#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <fastboot.h>

#define BOOT_TYPE ( ( *(volatile unsigned int*) ( 0x4A326000 ) ) & 0xFF )
#define RESET_REASON ( ( * ( (volatile unsigned int *) ( 0x4A307B04 ) ) ) & 0x3 )
#define WARM_RESET ( 1 << 1 )
#define COLD_RESET ( 1 )
#define SD_BOOT  ( 0x05 )
#define EMMC_BOOT ( 0x06 )

#define EFI_VERSION 0x00010000
#define EFI_ENTRIES 128
#define EFI_NAMELEN 36

/* Default to eMMC slot for omap4430sdp Blaze and Blaze Tablet */
int mmc_slot = 1;

static const u8 partition_type[16] = {
	0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
	0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};

static const u8 random_uuid[16] = {
	0xff, 0x1f, 0xf2, 0xf9, 0xd4, 0xa8, 0x0e, 0x5f,
	0x97, 0x46, 0x59, 0x48, 0x69, 0xae, 0xc3, 0x4e,
};
	
struct efi_entry {
	u8 type_uuid[16];
	u8 uniq_uuid[16];
	u64 first_lba;
	u64 last_lba;
	u64 attr;
	u16 name[EFI_NAMELEN];
};

struct efi_header {
	u8 magic[8];

	u32 version;
	u32 header_sz;

	u32 crc32;
	u32 reserved;

	u64 header_lba;
	u64 backup_lba;
	u64 first_lba;
	u64 last_lba;

	u8 volume_uuid[16];

	u64 entries_lba;

	u32 entries_count;
	u32 entries_size;
	u32 entries_crc32;
} __attribute__((packed));

struct ptable {
	u8 mbr[512];
	union {
		struct efi_header header;
		u8 block[512];
	};
	struct efi_entry entry[EFI_ENTRIES];	
};

int board_set_flash_slot(char * slot_name)
{
	int ret = 0;
	if (!strcmp(slot_name, "SD"))
		mmc_slot = 0;
	else if (!strcmp(slot_name, "EMMC"))
		mmc_slot = 1;
	else
		ret = -1;

	return(ret);
}

static void init_mbr(u8 *mbr, u32 blocks)
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

static void start_ptbl(struct ptable *ptbl, unsigned blocks)
{
	struct efi_header *hdr = &ptbl->header;

	memset(ptbl, 0, sizeof(*ptbl));

	init_mbr(ptbl->mbr, blocks - 1);

	memcpy(hdr->magic, "EFI PART", 8);
	hdr->version = EFI_VERSION;
	hdr->header_sz = sizeof(struct efi_header);
	hdr->header_lba = 1;
	hdr->backup_lba = blocks - 1;
	hdr->first_lba = 34;
	hdr->last_lba = blocks - 1;
	memcpy(hdr->volume_uuid, random_uuid, 16);
	hdr->entries_lba = 2;
	hdr->entries_count = EFI_ENTRIES;
	hdr->entries_size = sizeof(struct efi_entry);
}

static void end_ptbl(struct ptable *ptbl)
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

int add_ptn(struct ptable *ptbl, u64 first, u64 last, const char *name)
{
	if (!strcmp(cmd, "format"))
		return efi_do_format(partitions, CFG_FASTBOOT_MMC_NO);
	return -1;
}

void board_mmc_init(void)
{
	/* nothing to do this early */
}

void read_bcb(unsigned char * bcb_data)
{
	static unsigned char data[512];
	static struct efi_entry entry[4];
	int n,m,r;
	printf("ptbl slot: %s:(%d).\n",
			mmc_slot?"EMMC":"SD", mmc_slot);
	r = mmc_read(mmc_slot, 1, data, 512);
	if (r != 1) {
		printf("error reading partition table\n");
		return -1;
	}
	if (memcmp(data, "EFI PART", 8)) {
		printf("efi partition table not found\n");
		return -1;
	}
	for (n = 0; n < (128/4); n++) {
		r = mmc_read(mmc_slot, 1 + n, (void*) entry, 512);
		if (r != 1) {
			printf("partition read failed\n");
			return 1;
		}
		for (m = 0; m < 4; m ++)
			import_efi_partition(entry + m);
	}
	return 0;
}
void write_bcb(unsigned char * bcb_data)
{
	struct ptable *ptbl = &the_ptable;
	unsigned sector_sz, blocks;
	unsigned next;
	int n;

	printf("Formatting %s(%d) slot.\n", mmc_slot?"EMMC":"SD", mmc_slot);
	if (mmc_init(mmc_slot)) {
		printf("mmc init failed?\n");
		return -1;
	}

	mmc_info(mmc_slot, &sector_sz, &blocks);
	printf("blocks %d\n", blocks);

	start_ptbl(ptbl, blocks);
	n = 0;
	next = 0;
	for (n = 0, next = 0; partitions[n].name; n++) {
		unsigned sz = partitions[n].size_kb * 2;
		if (!strcmp(partitions[n].name,"-")) {
			next += sz;
			continue;
		}
		if (sz == 0)
			sz = blocks - next;
		if (add_ptn(ptbl, next, next + sz - 1, partitions[n].name))
			return -1;
		next += sz;
	}
	end_ptbl(ptbl);

	fastboot_flash_reset_ptn();
	if (mmc_write(mmc_slot, (void*) ptbl, 0, sizeof(struct ptable)) != 1)
		return -1;

	printf("\nnew partition table:\n");
	load_ptbl();

	return 0;
}
/**
*  get_partition_sz: Returns size of requested paritition in eMMC
* @buf: Caller buffer pointer the size will be returned in
* @partname: partittion name being requested
*/
char * get_partition_sz(char *buf, const char *partname)
{
	struct ptable *ptbl = &the_ptable;
	struct efi_header *hdr = &ptbl->header;
	struct efi_entry *entry = ptbl->entry;
	unsigned n, i;
	char curr_partname[EFI_NAMELEN];
	u64 fist_lba, last_lba, sz;
	u32 crc_orig;
	u32 crc;
	u32 *szptr = (u32 *) &sz;

	if (mmc_read(mmc_slot, 0,  (void *)ptbl, sizeof(struct ptable)) != 1){
		printf("\n ERROR Reading Partition Table \n");
		return buf;
	}

	/*Make sure there is a legit partition table*/
	if (load_ptbl()){
		printf("\n INVALID PARTITION TABLE \n");
		return buf;
	}
}

int board_late_init(void)
{
	int dev = CFG_FASTBOOT_MMC_NO;
	unsigned int boot_type = BOOT_TYPE;
	int ret_val = 0;
	unsigned char temp_buffer[1024];

	volatile unsigned int * reset_reason = 0x4A307B04;
	volatile unsigned int * update_image_location = 0x80000000;

	printf("\n RESET REASON : %d\n",(*reset_reason)&0x3);

	if( boot_type == SD_BOOT ) {
		/*
		 * Override boot args incase of SD CARD boot only,
		 * change this to cold boot once TI fixes reset issue
		 */
		printf("\n Booting from SDCARD\n");
		setenv ("bootargs","androidboot.console=ttyO0 console=ttyO0,115200n8 mem=448M@0x80000000 mem=512M@0xA0000000 root=/dev/mmcblk1p2 rw init=/init rootwait vram=\"32M\" omapfb.vram=\"0:5M\" ");
		setenv ("bootcmd", "mmcinit 0; fatload mmc 0 0x81000000 uImage; bootm 0x81000000 ");
	} else {

		* update_image_location  = 0xDEADBEEF;
		ret_val = efi_load_ptbl(dev);

		printf("\n Check for update image in sdcard\n");
		if ( ! run_command ( "mmcinit 0; fatload mmc 0 0x80000000 acclaim_update.zip 4",0)){
			if ( (* update_image_location ) != 0xDEADBEEF ) {
				printf( "\n Found update image on sdcard, boot into recovery\n");
				run_command( " booti mmc1 recovery", 0);
			}
		} else {
			printf("\n No update image found, booting into android\n");
		}
	}

	return ret_val;
}

/**
*  get_boot_slot: Returns boot from SD or eMMC
* @ret: 0:SD	1:eMMC
*/
static int get_boot_slot(void)
{
	u32 control_register = __raw_readl(OMAP44xx_BOOT_DEVICE);

	if ((control_register & 0xff) == 0x5)
		return 0;
	else
		return 1;
}

int omap4_mmc_init(void)
{
	int i;
	char booticmd[20];
	int boot_slot = get_boot_slot();

	/* If we booted off of SD slot, initialize SD card as well. */
	if (boot_slot == 0) {
		printf("Initializing SD(0) Slot.\n");
		/* Temporarily set mmc_slot to SD */
		mmc_slot = 0;
		if (mmc_init(boot_slot)) {
			printf("mmc init failed?\n");
			return 1;
		}
		load_ptbl();
	}

	/* Default back to eMMC(1) slot
	  * If someone wants to flash all partitions to SD slot
	  * they need to explicty give "fastboot oem set_boot_slot:SD"
	  */
	mmc_slot = 1;

	if (mmc_init(mmc_slot)) {
		printf("mmc init failed?\n");
		return 1;
	}
	sprintf(booticmd, "booti mmc%d", boot_slot);
	setenv("bootcmd", booticmd);
	printf("efi partition table:\n");
	return load_ptbl();
}

