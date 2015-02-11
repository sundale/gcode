/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

#ifndef EMC2_CONFIG_H
#define EMC2_CONFIG_H

/* LINELEN is used throughout for buffer sizes, length of file name strings,
   etc. Let's just have one instead of a multitude of defines all the same. */
#define LINELEN 255
/* Used in a number of places for sprintf() buffers. */
#define BUFFERLEN 80

#define MM_PER_INCH 25.4
#define INCH_PER_MM (1.0/25.4)

#define TOOL_CONFIGURE_PATH "./tool/sim.tbl"

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif


#endif

