#pragma once

#include "types.h"

/**
 * Describes a file located through the CD/DVD library.
 */
struct sceCdlFILE {
    u_int lsn;      /**< Starting logical sector number. */
    u_int size;     /**< File size in bytes. */
    char name[16];  /**< ISO 9660 file name. */
    u_char date[8]; /**< ISO 9660 recording date. */
    u_int flag;     /**< ISO 9660 file flags. */
};

/**
 * Configures a CD/DVD read request.
 */
struct sceCdRMode {
    u_char trycount;    /**< Number of retry attempts. */
    u_char spindlctrl;  /**< Spindle control mode. */
    u_char datapattern; /**< Requested sector data pattern. */
    u_char pad;         /**< Structure padding. */
};

extern "C" {
int sceCdInit(int mode);
int sceCdSeek(u_int lsn);
int sceCdMmode(int media);
int sceCdSearchFile(sceCdlFILE *file, const char *name);
int sceCdRead(u_int lsn, u_int sectors, void *buffer, sceCdRMode *mode);
int sceCdSync(int mode);
int sceCdGetError(void);
int sceCdBreak(void);
/**
 * Waits for the disc drive to become ready.
 */
int sceCdDiskReady(int mode);

/**
 * Initializes the CD streaming ring in IOP memory.
 */
int sceCdStInit(int sectors, int banks, void *buffer);

/**
 * Starts streaming at a logical sector.
 */
int sceCdStStart(u_int sector, sceCdRMode *mode);

/**
 * Seeks the CD stream to a logical sector.
 */
int sceCdStSeekF(u_int sector);

/**
 * Stops CD streaming.
 */
int sceCdStStop(void);

/**
 * Reads sectors from the CD streaming ring.
 */
int sceCdStRead(u_int sectors, void *buffer, u_int mode, u_int *error);
}
