// file management(FAT12)

#include "../include/bootpack.h"
#include "../include/file.h"
#include <string.h>

#define MAX_CLUSTER   2880
#define CLUSTER_SIZE  512

#define FINFO_TOP  ((struct FILEINFO*)(ADR_DISKIMG + 0x002600))
#define FINFO_MAX  224

#define DISK_FAT           (unsigned char*)(ADR_DISKIMG + 0x000200)
#define DISK_CLUSTER_DATA  (unsigned char*)(ADR_DISKIMG + 0x003e00)

#define CLUSTNO_FAT1     1       // FAT1:    0x000200 - 0x001400  ( 1~ 9)
#define CLUSTNO_FAT2     10      // FAT2:    0x001400 - 0x002600  (10~18)
#define CLUSTNO_ROOTDIR  19      // ROOTDIR: 0x002600 - 0x004200  (19~32)
#define CLUSTNO_ENTITY   31      // ENTITY:  0x004200 -           (33~)  (Cluster starts from 2)

#define DMA_DATABUF     1024
#define FDC_RESULT_MAXCOUNT 0x10

#define DMA_ADD_SEC     0x04  //channel2 low address
#define DMA_CNT_SEC     0x05  //channel2 count address
#define DMA_TOP         0x81  //channel2 high address

#define DMA_CMD_PRI     0xD0
#define DMA_CMD_SEC     0x08
#define DMA_REQ_PRI     0xD2
#define DMA_REQ_SEC     0x09
#define DMA_SGL_MSK_PRI 0xD4
#define DMA_SGL_MSK_SEC 0x0A
#define DMA_MOD_PRI     0xD6
#define DMA_MOD_SEC     0x0B
#define DMA_CLR_FLP_PRI 0x0C
#define DMA_CLR_FLP_SEC 0xD8
#define DMA_MSR_CLR_PRI 0xDA
#define DMA_MSR_CLR_SEC 0x0D
#define DMA_CLR_MSK_PRI 0xDC
#define DMA_CLR_MSK_SEC 0x0E
#define DMA_ALL_MSK_PRI 0xDE
#define DMA_ALL_MSK_SEC 0x0F


#define FDC_SRA  0x3f0	// FDC status registerA (R)
#define FDC_SRB  0x3f1	// FDC status registerB (R)
#define FDC_DOR  0x3f2	// FDC Control register (R/W)
#define FDC_MSR  0x3f4	// FDC Status register (R)
#define FDC_DSR  0x3f4	// FDC data rate select register (W)
#define FDC_DAT  0x3f5	// FDC Data(R/W)
#define FDC_DIR  0x3f7	// FDC digital input register (R)
#define FDC_CCR  0x3f7	// FDC configuration control register (W)

#define MSR_RQM    0x80
#define MSR_DIO    0x40
#define MSR_BUSY   0x10
#define MSR_READY  0

/* FDC CMD */
#define CMD_SPECIFY         0x03
#define CMD_RECALIBRATE     0x07
#define CMD_SENSE_INT_STS   0x08
#define CMD_SEEK            0x0f
#define CMD_READ            0x46  	//MT=0,MF=1,SK=0
#define CMD_WRITE           0x45 	//MT=0,MF=1,SK=0

#define CMD_SUB 0x00 //HD=0, US1 & US0 = 0

static unsigned char dma_databuf[DMA_DATABUF];
static unsigned int fdc_interrupt = 0;

static struct _dma_trans {
	unsigned int count;
	unsigned int addr;
} dma_trans;

static struct FDC_RESULTS {
	unsigned char gets;
	unsigned char req_sense;
	unsigned int  status_count;
	unsigned char status[10];
} fdc_results;

static int fdc_chk_interrupt()
{
	return fdc_interrupt;
}

static unsigned int fdc_wait_interrupt()
{
	while (!fdc_chk_interrupt());
	return 1;
}

static void fdc_clear_interrupt()
{
	fdc_interrupt = 0;
	return;
}

int *inthandler26()
{
	++fdc_interrupt;
	io_out8(PIC0_OCW2, 0x66);
	return 0;
}

void fdc_dma_start()
{
	io_out8(DMA_SGL_MSK_SEC, 0x02);
}

