/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#include <config.h>

#include <string.h>
#include <stdbool.h>

#include "media/media.h"
#include "fnc_log.h"

#include <libavutil/md5.h>

typedef struct {
    int stacksize;
    int stackcount;
    unsigned char* framestack;
} framestack;

//! Parse extradata and reformat it, most of the code is shamelessly ripped from ffvorbis.
static int encode_header(uint8_t *data, int len, xiph_priv *priv)
{
    uint8_t *headers = data;
    int headers_len = len, i , j;
    uint8_t *header_start[3];
    int header_len[3];
    int hash[4];
    static const uint8_t comment[26] =
        /*quite minimal comment */
    { 3, 'v', 'o', 'r', 'b', 'i', 's',
        10, 0, 0, 0,
        'v', 'o', 'r', 'b', 'i', 's', '-', 'r', 't', 'p',
        0, 0, 0, 0,
        1
    };

// old way.
    if(headers[0] == 0 && headers[1] == 30) {
        for(i = 0; i < 3; i++){
            header_len[i] = *headers++ << 8;
            header_len[i] += *headers++;
            header_start[i] = headers;
            headers += header_len[i];
        }
// xiphlaced
    } else if(headers[0] == 2) {
        for(j=1, i=0; i<2 ; ++i, ++j) {
            header_len[i]=0;
            while(j<headers_len && headers[j]==0xff) {
                header_len[i]+=0xff;
                ++j;
            }
            if (j>=headers_len) {
                fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
                return -1;
            }
            header_len[i]+=headers[j];
        }
        header_len[2] = headers_len - header_len[0] - header_len[1] - j;
        headers += j;
        header_start[0] = headers;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
        return -1;
    }
    if (header_len[2] + header_len[0]>UINT16_MAX) {
        return -1;
    }

    av_md5_sum((uint8_t *)hash, data, len);
    priv->ident = hash[0]^hash[1]^hash[2]^hash[3];

    // Envelope size
    headers_len = header_len[0] + sizeof(comment) + header_len[2];
    priv->conf_len = 4 +                // count field
                     3 +                // Ident field
                     2 +                // pack size
                     1 +                // headers count (2)
                     2 +                // headers sizes
                     headers_len;       // the rest

    priv->conf = g_malloc(priv->conf_len);
    priv->conf[0] = priv->conf[1] = priv->conf[2] = 0;
    priv->conf[3] = 1; //just one packet for now
    // new config
    priv->conf[4] = (priv->ident >> 16) & 0xff;
    priv->conf[5] = (priv->ident >> 8) & 0xff;
    priv->conf[6] = priv->ident & 0xff;
    priv->conf[7] = (headers_len)>>8;
    priv->conf[8] = (headers_len) & 0xff;
    priv->conf[9] = 2;
    priv->conf[10] = header_len[0];     // 30, always
    priv->conf[11] = sizeof(comment);   // 26
    memcpy(priv->conf + 12, header_start[0], header_len[0]);
    memcpy(priv->conf + 12 + header_len[0], comment, sizeof(comment));
    memcpy(priv->conf + 12 + header_len[0] + sizeof(comment), header_start[2],
           header_len[2]);
    return 0;
}

int vorbis_init(Track *track)
{
    xiph_priv *priv;

    priv = g_slice_new(xiph_priv);

    if ( encode_header(track->extradata,
                       track->extradata_len, priv) != 0 )
        goto err_alloc;

    track->private_data = priv;
    xiph_sdp_descr_append(track);

    return 0;

 err_alloc:
    g_free(priv->conf);
    g_free(priv->packet);
    g_slice_free(xiph_priv, priv);
    return -1;
}