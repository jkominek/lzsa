/*
 * lib.h - LZSA library definitions
 *
 * Copyright (C) 2019 Emmanuel Marty
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * Uses the libdivsufsort library Copyright (c) 2003-2008 Yuta Mori
 *
 * Inspired by LZ4 by Yann Collet. https://github.com/lz4/lz4
 * With help, ideas, optimizations and speed measurements by spke <zxintrospec@gmail.com>
 * With ideas from Lizard by Przemyslaw Skibinski and Yann Collet. https://github.com/inikep/lizard
 * Also with ideas from smallz4 by Stephan Brumme. https://create.stephan-brumme.com/smallz4/
 *
 */

#ifndef _LIB_H
#define _LIB_H

#include "divsufsort.h"
#include "stream.h"

/** High level status for compression and decompression */
typedef enum {
   LZSA_OK = 0,                           /**< Success */
   LZSA_ERROR_SRC,                        /**< Error reading input */
   LZSA_ERROR_DST,                        /**< Error reading output */
   LZSA_ERROR_DICTIONARY,                 /**< Error reading dictionary */
   LZSA_ERROR_MEMORY,                     /**< Out of memory */

   /* Compression-specific status codes */
   LZSA_ERROR_COMPRESSION,                /**< Internal compression error */
   LZSA_ERROR_RAW_TOOLARGE,               /**< Input is too large to be compressed to a raw block */
   LZSA_ERROR_RAW_UNCOMPRESSED,           /**< Input is incompressible and raw blocks don't support uncompressed data */

   /* Decompression-specific status codes */
   LZSA_ERROR_FORMAT,                     /**< Invalid input format or magic number when decompressing */
   LZSA_ERROR_DECOMPRESSION,              /**< Internal decompression error */
} lzsa_status_t;

/* Compression flags */
#define LZSA_FLAG_FAVOR_RATIO    (1<<0)      /**< 1 to compress with the best ratio, 0 to trade some compression ratio for extra decompression speed */
#define LZSA_FLAG_RAW_BLOCK      (1<<1)      /**< 1 to emit raw block */

/*-------------- Top level API -------------- */

/**
 * Compress file
 *
 * @param pszInFilename name of input(source) file to compress
 * @param pszOutFilename name of output(compressed) file to generate
 * @param pszDictionaryFilename name of dictionary file, or NULL for none
 * @param nFlags compression flags (LZSA_FLAG_xxx)
 * @param nMinMatchSize minimum match size
 * @param nFormatVersion version of format to use (1-2)
 * @param progress progress function, called after compressing each block, or NULL for none
 * @param pOriginalSize pointer to returned input(source) size, updated when this function is successful
 * @param pCompressedSize pointer to returned output(compressed) size, updated when this function is successful
 * @param pCommandCount pointer to returned token(compression commands) count, updated when this function is successful
 *
 * @return LZSA_OK for success, or an error value from lzsa_status_t
 */
lzsa_status_t lsza_compress_file(const char *pszInFilename, const char *pszOutFilename, const char *pszDictionaryFilename,
   const unsigned int nFlags, const int nMinMatchSize, const int nFormatVersion,
   void(*progress)(long long nOriginalSize, long long nCompressedSize), long long *pOriginalSize, long long *pCompressedSize, int *pCommandCount);

/**
 * Decompress file
 *
 * @param pszInFilename name of input(compressed) file to decompress
 * @param pszOutFilename name of output(decompressed) file to generate
 * @param pszDictionaryFilename name of dictionary file, or NULL for none
 * @param nFlags compression flags (LZSA_FLAG_RAW_BLOCK to decompress a raw block, or 0)
 * @param nFormatVersion default version of format to use (1-2). This is used when decompressing a raw block, otherwise the version is extracted from the source file
 * @param pOriginalSize pointer to returned output(decompressed) size, updated when this function is successful
 * @param pCompressedSize pointer to returned input(compressed) size, updated when this function is successful
 *
 * @return LZSA_OK for success, or an error value from lzsa_status_t
 */
lzsa_status_t lzsa_decompress_file(const char *pszInFilename, const char *pszOutFilename, const char *pszDictionaryFilename, const unsigned int nFlags, int nFormatVersion,
   long long *pOriginalSize, long long *pCompressedSize);

/*-------------- Streaming API -------------- */

/**
 * Load dictionary contents
 *
 * @param pszDictionaryFilename name of dictionary file, or NULL for none
 * @param pDictionaryData pointer to returned dictionary contents, or NULL for none
 * @param nDictionaryDataSize pointer to returned size of dictionary contents, or 0
 *
 * @return LZSA_OK for success, or an error value from lzsa_status_t
 */
int lzsa_dictionary_load(const char *pszDictionaryFilename, void **ppDictionaryData, int *pDictionaryDataSize);

/**
 * Free dictionary contents
 *
 * @param pDictionaryData pointer to pointer to dictionary contents
 */
void lzsa_dictionary_free(void **ppDictionaryData);

/**
 * Compress stream
 *
 * @param pInStream input(source) stream to compress
 * @param pOutStream output(compressed) stream to write to
 * @param pDictionaryData dictionary contents, or NULL for none
 * @param nDictionaryDataSize size of dictionary contents, or 0
 * @param nFlags compression flags (LZSA_FLAG_xxx)
 * @param nMinMatchSize minimum match size
 * @param nFormatVersion version of format to use (1-2)
 * @param progress progress function, called after compressing each block, or NULL for none
 * @param pOriginalSize pointer to returned input(source) size, updated when this function is successful
 * @param pCompressedSize pointer to returned output(compressed) size, updated when this function is successful
 * @param pCommandCount pointer to returned token(compression commands) count, updated when this function is successful
 *
 * @return LZSA_OK for success, or an error value from lzsa_status_t
 */