void fdc_dma_stop()
{
	io_out8(DMA_SGL_MSK_SEC, 0x06);
}

void init_dma(void)
{
	io_out8(DMA_MSR_CLR_PRI, 0x00);
	io_out8(DMA_MSR_CLR_SEC, 0x00);

	io_out8(DMA_CMD_PRI, 0x00); 
	io_out8(DMA_CMD_SEC, 0x00); 

	io_out8(DMA_MOD_PRI, 0xC0); 
	io_out8(DMA_MOD_SEC, 0x46); 

	io_out8(DMA_ALL_MSK_PRI, 0x00);
	return;
}

static void init_dma_r() {
  fdc_dma_stop();

  io_out8(DMA_MSR_CLR_SEC, 0x00);
  io_out8(DMA_CLR_FLP_SEC, 0);

  io_out8(DMA_MOD_SEC, 0x46);
  io_cli();
  io_out8(DMA_ADD_SEC, dma_trans.addr >> 0);
  io_out8(DMA_ADD_SEC, dma_trans.addr >> 8);
  io_out8(DMA_TOP, dma_trans.addr >> 16);
  io_out8(DMA_CNT_SEC, dma_trans.count >> 0);
  io_out8(DMA_CNT_SEC, dma_trans.count >> 8);
  io_sti();
  fdc_dma_start();
}

static void init_dma_w()
{
  fdc_dma_stop();

  io_out8(DMA_MSR_CLR_SEC, 0x00);
  io_out8(DMA_CLR_FLP_SEC, 0);

  io_out8(DMA_MOD_SEC, 0x4a);
  io_cli();
  io_out8(DMA_ADD_SEC, dma_trans.addr >> 0);
  io_out8(DMA_ADD_SEC, dma_trans.addr >> 8);
  io_out8(DMA_TOP, dma_trans.addr >> 16);
  io_out8(DMA_CNT_SEC, dma_trans.count >> 0);
  io_out8(DMA_CNT_SEC, dma_trans.count >> 8);
  io_sti();
  fdc_dma_start();
}

static int fdc_wait_msrStatus(unsigned char mask, unsigned char expected)
{
	int count;
	for (count=0; count<FDC_RESULT_MAXCOUNT; ++count) {
		unsigned char status = io_in8(FDC_MSR);
		if ((status & mask) == expected) {
			return status;
		}
	}
	return 0;
}

static int fdc_cmd(const unsigned char *cmd, const int length)
{
	if (!fdc_wait_msrStatus(MSR_BUSY, MSR_READY)) {
		return 0;
	}

	int i;
	for (i=0; i<length; i++) {
		if (!fdc_wait_msrStatus(MSR_RQM | MSR_DIO, MSR_RQM)) {
			return 0;
		}
		io_out8(FDC_DAT, cmd[i]);
	}
	return 1;
}

static int fdc_read_results()
{
	fdc_results.status_count = 0;

	if (fdc_wait_msrStatus(MSR_RQM | MSR_DIO, MSR_RQM | MSR_DIO) < 0) {
		return 0;
	}

	unsigned char *msr = &fdc_results.status[0];
	while (1) {
		*msr++ = io_in8(FDC_DAT);
		++fdc_results.status_count;

		int status = fdc_wait_msrStatus(MSR_RQM, MSR_RQM);
		if (status < 0) {
			return 0;
		}
		if (!(status & MSR_DIO)) {
			return 1;
		}
	}
}

static int fdc_sense_interrupt()
{
	unsigned char cmd[] = {
		CMD_SENSE_INT_STS
	};

	fdc_clear_interrupt();
	if (!fdc_cmd(cmd, sizeof(cmd))) {
		return 0;
	}
	fdc_read_results();
	return 1;
}

static void fdc_motor_on(void)
{
	io_out8(FDC_DOR, 0x1C);
	return;
}

static void fdc_motor_off(void)
{
	io_out8(FDC_DOR, 0x0C);
	return;
}

static int fdc_recalibrate()
{
	static const unsigned char cmd[] = { CMD_RECALIBRATE, CMD_SUB };

	fdc_clear_interrupt();

	if (!fdc_cmd(cmd, sizeof(cmd))) {
		return 0;
	}

	if (!fdc_wait_interrupt()) {
		return 0;
	}

	fdc_sense_interrupt();

	if (!fdc_wait_msrStatus(MSR_BUSY, MSR_READY)) {
		return 0;
	}

	return 1;
}

