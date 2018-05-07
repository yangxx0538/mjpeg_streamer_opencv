/*******************************************************************************
# Linux-UVC streaming input-plugin for MJPG-streamer                           #
#                                                                              #
# This package work with the Logitech UVC based webcams with the mjpeg feature #
#                                                                              #
#   Orginally Copyright (C) 2005 2006 Laurent Pinchart &&  Michel Xhaard       #
#   Modifications Copyright (C) 2006  Gabriel A. Devenyi                       #
#   Modifications Copyright (C) 2007  Tom Stöveken                             #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; either version 2 of the License, or            #
# (at your option) any later version.                                          #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/

#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include "TTYIO.h"
#include "v4l2uvc.h"

#define OUTPUT_BUF_SIZE  4096

typedef struct {
    struct jpeg_destination_mgr pub; /* public fields */

    JOCTET * buffer;    /* start of buffer */

    unsigned char *outbuffer;
    int outbuffer_size;
    unsigned char *outbuffer_cursor;
    int *written;

} mjpg_destination_mgr;

typedef mjpg_destination_mgr * mjpg_dest_ptr;

/******************************************************************************
Description.:
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    /* Allocate the output buffer --- it will be released when done with image */
    dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));

    *(dest->written) = 0;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/******************************************************************************
Description.: called whenever local jpeg buffer fills up
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;

    memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
    dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
    *(dest->written) += OUTPUT_BUF_SIZE;

    dest->pub.next_output_byte = dest->buffer;
    dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

    return TRUE;
}

/******************************************************************************
Description.: called by jpeg_finish_compress after all data has been written.
              Usually needs to flush buffer.
Input Value.:
Return Value:
******************************************************************************/
METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
    mjpg_dest_ptr dest = (mjpg_dest_ptr) cinfo->dest;
    size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

    /* Write any data remaining in the buffer */
    memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
    dest->outbuffer_cursor += datacount;
    *(dest->written) += datacount;
}

/******************************************************************************
Description.: Prepare for output to a stdio stream.
Input Value.: buffer is the already allocated buffer memory that will hold
              the compressed picture. "size" is the size in bytes.
Return Value: -
******************************************************************************/
GLOBAL(void) dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
    mjpg_dest_ptr dest;

    if(cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(mjpg_destination_mgr));
    }

    dest = (mjpg_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination = term_destination;
    dest->outbuffer = buffer;
    dest->outbuffer_size = size;
    dest->outbuffer_cursor = buffer;
    dest->written = written;
}

/******************************************************************************
Description.: yuv2jpeg function is based on compress_yuv_to_jpeg written by
              Gabriel A. Devenyi.
              It uses the destination manager implemented above to compress
              YUYV data to JPEG. Most other implementations use the
              "jpeg_stdio_dest" from libjpeg, which can not store compressed
              pictures to memory instead of a file.
Input Value.: video structure from v4l2uvc.c/h, destination buffer and buffersize
              the buffer must be large enough, no error/size checking is done!
Return Value: the buffer will contain the compressed data
******************************************************************************/
int compress_yuv_to_jpeg(struct vdIn *vd, unsigned char *buffer, int size, int quality, int fmt)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    unsigned char *line_buffer, *yuyv;
    int z;
    static int written;
    double totel_g_sum;
    double totel_g_num;
    unsigned char send_buf[6];
    int offset;
    line_buffer = calloc(vd->width * 3, 1);
    yuyv = vd->framebuffer;
    send_buf[0] = 0x7F;
    send_buf[5] = 0xF7;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    /* jpeg_stdio_dest (&cinfo, file); */
    dest_buffer(&cinfo, buffer, size, &written);

    cinfo.image_width = vd->width;
    cinfo.image_height = vd->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    z = 0;
    DBG("height:%d,wdith:%d\n",vd->height,vd->height);
    while(cinfo.next_scanline < vd->height)   //240
    {
        int x;
        unsigned char *ptr = line_buffer;
        for(x = 0; x < vd->width; x++)    //320
        {
            int r, g, b;
            int y, u, v;
            switch(fmt)
            {
                case V4L2_PIX_FMT_YUYV:   //my  fmt
                    if(!z)
                    y = yuyv[0] << 8;
                    else
                    y = yuyv[2] << 8;
                    u = yuyv[1] - 128;
                    v = yuyv[3] - 128;
                break;
                case V4L2_PIX_FMT_YVYU:
                    if(!z)
                    y = yuyv[0] << 8;
                    else
                    y = yuyv[2] << 8;
                    v = yuyv[1] - 128;
                    u = yuyv[3] - 128;
                break;
                case V4L2_PIX_FMT_UYVY:
                    if(!z)
                    y = yuyv[1] << 8;
                    else
                    y = yuyv[3] << 8;
                    u = yuyv[0] - 128;
                    v = yuyv[2] - 128;
                break;
                case V4L2_PIX_FMT_VYUY:
                    if(!z)
                    y = yuyv[1] << 8;
                    else
                    y = yuyv[3] << 8;
                    v = yuyv[0] - 128;
                    u = yuyv[2] - 128;
                break;
            }

            r = (y + (359 * v)) >> 8;
            g = (y - (88 * u) - (183 * v)) >> 8;
            b = (y + (454 * u)) >> 8;

            float color_value;
            color_value = g/(r+g+b);
			
            if((color_value > 0.4)&&(r+g+b >180))
            {
              totel_g_sum += x;
              totel_g_num ++;
              r = g = b = 250;
            }


            *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
            *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
            *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

            if(z++)
            {
                z = 0;
                yuyv += 4;
            }
        }

        row_pointer[0] = line_buffer;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
	printf("total_g_num = %3f\n",totel_g_num);
    offset = (int)(totel_g_sum/totel_g_num - 160);
    send_buf[1] = offset >> 8;
    send_buf[2] = offset & 0xFF;
    send_buf[3] = (short int)(totel_g_num) >> 8;
    send_buf[4] = (short int)(totel_g_num) & 0xFF;
    
    TTY_Sendn(info, (char *)send_buf, 6);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    free(line_buffer);

    return (written);
}
