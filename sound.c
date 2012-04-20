/* esdplay.c - part of esdplay
 * Copyright (C) 1998 Simon Kågedal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the file COPYING for more details.
 */

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <audiofile.h>
#include <esd.h>

int play_file (char *filename) {
  char buf[ESD_BUF_SIZE];
  int buf_frames;
  int frames_read = 0, bytes_written = 0;

  AFfilehandle in_file;
  int in_format, in_width, in_channels, frame_count;
  double in_rate;
  int bytes_per_frame;

  int out_sock, out_bits, out_channels, out_rate;
  int out_mode = ESD_STREAM, out_func = ESD_PLAY;
  esd_format_t out_format;

  in_file = afOpenFile(filename, "r", NULL);

  if (!in_file) {
	free(filename);
    	return 1;
  }

  frame_count = afGetFrameCount (in_file, AF_DEFAULT_TRACK);
  in_channels = afGetChannels (in_file, AF_DEFAULT_TRACK);
  in_rate = afGetRate (in_file, AF_DEFAULT_TRACK);
  afGetSampleFormat (in_file, AF_DEFAULT_TRACK, &in_format, &in_width);

  afSetVirtualByteOrder (in_file, AF_DEFAULT_TRACK, AF_BYTEORDER_LITTLEENDIAN);

  if (in_width == 8)
    out_bits = ESD_BITS8;
  else if (in_width == 16)
    out_bits = ESD_BITS16;
  else {
      fputs ("only sample widths of 8 and 16 supported\n", stderr);

      free(filename);
      afCloseFile (in_file);   
      return 1;
  }

  bytes_per_frame = (in_width  * in_channels) / 8;

  if (in_channels == 1)
    out_channels = ESD_MONO;
  else if (in_channels == 2)
    out_channels = ESD_STEREO;
  else {
      fputs ("only 1 or 2 channel samples supported\n", stderr);

      free(filename);
      afCloseFile (in_file);   

      return 1;
  }

  out_format = out_bits | out_channels | out_mode | out_func;
  out_rate = (int) in_rate;
  out_sock = esd_play_stream_fallback (out_format, out_rate, NULL, (char *) filename);

  if (out_sock <= 0) {
    free(filename);
    afCloseFile (in_file);
    close (out_sock);

    return 1;
  }

  buf_frames = ESD_BUF_SIZE / bytes_per_frame;

  while ((frames_read = afReadFrames(in_file, AF_DEFAULT_TRACK, buf, buf_frames))) {
      bytes_written += frames_read * bytes_per_frame;
      
      if (write (out_sock, buf, frames_read * bytes_per_frame) <= 0)
	break;
  }

  free(filename);

  close (out_sock);

  if (afCloseFile (in_file))
    return 1;

  return 0;
}
	