static void fdc_specify(void)
{
	fdc_motor_on();

	static const unsigned char specify_cmd[] = {
		CMD_SPECIFY,
		0xC1,
		0x10
	};
	fdc_clear_interrupt();
	fdc_cmd(specify_cmd, sizeof(specify_cmd));

	fdc_motor_off();
	return;
}

void init_fdc(void)
{
	init_dma();
	dma_trans.addr = (unsigned int)&dma_databuf[0];
	dma_trans.count = 512;

	io_out8(FDC_DOR, 0x00);
	io_out8(FDC_CCR, 0x00);
	io_out8(FDC_DOR, 0x0C);

	fdc_specify();

	return;
}

static int fdc_seek(unsigned char cyl)
{
	int drive = 0, head = 0;
	unsigned char cmd[] = {
		CMD_SEEK,
		(head << 2) | drive,
		cyl
	};

	fdc_clear_interrupt();

	if (!fdc_cmd(cmd, sizeof(cmd))) {
		return 0;
	}

	if (!fdc_wait_interrupt()) {
		return 0;
	}

	if (!fdc_sense_interrupt()) {
		return 0;
	}

	return 1;
}

int fdc_write(void *buf, int head, int cyl, int sector)
{
	init_dma_w();
	fdc_motor_on();

	if (!fdc_recalibrate()) {
		return 0;
	}

	if (!fdc_seek(cyl)) {
		return 0;
	}

	memcpy(dma_databuf, buf, 512);

	unsigned char cmd[] = {
		CMD_WRITE,
		head << 2,
		cyl,
		head,
		sector,
		0x2,
		0x12,
		0x1B,
		0x00,
	};

	fdc_clear_interrupt();

	if (!fdc_cmd(cmd, sizeof(cmd))) {
		return 0;
	}

	if (!fdc_wait_interrupt()) {
		return 0;
	}

	if (!fdc_read_results()) {
		return 0;
	}

	fdc_motor_off();
	return 1;
}

static int write_sector(int sector)
{
	const int SECTOR_COUNT = 18, HEAD_COUNT = 2;
	int cyl = sector / SECTOR_COUNT / HEAD_COUNT;
	int head = (sector / SECTOR_COUNT) & 1;
	int sec = (sector % SECTOR_COUNT) + 1;

	unsigned char *buf = (unsigned char *)(ADR_DISKIMG + sector * CLUSTER_SIZE);

	return fdc_write(buf, head, cyl, sec);
}

short get_next_cluster(short cluster)
{
	const unsigned char *p = DISK_FAT + (cluster >> 1) * 3;
	if ((cluster & 1) == 0) {
		return (p[0] | p[1] << 8) & 0xfff;
	} else {
		return (p[1] >> 4 | p[2] << 4) & 0xfff;
	}
}

static void set_next_cluster(short cluster, short next)
{
	unsigned char *p = DISK_FAT + (cluster >> 1) * 3;
	if ((cluster & 1) == 0) {
		p[0] = next;
		p[1] = (p[1] & 0xF0) | ((next >> 8) & 0x0F);
	} else {
		p[1] = (p[1] & 0x0F) | ((next << 4) & 0xF0);
		p[2] = next >> 4;
	}
}

static int write_dir_table(struct FILEINFO *finfo)
{
	int file_pos = finfo - FINFO_TOP;
	int offset = file_pos * sizeof(struct FILEINFO) / CLUSTER_SIZE;
	return write_sector(offset + CLUSTNO_ROOTDIR);
}

short allocate_cluster(void)
{
	int i;
	for (i=0; i<MAX_CLUSTER; ++i) {
		if (get_next_cluster(i) == 0x00) {
			set_next_cluster(i, 0xfff);
			return i;
		}
	}
	return -1;
}

static unsigned char *cluster_data(int cluster)
{
	return DISK_CLUSTER_DATA + cluster * CLUSTER_SIZE;
}