lzsa_status_t lsza_compress_stream(lzsa_stream_t *pInStream, lzsa_stream_t *pOutStream, const void *pDictionaryData, int nDictionaryDataSize,
   const unsigned int nFlags, const int nMinMatchSize, const int nFormatVersion,
   void(*progress)(long long nOriginalSize, long long nCompressedSize), long long *pOriginalSize, long long *pCompressedSize, int *pCommandCount);

/**
 * Decompress stream
 *
 * @param pInStream input(compressed) stream to decompress
 * @param pOutStream output(decompressed) stream to write to
 * @param pDictionaryData dictionary contents, or NULL for none
 * @param nDictionaryDataSize size of dictionary contents, or 0
 * @param nFlags compression flags (LZSA_FLAG_RAW_BLOCK to decompress a raw block, or 0)
 * @param nFormatVersion default version of format to use (1-2). This is used when decompressing a raw block, otherwise the version is extracted from the source file
 * @param pOriginalSize pointer to returned output(decompressed) size, updated when this function is successful
 * @param pCompressedSize pointer to returned input(compressed) size, updated when this function is successful
 *
 * @return LZSA_OK for success, or an error value from lzsa_status_t
 */
lzsa_status_t lzsa_decompress_stream(lzsa_stream_t *pInStream, lzsa_stream_t *pOutStream, const void *pDictionaryData, int nDictionaryDataSize, const unsigned int nFlags, int nFormatVersion,
   long long *pOriginalSize, long long *pCompressedSize);

/*-------------- Block compression API --------------*/

#define LCP_BITS 15
#define LCP_MAX (1<<(LCP_BITS - 1))
#define LCP_SHIFT (32-LCP_BITS)
#define LCP_MASK (((1<<LCP_BITS) - 1) << LCP_SHIFT)
#define POS_MASK ((1<<LCP_SHIFT) - 1)

#define NMATCHES_PER_OFFSET 8
#define MATCHES_PER_OFFSET_SHIFT 3

#define LEAVE_ALONE_MATCH_SIZE 1000

#define LAST_MATCH_OFFSET 4
#define LAST_LITERALS 1

#define MODESWITCH_PENALTY 1

/** One match */
typedef struct _lzsa_match {
   unsigned short length;
   unsigned short offset;
} lzsa_match;

/** One rep-match slot (for LZSA2) */
typedef struct _lzsa_repmatch_opt {
   int incoming_offset;
   short best_slot_for_incoming;
   short expected_repmatch;
} lzsa_repmatch_opt;

/** Compression context */
typedef struct _lsza_compressor {
   divsufsort_ctx_t divsufsort_context;
   unsigned int *intervals;
   unsigned int *pos_data;
   unsigned int *open_intervals;
   lzsa_match *match;
   lzsa_match *best_match;
   int *slot_cost;
   lzsa_repmatch_opt *repmatch_opt;
   int min_match_size;
   int format_version;
   int flags;
   int num_commands;
} lsza_compressor;

/**
 * Initialize compression context
 *
 * @param pCompressor compression context to initialize
 * @param nMaxWindowSize maximum size of input data window (previously compressed bytes + bytes to compress)
 * @param nMinMatchSize minimum match size (cannot be less than MIN_MATCH_SIZE)
 * @param nFlags compression flags
 *
 * @return 0 for success, non-zero for failure
 */
int lzsa_compressor_init(lsza_compressor *pCompressor, const int nMaxWindowSize, const int nMinMatchSize, const int nFormatVersion, const int nFlags);

/**
 * Clean up compression context and free up any associated resources
 *
 * @param pCompressor compression context to clean up
 */
void lzsa_compressor_destroy(lsza_compressor *pCompressor);

/**
 * Compress one block of data
 *
 * @param pCompressor compression context
 * @param pInWindow pointer to input data window (previously compressed bytes + bytes to compress)
 * @param nPreviousBlockSize number of previously compressed bytes (or 0 for none)
 * @param nInDataSize number of input bytes to compress
 * @param pOutData pointer to output buffer
 * @param nMaxOutDataSize maximum size of output buffer, in bytes
 *
 * @return size of compressed data in output buffer, or -1 if the data is uncompressible
 */
int lzsa_compressor_shrink_block(lsza_compressor *pCompressor, const unsigned char *pInWindow, const int nPreviousBlockSize, const int nInDataSize, unsigned char *pOutData, const int nMaxOutDataSize);

/**
 * Get the number of compression commands issued in compressed data blocks
 *
 * @return number of commands
 */
int lzsa_compressor_get_command_count(lsza_compressor *pCompressor);

/**
 * Decompress one data block
 *
 * @param pInBlock pointer to compressed data
 * @param nInBlockSize size of compressed data, in bytes
 * @param pOutData pointer to output decompression buffer (previously decompressed bytes + room for decompressing this block)
 * @param nOutDataOffset starting index of where to store decompressed bytes in output buffer (and size of previously decompressed bytes)
 * @param nBlockMaxSize total size of output decompression buffer, in bytes
 *
 * @return size of decompressed data in bytes, or -1 for error
 */
int lzsa_decompressor_expand_block(const int nFormatVersion, const unsigned char *pInBlock, int nBlockSize, unsigned char *pOutData, int nOutDataOffset, int nBlockMaxSize);

#endif /* _LIB_H */