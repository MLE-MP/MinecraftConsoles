#include <cstring>
#include "mspack/mspack.h"
#include "mspack/lzx.h"

#ifndef LZX_LOG
#define LZX_LOG(fmt, ...) MC_LOG("[LZX] " fmt, __VA_ARGS__)
#endif

#define BLOCK_MAGIC				0xAE

typedef unsigned char			byte;

struct CBlockHeader
{
	byte			magic;
	byte			offset;
	byte			align;
	int				blockSize;
};

struct mspack_file
{
	byte* buf;
	int			bufSize;
	int			pos;
	int			rest;
};

// Align integer or pointer of any type
template<class T> inline T Align(const T ptr, int alignment)
{
	return (T)(((size_t)ptr + alignment - 1) & ~(alignment - 1));
}

// Using size_t typecasts - that's platform integer type
template<class T> inline T OffsetPointer(const T ptr, int offset)
{
	return (T)((size_t)ptr + offset);
}

static int mspack_read_count = 0;

static int mspack_read(mspack_file* file, void* buffer, int bytes)
{
	int callNum = mspack_read_count++;
	if (!file->rest)
	{
		if (file->pos >= file->bufSize) {
			LZX_LOG("  mspack_read[%d]: pos=%d >= bufSize=%d, returning 0", callNum, file->pos, file->bufSize);
			return 0;
		}
		// read block header
		if (file->buf[file->pos] == 0xFF)
		{
			// [0]   = FF
			// [1,2] = uncompressed block size
			// [3,4] = compressed block size
			int uncSz = (file->buf[file->pos + 1] << 8) | file->buf[file->pos + 2];
			file->rest = (file->buf[file->pos + 3] << 8) | file->buf[file->pos + 4];
			LZX_LOG("  mspack_read[%d]: FF block at pos=%d, uncompSize=%d, compSize=%d", callNum, file->pos, uncSz, file->rest);
			file->pos += 5;
		}
		else
		{
			// [0,1] = compressed size
			file->rest = (file->buf[file->pos + 0] << 8) | file->buf[file->pos + 1];
			LZX_LOG("  mspack_read[%d]: 2-byte block at pos=%d, compSize=%d, header=[%02X %02X]", callNum, file->pos, file->rest, file->buf[file->pos], file->buf[file->pos+1]);
			file->pos += 2;
		}
		if (file->rest > file->bufSize - file->pos) {
			LZX_LOG("  mspack_read[%d]: CLAMPED rest from %d to %d (bufSize=%d pos=%d)", callNum, file->rest, file->bufSize - file->pos, file->bufSize, file->pos);
			file->rest = file->bufSize - file->pos;
		}
	}
	if (bytes > file->rest) bytes = file->rest;
	if (bytes <= 0) {
		LZX_LOG("  mspack_read[%d]: bytes<=0, returning 0 (rest=%d)", callNum, file->rest);
		return 0;
	}

	// copy block data
	memcpy(buffer, file->buf + file->pos, bytes);
	file->pos += bytes;
	file->rest -= bytes;

	return bytes;
}

static int mspack_write(mspack_file* file, void* buffer, int bytes)
{
	memcpy(file->buf + file->pos, buffer, bytes);
	file->pos += bytes;
	return bytes;
}

void* appMalloc(int size, int alignment = 8, bool noInit = false)
{
	// Allocate memory
	void* block = malloc(size + sizeof(CBlockHeader) + (alignment - 1));
	if (!block)
	{
		// OUT OF MEMORY!
		return NULL; // Will crash
	}

	// Initialize the allocated block
	void* ptr = Align(OffsetPointer(block, sizeof(CBlockHeader)), alignment);
	if (size > 0 && !noInit)
		memset(ptr, 0, size);

	// Prepare block header
	CBlockHeader* hdr = (CBlockHeader*)ptr - 1;
	byte offset = (byte*)ptr - (byte*)block;
	hdr->magic = BLOCK_MAGIC;
	hdr->offset = offset - 1;
	hdr->align = alignment - 1;
	hdr->blockSize = size;
	return ptr;
}

static void* mspack_alloc(mspack_system* self, size_t bytes)
{
	return appMalloc(bytes);
}

void appFree(void* ptr)
{
	CBlockHeader* hdr = (CBlockHeader*)ptr - 1;
	hdr->magic--;		// modify to any value
	int offset = hdr->offset + 1;
	void* block = OffsetPointer(ptr, -offset);
	free(block);
}

static void mspack_free(void* ptr)
{
	appFree(ptr);
}

static void mspack_copy(void* src, void* dst, size_t bytes)
{
	memcpy(dst, src, bytes);
}

static struct mspack_system lzxSys =
{
	NULL,				// open
	NULL,				// close
	mspack_read,
	mspack_write,
	NULL,				// seek
	NULL,				// tell
	NULL,				// message
	mspack_alloc,
	mspack_free,
	mspack_copy
};