int fd_open(struct FILEHANDLE *fh, const char *name)
{
	fh->pos = 0;
	fh->cluster = 0;
	fh->modified = 0;

	fh->finfo = file_search((char *)name, fh->finfo, FINFO_MAX);
	if (fh->finfo == 0) {
		return 0;
	}

	fh->cluster = fh->finfo->clustno;
	return 1;
}

int fd_write(struct FILEHANDLE *fh, const void* src_data, int size)
{
	if (size <= 0) return 0;

	fh->modified = 1;
	int write_size = 0;
	unsigned char *src = (unsigned char *)src_data;
	while (size > 0) {
		if (fh->pos == 0) {
			if (fh->finfo->clustno > 0) {
				fh->cluster = fh->finfo->clustno;
			} else {
				fh->cluster = fh->finfo->clustno = allocate_cluster();
			}
		} else if ((fh->pos % CLUSTER_SIZE) == 0) {
			short next_cluster = get_next_cluster(fh->cluster);
			if (next_cluster < 0xff0) {
				fh->cluster = next_cluster;
			} else {
				next_cluster = allocate_cluster();
				set_next_cluster(fh->cluster, next_cluster);
				fh->cluster = next_cluster;
			}
		}
		int s = CLUSTER_SIZE - (fh->pos % CLUSTER_SIZE);
		if (size < s) s = size;

		unsigned char *dst = cluster_data(fh->cluster) + (fh->pos % CLUSTER_SIZE);

		memcpy(dst, src, s);
		fh->pos += s;
		write_size += s;
		dst += s;
		size -= s;
	}
	return write_size;
}

/**
 * @brief FAT 테이블 읽기 함수
 * 
 * @param fat: FAT 테이블 포인터
 * @param img: 디스크 이미지 포인터
 * @return: void
 */
void file_readfat(int *fat, unsigned char *img)
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}

/**
 * @brief 파일 로드 함수
 * 
 * @param clustno: 시작 클러스터 번호
 * @param size: 파일 크기
 * @param buf: 파일 데이터를 저장할 버퍼 포인터
 * @param fat: FAT 테이블 포인터
 * @param img: 디스크 이미지 포인터
 * @return: void
 */
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}

/**
 * @brief 파일 검색 함수
 * 
 * @param name: 검색할 파일 이름 문자열
 * @param finfo: 파일 정보 구조체 배열 포인터
 * @param max: 파일 정보 구조체 배열의 최대 개수
 * @return struct FILEINFO*: 검색된 파일 정보 구조체 포인터, 없으면 0 반환
 */
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
    int x, y;
    unsigned char s[12];
    for (y = 0; y < 11; y++) {
		s[y] = ' ';
	}
	y = 0;
	for (x = 0; name[x] != 0; x++) {
        if (y >= 11) { return 0; } // 이름이 너무 김
		if (name[x] == '.' && y <= 8) {
			y = 8;
		} else {
			s[y] = name[x];
		    if ('a' <= s[y] && s[y] <= 'z') {
		    	s[y] -= 0x20;
			} 
			y++;
		}
	}
	for (x = 0; x < 224; ) {
		if (finfo[x].name[0] == 0x00) {
	    	break;
		}
		if ((finfo[x].type & 0x18) == 0) {
			for (y = 0; y < 11; y++) {
		    	if (finfo[x].name[y] != s[y]) {
					goto next;
				}
			}
			return finfo + x; 
		}
next:
		x++;
	}
    return 0;
}

/**
 * @brief 파일 로드 함수 (tek 압축 해제 지원)
 * 
 * @param clustno: 시작 클러스터 번호
 * @param psize: 파일 크기 포인터 (압축 해제 시 크기 변경됨)
 * @param fat: FAT 테이블 포인터
 * @return char*: 파일 데이터를 저장한 버퍼 포인터
 */
char *file_loadfile_check_tek(int clustno, int *psize, int *fat)
{
	int size = *psize, size2;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *buf, *buf2;
	buf = (char *) memman_alloc_4k(memman, size);
	file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
	// tek 압축 해제
	if (size >= 17) {
		size2 = tek_getsize(buf);
		if (size2 > 0) {
			buf2 = (char *) memman_alloc_4k(memman, size2);
			tek_decomp(buf, buf2, size2);
			memman_free_4k(memman, (int) buf, size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}