static int scanXMemUncompressedSize(byte* buf, int bufSize)
{
	int pos = 0;
	int totalUncomp = 0;
	while (pos + 1 < bufSize) {
		if (buf[pos] == 0xFF) {
			if (pos + 4 >= bufSize) break;
			int uncompSize = (buf[pos + 1] << 8) | buf[pos + 2];
			int compSize = (buf[pos + 3] << 8) | buf[pos + 4];
			if (compSize == 0 && uncompSize == 0) break;
			LZX_LOG("  scanXMem: FF block at pos=%d, uncomp=%d, comp=%d", pos, uncompSize, compSize);
			totalUncomp += uncompSize;
			pos += 5 + compSize;
		} else {
			int compSize = (buf[pos] << 8) | buf[pos + 1];
			if (compSize == 0) break;
			LZX_LOG("  scanXMem: 2-byte block at pos=%d, comp=%d (uncompressed block)", pos, compSize);
			totalUncomp += 0x8000;
			pos += 2 + compSize;
		}
	}
	LZX_LOG("  scanXMem: totalUncompressedSize=%d", totalUncomp);
	return totalUncomp;
}

static int appDecompressLZX(byte* CompressedBuffer, int CompressedSize, byte* UncompressedBuffer, int* pUncompressedSize)
{
	int bufferSize = *pUncompressedSize;
	LZX_LOG("appDecompressLZX ENTER: compSize=%d bufferSize=%d compBuf=%p uncompBuf=%p", CompressedSize, bufferSize, CompressedBuffer, UncompressedBuffer);
	if (CompressedSize > 0) {
		LZX_LOG("  first 16 compressed bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
			CompressedBuffer[0], CompressedBuffer[1], CompressedBuffer[2], CompressedBuffer[3],
			CompressedBuffer[4], CompressedBuffer[5], CompressedBuffer[6], CompressedBuffer[7],
			CompressedSize > 8 ? CompressedBuffer[8] : 0, CompressedSize > 9 ? CompressedBuffer[9] : 0,
			CompressedSize > 10 ? CompressedBuffer[10] : 0, CompressedSize > 11 ? CompressedBuffer[11] : 0,
			CompressedSize > 12 ? CompressedBuffer[12] : 0, CompressedSize > 13 ? CompressedBuffer[13] : 0,
			CompressedSize > 14 ? CompressedBuffer[14] : 0, CompressedSize > 15 ? CompressedBuffer[15] : 0);
	}

	int actualUncompSize = scanXMemUncompressedSize(CompressedBuffer, CompressedSize);
	if (actualUncompSize <= 0) {
		LZX_LOG("  scanXMem returned %d, cannot decompress", actualUncompSize);
		*pUncompressedSize = 0;
		return -1;
	}
	if (actualUncompSize > bufferSize) {
		LZX_LOG("  scanXMem size %d exceeds buffer %d!", actualUncompSize, bufferSize);
		*pUncompressedSize = 0;
		return -1;
	}
	LZX_LOG("  using scanned uncompressed size: %d (buffer was %d)", actualUncompSize, bufferSize);

	mspack_read_count = 0;
	// setup streams
	mspack_file src, dst;
	src.buf = CompressedBuffer;
	src.bufSize = CompressedSize;
	src.pos = 0;
	src.rest = 0;
	dst.buf = UncompressedBuffer;
	dst.bufSize = actualUncompSize;
	dst.pos = 0;
	// prepare decompressor
	LZX_LOG("  lzxd_init: window_bits=17, reset_interval=0, input_buffer=256KB, output=%d", actualUncompSize);
	lzxd_stream* lzxd = lzxd_init(&lzxSys, &src, &dst, 17, 0, 256 * 1024, actualUncompSize);
	if (!lzxd) {
		LZX_LOG("  lzxd_init FAILED! returned NULL");
		*pUncompressedSize = 0;
		return -1;
	}
	LZX_LOG("  lzxd_init OK, calling lzxd_decompress(%d)", actualUncompSize);

	// decompress
	int r = lzxd_decompress(lzxd, actualUncompSize);
	LZX_LOG("  lzxd_decompress returned: %d (0=OK, MSPACK_ERR values otherwise)", r);
	if (r != 0) {
		LZX_LOG("  DECOMPRESSION FAILED: error=%d, src.pos=%d/%d, dst.pos=%d/%d", r, src.pos, src.bufSize, dst.pos, dst.bufSize);
		*pUncompressedSize = 0;
	} else {
		*pUncompressedSize = dst.pos;
		LZX_LOG("  decompressed %d bytes", dst.pos);
	}
	// free resources
	lzxd_free(lzxd);
	LZX_LOG("appDecompressLZX EXIT: result=%d, outputSize=%d", r, *pUncompressedSize);
	return r;
}
