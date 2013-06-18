/*
 * Copyright (c) 2012 Vladimir Serbinenko
 * Copyright (c) 2012 Thomas Schmitt
 * 
 * This file is part of the libisofs project; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * or later as published by the Free Software Foundation.
 * See COPYING file for details.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <string.h>

#include "hfsplus.h"

/*
  Based on tn1150 (HFS+ format specification)
*/


/* This encodes a matrix of page and character, with a number of
   HFSPLUS_MAX_DECOMPOSE_LEN + 1 elements of 16 bit each.
   Initially the matrix is filled with zeros.
   1: The first element is the page number.
      If it is equal or lower than the previous one, then the matrix is done.
   2: The next element is the character number
      If it is  equal or lower than the previous one, the page is done. Goto 1.
   3: The next elements are matrix elements.
      If element 0 is encountered, then the character is done. Goto 2.
*/

static const uint16_t decompose_page_data[] = {

  /* page00 */
  0x00,
  0xc0, 0x0041, 0x0300, 0,
  0xc1, 0x0041, 0x0301, 0,
  0xc2, 0x0041, 0x0302, 0,
  0xc3, 0x0041, 0x0303, 0,
  0xc4, 0x0041, 0x0308, 0,
  0xc5, 0x0041, 0x030A, 0,
  0xc7, 0x0043, 0x0327, 0,
  0xc8, 0x0045, 0x0300, 0,
  0xc9, 0x0045, 0x0301, 0,
  0xca, 0x0045, 0x0302, 0,
  0xcb, 0x0045, 0x0308, 0,
  0xcc, 0x0049, 0x0300, 0,
  0xcd, 0x0049, 0x0301, 0,
  0xce, 0x0049, 0x0302, 0,
  0xcf, 0x0049, 0x0308, 0,
  0xd1, 0x004E, 0x0303, 0,
  0xd2, 0x004F, 0x0300, 0,
  0xd3, 0x004F, 0x0301, 0,
  0xd4, 0x004F, 0x0302, 0,
  0xd5, 0x004F, 0x0303, 0,
  0xd6, 0x004F, 0x0308, 0,
  0xd9, 0x0055, 0x0300, 0,
  0xda, 0x0055, 0x0301, 0,
  0xdb, 0x0055, 0x0302, 0,
  0xdc, 0x0055, 0x0308, 0,
  0xdd, 0x0059, 0x0301, 0,
  0xe0, 0x0061, 0x0300, 0,
  0xe1, 0x0061, 0x0301, 0,
  0xe2, 0x0061, 0x0302, 0,
  0xe3, 0x0061, 0x0303, 0,
  0xe4, 0x0061, 0x0308, 0,
  0xe5, 0x0061, 0x030A, 0,
  0xe7, 0x0063, 0x0327, 0,
  0xe8, 0x0065, 0x0300, 0,
  0xe9, 0x0065, 0x0301, 0,
  0xea, 0x0065, 0x0302, 0,
  0xeb, 0x0065, 0x0308, 0,
  0xec, 0x0069, 0x0300, 0,
  0xed, 0x0069, 0x0301, 0,
  0xee, 0x0069, 0x0302, 0,
  0xef, 0x0069, 0x0308, 0,
  0xf1, 0x006E, 0x0303, 0,
  0xf2, 0x006F, 0x0300, 0,
  0xf3, 0x006F, 0x0301, 0,
  0xf4, 0x006F, 0x0302, 0,
  0xf5, 0x006F, 0x0303, 0,
  0xf6, 0x006F, 0x0308, 0,
  0xf9, 0x0075, 0x0300, 0,
  0xfa, 0x0075, 0x0301, 0,
  0xfb, 0x0075, 0x0302, 0,
  0xfc, 0x0075, 0x0308, 0,
  0xfd, 0x0079, 0x0301, 0,
  0xff, 0x0079, 0x0308, 0,
  0x00,

  /* page01 */
  0x01,
  0x00, 0x0041, 0x0304, 0,
  0x01, 0x0061, 0x0304, 0,
  0x02, 0x0041, 0x0306, 0,
  0x03, 0x0061, 0x0306, 0,
  0x04, 0x0041, 0x0328, 0,
  0x05, 0x0061, 0x0328, 0,
  0x06, 0x0043, 0x0301, 0,
  0x07, 0x0063, 0x0301, 0,
  0x08, 0x0043, 0x0302, 0,
  0x09, 0x0063, 0x0302, 0,
  0x0a, 0x0043, 0x0307, 0,
  0x0b, 0x0063, 0x0307, 0,
  0x0c, 0x0043, 0x030C, 0,
  0x0d, 0x0063, 0x030C, 0,
  0x0e, 0x0044, 0x030C, 0,
  0x0f, 0x0064, 0x030C, 0,
  0x12, 0x0045, 0x0304, 0,
  0x13, 0x0065, 0x0304, 0,
  0x14, 0x0045, 0x0306, 0,
  0x15, 0x0065, 0x0306, 0,
  0x16, 0x0045, 0x0307, 0,
  0x17, 0x0065, 0x0307, 0,
  0x18, 0x0045, 0x0328, 0,
  0x19, 0x0065, 0x0328, 0,
  0x1a, 0x0045, 0x030C, 0,
  0x1b, 0x0065, 0x030C, 0,
  0x1c, 0x0047, 0x0302, 0,
  0x1d, 0x0067, 0x0302, 0,
  0x1e, 0x0047, 0x0306, 0,
  0x1f, 0x0067, 0x0306, 0,
  0x20, 0x0047, 0x0307, 0,
  0x21, 0x0067, 0x0307, 0,
  0x22, 0x0047, 0x0327, 0,
  0x23, 0x0067, 0x0327, 0,
  0x24, 0x0048, 0x0302, 0,
  0x25, 0x0068, 0x0302, 0,
  0x28, 0x0049, 0x0303, 0,
  0x29, 0x0069, 0x0303, 0,
  0x2a, 0x0049, 0x0304, 0,
  0x2b, 0x0069, 0x0304, 0,
  0x2c, 0x0049, 0x0306, 0,
  0x2d, 0x0069, 0x0306, 0,
  0x2e, 0x0049, 0x0328, 0,
  0x2f, 0x0069, 0x0328, 0,
  0x30, 0x0049, 0x0307, 0,
  0x34, 0x004A, 0x0302, 0,
  0x35, 0x006A, 0x0302, 0,
  0x36, 0x004B, 0x0327, 0,
  0x37, 0x006B, 0x0327, 0,
  0x39, 0x004C, 0x0301, 0,
  0x3a, 0x006C, 0x0301, 0,
  0x3b, 0x004C, 0x0327, 0,
  0x3c, 0x006C, 0x0327, 0,
  0x3d, 0x004C, 0x030C, 0,
  0x3e, 0x006C, 0x030C, 0,
  0x43, 0x004E, 0x0301, 0,
  0x44, 0x006E, 0x0301, 0,
  0x45, 0x004E, 0x0327, 0,
  0x46, 0x006E, 0x0327, 0,
  0x47, 0x004E, 0x030C, 0,
  0x48, 0x006E, 0x030C, 0,
  0x4c, 0x004F, 0x0304, 0,
  0x4d, 0x006F, 0x0304, 0,
  0x4e, 0x004F, 0x0306, 0,
  0x4f, 0x006F, 0x0306, 0,
  0x50, 0x004F, 0x030B, 0,
  0x51, 0x006F, 0x030B, 0,
  0x54, 0x0052, 0x0301, 0,
  0x55, 0x0072, 0x0301, 0,
  0x56, 0x0052, 0x0327, 0,
  0x57, 0x0072, 0x0327, 0,
  0x58, 0x0052, 0x030C, 0,
  0x59, 0x0072, 0x030C, 0,
  0x5a, 0x0053, 0x0301, 0,
  0x5b, 0x0073, 0x0301, 0,
  0x5c, 0x0053, 0x0302, 0,
  0x5d, 0x0073, 0x0302, 0,
  0x5e, 0x0053, 0x0327, 0,
  0x5f, 0x0073, 0x0327, 0,
  0x60, 0x0053, 0x030C, 0,
  0x61, 0x0073, 0x030C, 0,
  0x62, 0x0054, 0x0327, 0,
  0x63, 0x0074, 0x0327, 0,
  0x64, 0x0054, 0x030C, 0,
  0x65, 0x0074, 0x030C, 0,
  0x68, 0x0055, 0x0303, 0,
  0x69, 0x0075, 0x0303, 0,
  0x6a, 0x0055, 0x0304, 0,
  0x6b, 0x0075, 0x0304, 0,
  0x6c, 0x0055, 0x0306, 0,
  0x6d, 0x0075, 0x0306, 0,
  0x6e, 0x0055, 0x030A, 0,
  0x6f, 0x0075, 0x030A, 0,
  0x70, 0x0055, 0x030B, 0,
  0x71, 0x0075, 0x030B, 0,
  0x72, 0x0055, 0x0328, 0,
  0x73, 0x0075, 0x0328, 0,
  0x74, 0x0057, 0x0302, 0,
  0x75, 0x0077, 0x0302, 0,
  0x76, 0x0059, 0x0302, 0,
  0x77, 0x0079, 0x0302, 0,
  0x78, 0x0059, 0x0308, 0,
  0x79, 0x005A, 0x0301, 0,
  0x7a, 0x007A, 0x0301, 0,
  0x7b, 0x005A, 0x0307, 0,
  0x7c, 0x007A, 0x0307, 0,
  0x7d, 0x005A, 0x030C, 0,
  0x7e, 0x007A, 0x030C, 0,
  0xa0, 0x004F, 0x031B, 0,
  0xa1, 0x006F, 0x031B, 0,
  0xaf, 0x0055, 0x031B, 0,
  0xb0, 0x0075, 0x031B, 0,
  0xcd, 0x0041, 0x030C, 0,
  0xce, 0x0061, 0x030C, 0,
  0xcf, 0x0049, 0x030C, 0,
  0xd0, 0x0069, 0x030C, 0,
  0xd1, 0x004F, 0x030C, 0,
  0xd2, 0x006F, 0x030C, 0,
  0xd3, 0x0055, 0x030C, 0,
  0xd4, 0x0075, 0x030C, 0,
  0xd5, 0x0055, 0x0308, 0x0304, 0,
  0xd6, 0x0075, 0x0308, 0x0304, 0,
  0xd7, 0x0055, 0x0308, 0x0301, 0,
  0xd8, 0x0075, 0x0308, 0x0301, 0,
  0xd9, 0x0055, 0x0308, 0x030C, 0,
  0xda, 0x0075, 0x0308, 0x030C, 0,
  0xdb, 0x0055, 0x0308, 0x0300, 0,
  0xdc, 0x0075, 0x0308, 0x0300, 0,
  0xde, 0x0041, 0x0308, 0x0304, 0,
  0xdf, 0x0061, 0x0308, 0x0304, 0,
  0xe0, 0x0041, 0x0307, 0x0304, 0,
  0xe1, 0x0061, 0x0307, 0x0304, 0,
  0xe2, 0x00C6, 0x0304, 0,
  0xe3, 0x00E6, 0x0304, 0,
  0xe6, 0x0047, 0x030C, 0,
  0xe7, 0x0067, 0x030C, 0,
  0xe8, 0x004B, 0x030C, 0,
  0xe9, 0x006B, 0x030C, 0,
  0xea, 0x004F, 0x0328, 0,
  0xeb, 0x006F, 0x0328, 0,
  0xec, 0x004F, 0x0328, 0x0304, 0,
  0xed, 0x006F, 0x0328, 0x0304, 0,
  0xee, 0x01B7, 0x030C, 0,
  0xef, 0x0292, 0x030C, 0,
  0xf0, 0x006A, 0x030C, 0,
  0xf4, 0x0047, 0x0301, 0,
  0xf5, 0x0067, 0x0301, 0,
  0xfa, 0x0041, 0x030A, 0x0301, 0,
  0xfb, 0x0061, 0x030A, 0x0301, 0,
  0xfc, 0x00C6, 0x0301, 0,
  0xfd, 0x00E6, 0x0301, 0,
  0xfe, 0x00D8, 0x0301, 0,
  0xff, 0x00F8, 0x0301, 0,
  0x00,

  /* page02 */
  0x02,
  0x00, 0x0041, 0x030F, 0,
  0x01, 0x0061, 0x030F, 0,
  0x02, 0x0041, 0x0311, 0,
  0x03, 0x0061, 0x0311, 0,
  0x04, 0x0045, 0x030F, 0,
  0x05, 0x0065, 0x030F, 0,
  0x06, 0x0045, 0x0311, 0,
  0x07, 0x0065, 0x0311, 0,
  0x08, 0x0049, 0x030F, 0,
  0x09, 0x0069, 0x030F, 0,
  0x0a, 0x0049, 0x0311, 0,
  0x0b, 0x0069, 0x0311, 0,
  0x0c, 0x004F, 0x030F, 0,
  0x0d, 0x006F, 0x030F, 0,
  0x0e, 0x004F, 0x0311, 0,
  0x0f, 0x006F, 0x0311, 0,
  0x10, 0x0052, 0x030F, 0,
  0x11, 0x0072, 0x030F, 0,
  0x12, 0x0052, 0x0311, 0,
  0x13, 0x0072, 0x0311, 0,
  0x14, 0x0055, 0x030F, 0,
  0x15, 0x0075, 0x030F, 0,
  0x16, 0x0055, 0x0311, 0,
  0x17, 0x0075, 0x0311, 0,
  0x00,

  /* page03 */
  0x03,
  0x10, 0x0306, 0x0307, 0,
  0x40, 0x0300, 0,
  0x41, 0x0301, 0,
  0x43, 0x0313, 0,
  0x44, 0x0308, 0x030D, 0,
  0x74, 0x02B9, 0,
  0x7e, 0x003B, 0,
  0x85, 0x00A8, 0x030D, 0,
  0x86, 0x0391, 0x030D, 0,
  0x87, 0x00B7, 0,
  0x88, 0x0395, 0x030D, 0,
  0x89, 0x0397, 0x030D, 0,
  0x8a, 0x0399, 0x030D, 0,
  0x8c, 0x039F, 0x030D, 0,
  0x8e, 0x03A5, 0x030D, 0,
  0x8f, 0x03A9, 0x030D, 0,
  0x90, 0x03B9, 0x0308, 0x030D, 0,
  0xaa, 0x0399, 0x0308, 0,
  0xab, 0x03A5, 0x0308, 0,
  0xac, 0x03B1, 0x030D, 0,
  0xad, 0x03B5, 0x030D, 0,
  0xae, 0x03B7, 0x030D, 0,
  0xaf, 0x03B9, 0x030D, 0,
  0xb0, 0x03C5, 0x0308, 0x030D, 0,
  0xca, 0x03B9, 0x0308, 0,
  0xcb, 0x03C5, 0x0308, 0,
  0xcc, 0x03BF, 0x030D, 0,
  0xcd, 0x03C5, 0x030D, 0,
  0xce, 0x03C9, 0x030D, 0,
  0xd3, 0x03D2, 0x030D, 0,
  0xd4, 0x03D2, 0x0308, 0,
  0x00,

  /* page04 */
  0x04,
  0x01, 0x0415, 0x0308, 0,
  0x03, 0x0413, 0x0301, 0,
  0x07, 0x0406, 0x0308, 0,
  0x0c, 0x041A, 0x0301, 0,
  0x0e, 0x0423, 0x0306, 0,
  0x19, 0x0418, 0x0306, 0,
  0x39, 0x0438, 0x0306, 0,
  0x51, 0x0435, 0x0308, 0,
  0x53, 0x0433, 0x0301, 0,
  0x57, 0x0456, 0x0308, 0,
  0x5c, 0x043A, 0x0301, 0,
  0x5e, 0x0443, 0x0306, 0,
  0x76, 0x0474, 0x030F, 0,
  0x77, 0x0475, 0x030F, 0,
  0xc1, 0x0416, 0x0306, 0,
  0xc2, 0x0436, 0x0306, 0,
  0xd0, 0x0410, 0x0306, 0,
  0xd1, 0x0430, 0x0306, 0,
  0xd2, 0x0410, 0x0308, 0,
  0xd3, 0x0430, 0x0308, 0,
  0xd4, 0x00C6, 0,
  0xd5, 0x00E6, 0,
  0xd6, 0x0415, 0x0306, 0,
  0xd7, 0x0435, 0x0306, 0,
  0xd8, 0x018F, 0,
  0xd9, 0x0259, 0,
  0xda, 0x018F, 0x0308, 0,
  0xdb, 0x0259, 0x0308, 0,
  0xdc, 0x0416, 0x0308, 0,
  0xdd, 0x0436, 0x0308, 0,
  0xde, 0x0417, 0x0308, 0,
  0xdf, 0x0437, 0x0308, 0,
  0xe0, 0x01B7, 0,
  0xe1, 0x0292, 0,
  0xe2, 0x0418, 0x0304, 0,
  0xe3, 0x0438, 0x0304, 0,
  0xe4, 0x0418, 0x0308, 0,
  0xe5, 0x0438, 0x0308, 0,
  0xe6, 0x041E, 0x0308, 0,
  0xe7, 0x043E, 0x0308, 0,
  0xe8, 0x019F, 0,
  0xe9, 0x0275, 0,
  0xea, 0x019F, 0x0308, 0,
  0xeb, 0x0275, 0x0308, 0,
  0xee, 0x0423, 0x0304, 0,
  0xef, 0x0443, 0x0304, 0,
  0xf0, 0x0423, 0x0308, 0,
  0xf1, 0x0443, 0x0308, 0,
  0xf2, 0x0423, 0x030B, 0,
  0xf3, 0x0443, 0x030B, 0,
  0xf4, 0x0427, 0x0308, 0,
  0xf5, 0x0447, 0x0308, 0,
  0xf8, 0x042B, 0x0308, 0,
  0xf9, 0x044B, 0x0308, 0,
  0x00,

  /* page09 */
  0x09,
  0x29, 0x0928, 0x093C, 0,
  0x31, 0x0930, 0x093C, 0,
  0x34, 0x0933, 0x093C, 0,
  0x58, 0x0915, 0x093C, 0,
  0x59, 0x0916, 0x093C, 0,
  0x5a, 0x0917, 0x093C, 0,
  0x5b, 0x091C, 0x093C, 0,
  0x5c, 0x0921, 0x093C, 0,
  0x5d, 0x0922, 0x093C, 0,
  0x5e, 0x092B, 0x093C, 0,
  0x5f, 0x092F, 0x093C, 0,
  0xb0, 0x09AC, 0x09BC, 0,
  0xcb, 0x09C7, 0x09BE, 0,
  0xcc, 0x09C7, 0x09D7, 0,
  0xdc, 0x09A1, 0x09BC, 0,
  0xdd, 0x09A2, 0x09BC, 0,
  0xdf, 0x09AF, 0x09BC, 0,
  0x00,

  /* page0a */
  0x0a,
  0x59, 0x0A16, 0x0A3C, 0,
  0x5a, 0x0A17, 0x0A3C, 0,
  0x5b, 0x0A1C, 0x0A3C, 0,
  0x5c, 0x0A21, 0x0A3C, 0,
  0x5e, 0x0A2B, 0x0A3C, 0,
  0x00,

  /* page0b */
  0x0b,
  0x48, 0x0B47, 0x0B56, 0,
  0x4b, 0x0B47, 0x0B3E, 0,
  0x4c, 0x0B47, 0x0B57, 0,
  0x5c, 0x0B21, 0x0B3C, 0,
  0x5d, 0x0B22, 0x0B3C, 0,
  0x5f, 0x0B2F, 0x0B3C, 0,
  0x94, 0x0B92, 0x0BD7, 0,
  0xca, 0x0BC6, 0x0BBE, 0,
  0xcb, 0x0BC7, 0x0BBE, 0,
  0xcc, 0x0BC6, 0x0BD7, 0,
  0x00,

  /* page0c */
  0x0c,
  0x48, 0x0C46, 0x0C56, 0,
  0xc0, 0x0CBF, 0x0CD5, 0,
  0xc7, 0x0CC6, 0x0CD5, 0,
  0xc8, 0x0CC6, 0x0CD6, 0,
  0xca, 0x0CC6, 0x0CC2, 0,
  0xcb, 0x0CC6, 0x0CC2, 0x0CD5, 0,
  0x00,

  /* page0d */
  0x0d,
  0x4a, 0x0D46, 0x0D3E, 0,
  0x4b, 0x0D47, 0x0D3E, 0,
  0x4c, 0x0D46, 0x0D57, 0,
  0x00,

  /* page0e */
  0x0e,
  0x33, 0x0E4D, 0x0E32, 0,
  0xb3, 0x0ECD, 0x0EB2, 0,
  0x00,

  /* page0f */
  0x0f,
  0x43, 0x0F42, 0x0FB7, 0,
  0x4d, 0x0F4C, 0x0FB7, 0,
  0x52, 0x0F51, 0x0FB7, 0,
  0x57, 0x0F56, 0x0FB7, 0,
  0x5c, 0x0F5B, 0x0FB7, 0,
  0x69, 0x0F40, 0x0FB5, 0,
  0x73, 0x0F72, 0x0F71, 0,
  0x75, 0x0F74, 0x0F71, 0,
  0x76, 0x0FB2, 0x0F80, 0,
  0x77, 0x0FB2, 0x0F80, 0x0F71, 0,
  0x78, 0x0FB3, 0x0F80, 0,
  0x79, 0x0FB3, 0x0F80, 0x0F71, 0,
  0x81, 0x0F80, 0x0F71, 0,
  0x93, 0x0F92, 0x0FB7, 0,
  0x9d, 0x0F9C, 0x0FB7, 0,
  0xa2, 0x0FA1, 0x0FB7, 0,
  0xa7, 0x0FA6, 0x0FB7, 0,
  0xac, 0x0FAB, 0x0FB7, 0,
  0xb9, 0x0F90, 0x0FB5, 0,
  0x00,

  /* page1e */
  0x1e,
  0x00, 0x0041, 0x0325, 0,
  0x01, 0x0061, 0x0325, 0,
  0x02, 0x0042, 0x0307, 0,
  0x03, 0x0062, 0x0307, 0,
  0x04, 0x0042, 0x0323, 0,
  0x05, 0x0062, 0x0323, 0,
  0x06, 0x0042, 0x0331, 0,
  0x07, 0x0062, 0x0331, 0,
  0x08, 0x0043, 0x0327, 0x0301, 0,
  0x09, 0x0063, 0x0327, 0x0301, 0,
  0x0a, 0x0044, 0x0307, 0,
  0x0b, 0x0064, 0x0307, 0,
  0x0c, 0x0044, 0x0323, 0,
  0x0d, 0x0064, 0x0323, 0,
  0x0e, 0x0044, 0x0331, 0,
  0x0f, 0x0064, 0x0331, 0,
  0x10, 0x0044, 0x0327, 0,
  0x11, 0x0064, 0x0327, 0,
  0x12, 0x0044, 0x032D, 0,
  0x13, 0x0064, 0x032D, 0,
  0x14, 0x0045, 0x0304, 0x0300, 0,
  0x15, 0x0065, 0x0304, 0x0300, 0,
  0x16, 0x0045, 0x0304, 0x0301, 0,
  0x17, 0x0065, 0x0304, 0x0301, 0,
  0x18, 0x0045, 0x032D, 0,
  0x19, 0x0065, 0x032D, 0,
  0x1a, 0x0045, 0x0330, 0,
  0x1b, 0x0065, 0x0330, 0,
  0x1c, 0x0045, 0x0327, 0x0306, 0,
  0x1d, 0x0065, 0x0327, 0x0306, 0,
  0x1e, 0x0046, 0x0307, 0,
  0x1f, 0x0066, 0x0307, 0,
  0x20, 0x0047, 0x0304, 0,
  0x21, 0x0067, 0x0304, 0,
  0x22, 0x0048, 0x0307, 0,
  0x23, 0x0068, 0x0307, 0,
  0x24, 0x0048, 0x0323, 0,
  0x25, 0x0068, 0x0323, 0,
  0x26, 0x0048, 0x0308, 0,
  0x27, 0x0068, 0x0308, 0,
  0x28, 0x0048, 0x0327, 0,
  0x29, 0x0068, 0x0327, 0,
  0x2a, 0x0048, 0x032E, 0,
  0x2b, 0x0068, 0x032E, 0,
  0x2c, 0x0049, 0x0330, 0,
  0x2d, 0x0069, 0x0330, 0,
  0x2e, 0x0049, 0x0308, 0x0301, 0,
  0x2f, 0x0069, 0x0308, 0x0301, 0,
  0x30, 0x004B, 0x0301, 0,
  0x31, 0x006B, 0x0301, 0,
  0x32, 0x004B, 0x0323, 0,
  0x33, 0x006B, 0x0323, 0,
  0x34, 0x004B, 0x0331, 0,
  0x35, 0x006B, 0x0331, 0,
  0x36, 0x004C, 0x0323, 0,
  0x37, 0x006C, 0x0323, 0,
  0x38, 0x004C, 0x0323, 0x0304, 0,
  0x39, 0x006C, 0x0323, 0x0304, 0,
  0x3a, 0x004C, 0x0331, 0,
  0x3b, 0x006C, 0x0331, 0,
  0x3c, 0x004C, 0x032D, 0,
  0x3d, 0x006C, 0x032D, 0,
  0x3e, 0x004D, 0x0301, 0,
  0x3f, 0x006D, 0x0301, 0,
  0x40, 0x004D, 0x0307, 0,
  0x41, 0x006D, 0x0307, 0,
  0x42, 0x004D, 0x0323, 0,
  0x43, 0x006D, 0x0323, 0,
  0x44, 0x004E, 0x0307, 0,
  0x45, 0x006E, 0x0307, 0,
  0x46, 0x004E, 0x0323, 0,
  0x47, 0x006E, 0x0323, 0,
  0x48, 0x004E, 0x0331, 0,
  0x49, 0x006E, 0x0331, 0,
  0x4a, 0x004E, 0x032D, 0,
  0x4b, 0x006E, 0x032D, 0,
  0x4c, 0x004F, 0x0303, 0x0301, 0,
  0x4d, 0x006F, 0x0303, 0x0301, 0,
  0x4e, 0x004F, 0x0303, 0x0308, 0,
  0x4f, 0x006F, 0x0303, 0x0308, 0,
  0x50, 0x004F, 0x0304, 0x0300, 0,
  0x51, 0x006F, 0x0304, 0x0300, 0,
  0x52, 0x004F, 0x0304, 0x0301, 0,
  0x53, 0x006F, 0x0304, 0x0301, 0,
  0x54, 0x0050, 0x0301, 0,
  0x55, 0x0070, 0x0301, 0,
  0x56, 0x0050, 0x0307, 0,
  0x57, 0x0070, 0x0307, 0,
  0x58, 0x0052, 0x0307, 0,
  0x59, 0x0072, 0x0307, 0,
  0x5a, 0x0052, 0x0323, 0,
  0x5b, 0x0072, 0x0323, 0,
  0x5c, 0x0052, 0x0323, 0x0304, 0,
  0x5d, 0x0072, 0x0323, 0x0304, 0,
  0x5e, 0x0052, 0x0331, 0,
  0x5f, 0x0072, 0x0331, 0,
  0x60, 0x0053, 0x0307, 0,
  0x61, 0x0073, 0x0307, 0,
  0x62, 0x0053, 0x0323, 0,
  0x63, 0x0073, 0x0323, 0,
  0x64, 0x0053, 0x0301, 0x0307, 0,
  0x65, 0x0073, 0x0301, 0x0307, 0,
  0x66, 0x0053, 0x030C, 0x0307, 0,
  0x67, 0x0073, 0x030C, 0x0307, 0,
  0x68, 0x0053, 0x0323, 0x0307, 0,
  0x69, 0x0073, 0x0323, 0x0307, 0,
  0x6a, 0x0054, 0x0307, 0,
  0x6b, 0x0074, 0x0307, 0,
  0x6c, 0x0054, 0x0323, 0,
  0x6d, 0x0074, 0x0323, 0,
  0x6e, 0x0054, 0x0331, 0,
  0x6f, 0x0074, 0x0331, 0,
  0x70, 0x0054, 0x032D, 0,
  0x71, 0x0074, 0x032D, 0,
  0x72, 0x0055, 0x0324, 0,
  0x73, 0x0075, 0x0324, 0,
  0x74, 0x0055, 0x0330, 0,
  0x75, 0x0075, 0x0330, 0,
  0x76, 0x0055, 0x032D, 0,
  0x77, 0x0075, 0x032D, 0,
  0x78, 0x0055, 0x0303, 0x0301, 0,
  0x79, 0x0075, 0x0303, 0x0301, 0,
  0x7a, 0x0055, 0x0304, 0x0308, 0,
  0x7b, 0x0075, 0x0304, 0x0308, 0,
  0x7c, 0x0056, 0x0303, 0,
  0x7d, 0x0076, 0x0303, 0,
  0x7e, 0x0056, 0x0323, 0,
  0x7f, 0x0076, 0x0323, 0,
  0x80, 0x0057, 0x0300, 0,
  0x81, 0x0077, 0x0300, 0,
  0x82, 0x0057, 0x0301, 0,
  0x83, 0x0077, 0x0301, 0,
  0x84, 0x0057, 0x0308, 0,
  0x85, 0x0077, 0x0308, 0,
  0x86, 0x0057, 0x0307, 0,
  0x87, 0x0077, 0x0307, 0,
  0x88, 0x0057, 0x0323, 0,
  0x89, 0x0077, 0x0323, 0,
  0x8a, 0x0058, 0x0307, 0,
  0x8b, 0x0078, 0x0307, 0,
  0x8c, 0x0058, 0x0308, 0,
  0x8d, 0x0078, 0x0308, 0,
  0x8e, 0x0059, 0x0307, 0,
  0x8f, 0x0079, 0x0307, 0,
  0x90, 0x005A, 0x0302, 0,
  0x91, 0x007A, 0x0302, 0,
  0x92, 0x005A, 0x0323, 0,
  0x93, 0x007A, 0x0323, 0,
  0x94, 0x005A, 0x0331, 0,
  0x95, 0x007A, 0x0331, 0,
  0x96, 0x0068, 0x0331, 0,
  0x97, 0x0074, 0x0308, 0,
  0x98, 0x0077, 0x030A, 0,
  0x99, 0x0079, 0x030A, 0,
  0x9b, 0x017F, 0x0307, 0,
  0xa0, 0x0041, 0x0323, 0,
  0xa1, 0x0061, 0x0323, 0,
  0xa2, 0x0041, 0x0309, 0,
  0xa3, 0x0061, 0x0309, 0,
  0xa4, 0x0041, 0x0302, 0x0301, 0,
  0xa5, 0x0061, 0x0302, 0x0301, 0,
  0xa6, 0x0041, 0x0302, 0x0300, 0,
  0xa7, 0x0061, 0x0302, 0x0300, 0,
  0xa8, 0x0041, 0x0302, 0x0309, 0,
  0xa9, 0x0061, 0x0302, 0x0309, 0,
  0xaa, 0x0041, 0x0302, 0x0303, 0,
  0xab, 0x0061, 0x0302, 0x0303, 0,
  0xac, 0x0041, 0x0323, 0x0302, 0,
  0xad, 0x0061, 0x0323, 0x0302, 0,
  0xae, 0x0041, 0x0306, 0x0301, 0,
  0xaf, 0x0061, 0x0306, 0x0301, 0,
  0xb0, 0x0041, 0x0306, 0x0300, 0,
  0xb1, 0x0061, 0x0306, 0x0300, 0,
  0xb2, 0x0041, 0x0306, 0x0309, 0,
  0xb3, 0x0061, 0x0306, 0x0309, 0,
  0xb4, 0x0041, 0x0306, 0x0303, 0,
  0xb5, 0x0061, 0x0306, 0x0303, 0,
  0xb6, 0x0041, 0x0323, 0x0306, 0,
  0xb7, 0x0061, 0x0323, 0x0306, 0,
  0xb8, 0x0045, 0x0323, 0,
  0xb9, 0x0065, 0x0323, 0,
  0xba, 0x0045, 0x0309, 0,
  0xbb, 0x0065, 0x0309, 0,
  0xbc, 0x0045, 0x0303, 0,
  0xbd, 0x0065, 0x0303, 0,
  0xbe, 0x0045, 0x0302, 0x0301, 0,
  0xbf, 0x0065, 0x0302, 0x0301, 0,
  0xc0, 0x0045, 0x0302, 0x0300, 0,
  0xc1, 0x0065, 0x0302, 0x0300, 0,
  0xc2, 0x0045, 0x0302, 0x0309, 0,
  0xc3, 0x0065, 0x0302, 0x0309, 0,
  0xc4, 0x0045, 0x0302, 0x0303, 0,
  0xc5, 0x0065, 0x0302, 0x0303, 0,
  0xc6, 0x0045, 0x0323, 0x0302, 0,
  0xc7, 0x0065, 0x0323, 0x0302, 0,
  0xc8, 0x0049, 0x0309, 0,
  0xc9, 0x0069, 0x0309, 0,
  0xca, 0x0049, 0x0323, 0,
  0xcb, 0x0069, 0x0323, 0,
  0xcc, 0x004F, 0x0323, 0,
  0xcd, 0x006F, 0x0323, 0,
  0xce, 0x004F, 0x0309, 0,
  0xcf, 0x006F, 0x0309, 0,
  0xd0, 0x004F, 0x0302, 0x0301, 0,
  0xd1, 0x006F, 0x0302, 0x0301, 0,
  0xd2, 0x004F, 0x0302, 0x0300, 0,
  0xd3, 0x006F, 0x0302, 0x0300, 0,
  0xd4, 0x004F, 0x0302, 0x0309, 0,
  0xd5, 0x006F, 0x0302, 0x0309, 0,
  0xd6, 0x004F, 0x0302, 0x0303, 0,
  0xd7, 0x006F, 0x0302, 0x0303, 0,
  0xd8, 0x004F, 0x0323, 0x0302, 0,
  0xd9, 0x006F, 0x0323, 0x0302, 0,
  0xda, 0x004F, 0x031B, 0x0301, 0,
  0xdb, 0x006F, 0x031B, 0x0301, 0,
  0xdc, 0x004F, 0x031B, 0x0300, 0,
  0xdd, 0x006F, 0x031B, 0x0300, 0,
  0xde, 0x004F, 0x031B, 0x0309, 0,
  0xdf, 0x006F, 0x031B, 0x0309, 0,
  0xe0, 0x004F, 0x031B, 0x0303, 0,
  0xe1, 0x006F, 0x031B, 0x0303, 0,
  0xe2, 0x004F, 0x031B, 0x0323, 0,
  0xe3, 0x006F, 0x031B, 0x0323, 0,
  0xe4, 0x0055, 0x0323, 0,
  0xe5, 0x0075, 0x0323, 0,
  0xe6, 0x0055, 0x0309, 0,
  0xe7, 0x0075, 0x0309, 0,
  0xe8, 0x0055, 0x031B, 0x0301, 0,
  0xe9, 0x0075, 0x031B, 0x0301, 0,
  0xea, 0x0055, 0x031B, 0x0300, 0,
  0xeb, 0x0075, 0x031B, 0x0300, 0,
  0xec, 0x0055, 0x031B, 0x0309, 0,
  0xed, 0x0075, 0x031B, 0x0309, 0,
  0xee, 0x0055, 0x031B, 0x0303, 0,
  0xef, 0x0075, 0x031B, 0x0303, 0,
  0xf0, 0x0055, 0x031B, 0x0323, 0,
  0xf1, 0x0075, 0x031B, 0x0323, 0,
  0xf2, 0x0059, 0x0300, 0,
  0xf3, 0x0079, 0x0300, 0,
  0xf4, 0x0059, 0x0323, 0,
  0xf5, 0x0079, 0x0323, 0,
  0xf6, 0x0059, 0x0309, 0,
  0xf7, 0x0079, 0x0309, 0,
  0xf8, 0x0059, 0x0303, 0,
  0xf9, 0x0079, 0x0303, 0,
  0x00,

  /* page1f */
  0x1f,
  0x00, 0x03B1, 0x0313, 0,
  0x01, 0x03B1, 0x0314, 0,
  0x02, 0x03B1, 0x0313, 0x0300, 0,
  0x03, 0x03B1, 0x0314, 0x0300, 0,
  0x04, 0x03B1, 0x0313, 0x0301, 0,
  0x05, 0x03B1, 0x0314, 0x0301, 0,
  0x06, 0x03B1, 0x0313, 0x0342, 0,
  0x07, 0x03B1, 0x0314, 0x0342, 0,
  0x08, 0x0391, 0x0313, 0,
  0x09, 0x0391, 0x0314, 0,
  0x0a, 0x0391, 0x0313, 0x0300, 0,
  0x0b, 0x0391, 0x0314, 0x0300, 0,
  0x0c, 0x0391, 0x0313, 0x0301, 0,
  0x0d, 0x0391, 0x0314, 0x0301, 0,
  0x0e, 0x0391, 0x0313, 0x0342, 0,
  0x0f, 0x0391, 0x0314, 0x0342, 0,
  0x10, 0x03B5, 0x0313, 0,
  0x11, 0x03B5, 0x0314, 0,
  0x12, 0x03B5, 0x0313, 0x0300, 0,
  0x13, 0x03B5, 0x0314, 0x0300, 0,
  0x14, 0x03B5, 0x0313, 0x0301, 0,
  0x15, 0x03B5, 0x0314, 0x0301, 0,
  0x18, 0x0395, 0x0313, 0,
  0x19, 0x0395, 0x0314, 0,
  0x1a, 0x0395, 0x0313, 0x0300, 0,
  0x1b, 0x0395, 0x0314, 0x0300, 0,
  0x1c, 0x0395, 0x0313, 0x0301, 0,
  0x1d, 0x0395, 0x0314, 0x0301, 0,
  0x20, 0x03B7, 0x0313, 0,
  0x21, 0x03B7, 0x0314, 0,
  0x22, 0x03B7, 0x0313, 0x0300, 0,
  0x23, 0x03B7, 0x0314, 0x0300, 0,
  0x24, 0x03B7, 0x0313, 0x0301, 0,
  0x25, 0x03B7, 0x0314, 0x0301, 0,
  0x26, 0x03B7, 0x0313, 0x0342, 0,
  0x27, 0x03B7, 0x0314, 0x0342, 0,
  0x28, 0x0397, 0x0313, 0,
  0x29, 0x0397, 0x0314, 0,
  0x2a, 0x0397, 0x0313, 0x0300, 0,
  0x2b, 0x0397, 0x0314, 0x0300, 0,
  0x2c, 0x0397, 0x0313, 0x0301, 0,
  0x2d, 0x0397, 0x0314, 0x0301, 0,
  0x2e, 0x0397, 0x0313, 0x0342, 0,
  0x2f, 0x0397, 0x0314, 0x0342, 0,
  0x30, 0x03B9, 0x0313, 0,
  0x31, 0x03B9, 0x0314, 0,
  0x32, 0x03B9, 0x0313, 0x0300, 0,
  0x33, 0x03B9, 0x0314, 0x0300, 0,
  0x34, 0x03B9, 0x0313, 0x0301, 0,
  0x35, 0x03B9, 0x0314, 0x0301, 0,
  0x36, 0x03B9, 0x0313, 0x0342, 0,
  0x37, 0x03B9, 0x0314, 0x0342, 0,
  0x38, 0x0399, 0x0313, 0,
  0x39, 0x0399, 0x0314, 0,
  0x3a, 0x0399, 0x0313, 0x0300, 0,
  0x3b, 0x0399, 0x0314, 0x0300, 0,
  0x3c, 0x0399, 0x0313, 0x0301, 0,
  0x3d, 0x0399, 0x0314, 0x0301, 0,
  0x3e, 0x0399, 0x0313, 0x0342, 0,
  0x3f, 0x0399, 0x0314, 0x0342, 0,
  0x40, 0x03BF, 0x0313, 0,
  0x41, 0x03BF, 0x0314, 0,
  0x42, 0x03BF, 0x0313, 0x0300, 0,
  0x43, 0x03BF, 0x0314, 0x0300, 0,
  0x44, 0x03BF, 0x0313, 0x0301, 0,
  0x45, 0x03BF, 0x0314, 0x0301, 0,
  0x48, 0x039F, 0x0313, 0,
  0x49, 0x039F, 0x0314, 0,
  0x4a, 0x039F, 0x0313, 0x0300, 0,
  0x4b, 0x039F, 0x0314, 0x0300, 0,
  0x4c, 0x039F, 0x0313, 0x0301, 0,
  0x4d, 0x039F, 0x0314, 0x0301, 0,
  0x50, 0x03C5, 0x0313, 0,
  0x51, 0x03C5, 0x0314, 0,
  0x52, 0x03C5, 0x0313, 0x0300, 0,
  0x53, 0x03C5, 0x0314, 0x0300, 0,
  0x54, 0x03C5, 0x0313, 0x0301, 0,
  0x55, 0x03C5, 0x0314, 0x0301, 0,
  0x56, 0x03C5, 0x0313, 0x0342, 0,
  0x57, 0x03C5, 0x0314, 0x0342, 0,
  0x59, 0x03A5, 0x0314, 0,
  0x5b, 0x03A5, 0x0314, 0x0300, 0,
  0x5d, 0x03A5, 0x0314, 0x0301, 0,
  0x5f, 0x03A5, 0x0314, 0x0342, 0,
  0x60, 0x03C9, 0x0313, 0,
  0x61, 0x03C9, 0x0314, 0,
  0x62, 0x03C9, 0x0313, 0x0300, 0,
  0x63, 0x03C9, 0x0314, 0x0300, 0,
  0x64, 0x03C9, 0x0313, 0x0301, 0,
  0x65, 0x03C9, 0x0314, 0x0301, 0,
  0x66, 0x03C9, 0x0313, 0x0342, 0,
  0x67, 0x03C9, 0x0314, 0x0342, 0,
  0x68, 0x03A9, 0x0313, 0,
  0x69, 0x03A9, 0x0314, 0,
  0x6a, 0x03A9, 0x0313, 0x0300, 0,
  0x6b, 0x03A9, 0x0314, 0x0300, 0,
  0x6c, 0x03A9, 0x0313, 0x0301, 0,
  0x6d, 0x03A9, 0x0314, 0x0301, 0,
  0x6e, 0x03A9, 0x0313, 0x0342, 0,
  0x6f, 0x03A9, 0x0314, 0x0342, 0,
  0x70, 0x03B1, 0x0300, 0,
  0x71, 0x03B1, 0x0301, 0,
  0x72, 0x03B5, 0x0300, 0,
  0x73, 0x03B5, 0x0301, 0,
  0x74, 0x03B7, 0x0300, 0,
  0x75, 0x03B7, 0x0301, 0,
  0x76, 0x03B9, 0x0300, 0,
  0x77, 0x03B9, 0x0301, 0,
  0x78, 0x03BF, 0x0300, 0,
  0x79, 0x03BF, 0x0301, 0,
  0x7a, 0x03C5, 0x0300, 0,
  0x7b, 0x03C5, 0x0301, 0,
  0x7c, 0x03C9, 0x0300, 0,
  0x7d, 0x03C9, 0x0301, 0,
  0x80, 0x03B1, 0x0345, 0x0313, 0,
  0x81, 0x03B1, 0x0345, 0x0314, 0,
  0x82, 0x03B1, 0x0345, 0x0313, 0x0300, 0,
  0x83, 0x03B1, 0x0345, 0x0314, 0x0300, 0,
  0x84, 0x03B1, 0x0345, 0x0313, 0x0301, 0,
  0x85, 0x03B1, 0x0345, 0x0314, 0x0301, 0,
  0x86, 0x03B1, 0x0345, 0x0313, 0x0342, 0,
  0x87, 0x03B1, 0x0345, 0x0314, 0x0342, 0,
  0x88, 0x0391, 0x0345, 0x0313, 0,
  0x89, 0x0391, 0x0345, 0x0314, 0,
  0x8a, 0x0391, 0x0345, 0x0313, 0x0300, 0,
  0x8b, 0x0391, 0x0345, 0x0314, 0x0300, 0,
  0x8c, 0x0391, 0x0345, 0x0313, 0x0301, 0,
  0x8d, 0x0391, 0x0345, 0x0314, 0x0301, 0,
  0x8e, 0x0391, 0x0345, 0x0313, 0x0342, 0,
  0x8f, 0x0391, 0x0345, 0x0314, 0x0342, 0,
  0x90, 0x03B7, 0x0345, 0x0313, 0,
  0x91, 0x03B7, 0x0345, 0x0314, 0,
  0x92, 0x03B7, 0x0345, 0x0313, 0x0300, 0,
  0x93, 0x03B7, 0x0345, 0x0314, 0x0300, 0,
  0x94, 0x03B7, 0x0345, 0x0313, 0x0301, 0,
  0x95, 0x03B7, 0x0345, 0x0314, 0x0301, 0,
  0x96, 0x03B7, 0x0345, 0x0313, 0x0342, 0,
  0x97, 0x03B7, 0x0345, 0x0314, 0x0342, 0,
  0x98, 0x0397, 0x0345, 0x0313, 0,
  0x99, 0x0397, 0x0345, 0x0314, 0,
  0x9a, 0x0397, 0x0345, 0x0313, 0x0300, 0,
  0x9b, 0x0397, 0x0345, 0x0314, 0x0300, 0,
  0x9c, 0x0397, 0x0345, 0x0313, 0x0301, 0,
  0x9d, 0x0397, 0x0345, 0x0314, 0x0301, 0,
  0x9e, 0x0397, 0x0345, 0x0313, 0x0342, 0,
  0x9f, 0x0397, 0x0345, 0x0314, 0x0342, 0,
  0xa0, 0x03C9, 0x0345, 0x0313, 0,
  0xa1, 0x03C9, 0x0345, 0x0314, 0,
  0xa2, 0x03C9, 0x0345, 0x0313, 0x0300, 0,
  0xa3, 0x03C9, 0x0345, 0x0314, 0x0300, 0,
  0xa4, 0x03C9, 0x0345, 0x0313, 0x0301, 0,
  0xa5, 0x03C9, 0x0345, 0x0314, 0x0301, 0,
  0xa6, 0x03C9, 0x0345, 0x0313, 0x0342, 0,
  0xa7, 0x03C9, 0x0345, 0x0314, 0x0342, 0,
  0xa8, 0x03A9, 0x0345, 0x0313, 0,
  0xa9, 0x03A9, 0x0345, 0x0314, 0,
  0xaa, 0x03A9, 0x0345, 0x0313, 0x0300, 0,
  0xab, 0x03A9, 0x0345, 0x0314, 0x0300, 0,
  0xac, 0x03A9, 0x0345, 0x0313, 0x0301, 0,
  0xad, 0x03A9, 0x0345, 0x0314, 0x0301, 0,
  0xae, 0x03A9, 0x0345, 0x0313, 0x0342, 0,
  0xaf, 0x03A9, 0x0345, 0x0314, 0x0342, 0,
  0xb0, 0x03B1, 0x0306, 0,
  0xb1, 0x03B1, 0x0304, 0,
  0xb2, 0x03B1, 0x0345, 0x0300, 0,
  0xb3, 0x03B1, 0x0345, 0,
  0xb4, 0x03B1, 0x0345, 0x0301, 0,
  0xb6, 0x03B1, 0x0342, 0,
  0xb7, 0x03B1, 0x0345, 0x0342, 0,
  0xb8, 0x0391, 0x0306, 0,
  0xb9, 0x0391, 0x0304, 0,
  0xba, 0x0391, 0x0300, 0,
  0xbb, 0x0391, 0x0301, 0,
  0xbc, 0x0391, 0x0345, 0,
  0xbe, 0x03B9, 0,
  0xc1, 0x00A8, 0x0342, 0,
  0xc2, 0x03B7, 0x0345, 0x0300, 0,
  0xc3, 0x03B7, 0x0345, 0,
  0xc4, 0x03B7, 0x0345, 0x0301, 0,
  0xc6, 0x03B7, 0x0342, 0,
  0xc7, 0x03B7, 0x0345, 0x0342, 0,
  0xc8, 0x0395, 0x0300, 0,
  0xc9, 0x0395, 0x0301, 0,
  0xca, 0x0397, 0x0300, 0,
  0xcb, 0x0397, 0x0301, 0,
  0xcc, 0x0397, 0x0345, 0,
  0xcd, 0x1FBF, 0x0300, 0,
  0xce, 0x1FBF, 0x0301, 0,
  0xcf, 0x1FBF, 0x0342, 0,
  0xd0, 0x03B9, 0x0306, 0,
  0xd1, 0x03B9, 0x0304, 0,
  0xd2, 0x03B9, 0x0308, 0x0300, 0,
  0xd3, 0x03B9, 0x0308, 0x0301, 0,
  0xd6, 0x03B9, 0x0342, 0,
  0xd7, 0x03B9, 0x0308, 0x0342, 0,
  0xd8, 0x0399, 0x0306, 0,
  0xd9, 0x0399, 0x0304, 0,
  0xda, 0x0399, 0x0300, 0,
  0xdb, 0x0399, 0x0301, 0,
  0xdd, 0x1FFE, 0x0300, 0,
  0xde, 0x1FFE, 0x0301, 0,
  0xdf, 0x1FFE, 0x0342, 0,
  0xe0, 0x03C5, 0x0306, 0,
  0xe1, 0x03C5, 0x0304, 0,
  0xe2, 0x03C5, 0x0308, 0x0300, 0,
  0xe3, 0x03C5, 0x0308, 0x0301, 0,
  0xe4, 0x03C1, 0x0313, 0,
  0xe5, 0x03C1, 0x0314, 0,
  0xe6, 0x03C5, 0x0342, 0,
  0xe7, 0x03C5, 0x0308, 0x0342, 0,
  0xe8, 0x03A5, 0x0306, 0,
  0xe9, 0x03A5, 0x0304, 0,
  0xea, 0x03A5, 0x0300, 0,
  0xeb, 0x03A5, 0x0301, 0,
  0xec, 0x03A1, 0x0314, 0,
  0xed, 0x00A8, 0x0300, 0,
  0xee, 0x00A8, 0x0301, 0,
  0xef, 0x0060, 0,
  0xf2, 0x03C9, 0x0345, 0x0300, 0,
  0xf3, 0x03C9, 0x0345, 0,
  0xf4, 0x03BF, 0x0345, 0x0301, 0,
  0xf6, 0x03C9, 0x0342, 0,
  0xf7, 0x03C9, 0x0345, 0x0342, 0,
  0xf8, 0x039F, 0x0300, 0,
  0xf9, 0x039F, 0x0301, 0,
  0xfa, 0x03A9, 0x0300, 0,
  0xfb, 0x03A9, 0x0301, 0,
  0xfc, 0x03A9, 0x0345, 0,
  0xfd, 0x00B4, 0,
  0x00,

  /* page30 */
  0x30,
  0x4c, 0x304B, 0x3099, 0,
  0x4e, 0x304D, 0x3099, 0,
  0x50, 0x304F, 0x3099, 0,
  0x52, 0x3051, 0x3099, 0,
  0x54, 0x3053, 0x3099, 0,
  0x56, 0x3055, 0x3099, 0,
  0x58, 0x3057, 0x3099, 0,
  0x5a, 0x3059, 0x3099, 0,
  0x5c, 0x305B, 0x3099, 0,
  0x5e, 0x305D, 0x3099, 0,
  0x60, 0x305F, 0x3099, 0,
  0x62, 0x3061, 0x3099, 0,
  0x65, 0x3064, 0x3099, 0,
  0x67, 0x3066, 0x3099, 0,
  0x69, 0x3068, 0x3099, 0,
  0x70, 0x306F, 0x3099, 0,
  0x71, 0x306F, 0x309A, 0,
  0x73, 0x3072, 0x3099, 0,
  0x74, 0x3072, 0x309A, 0,
  0x76, 0x3075, 0x3099, 0,
  0x77, 0x3075, 0x309A, 0,
  0x79, 0x3078, 0x3099, 0,
  0x7a, 0x3078, 0x309A, 0,
  0x7c, 0x307B, 0x3099, 0,
  0x7d, 0x307B, 0x309A, 0,
  0x94, 0x3046, 0x3099, 0,
  0x9e, 0x309D, 0x3099, 0,
  0xac, 0x30AB, 0x3099, 0,
  0xae, 0x30AD, 0x3099, 0,
  0xb0, 0x30AF, 0x3099, 0,
  0xb2, 0x30B1, 0x3099, 0,
  0xb4, 0x30B3, 0x3099, 0,
  0xb6, 0x30B5, 0x3099, 0,
  0xb8, 0x30B7, 0x3099, 0,
  0xba, 0x30B9, 0x3099, 0,
  0xbc, 0x30BB, 0x3099, 0,
  0xbe, 0x30BD, 0x3099, 0,
  0xc0, 0x30BF, 0x3099, 0,
  0xc2, 0x30C1, 0x3099, 0,
  0xc5, 0x30C4, 0x3099, 0,
  0xc7, 0x30C6, 0x3099, 0,
  0xc9, 0x30C8, 0x3099, 0,
  0xd0, 0x30CF, 0x3099, 0,
  0xd1, 0x30CF, 0x309A, 0,
  0xd3, 0x30D2, 0x3099, 0,
  0xd4, 0x30D2, 0x309A, 0,
  0xd6, 0x30D5, 0x3099, 0,
  0xd7, 0x30D5, 0x309A, 0,
  0xd9, 0x30D8, 0x3099, 0,
  0xda, 0x30D8, 0x309A, 0,
  0xdc, 0x30DB, 0x3099, 0,
  0xdd, 0x30DB, 0x309A, 0,
  0xf4, 0x30A6, 0x3099, 0,
  0xf7, 0x30EF, 0x3099, 0,
  0xf8, 0x30F0, 0x3099, 0,
  0xf9, 0x30F1, 0x3099, 0,
  0xfa, 0x30F2, 0x3099, 0,
  0xfe, 0x30FD, 0x3099, 0,
  0x00,

  /* pagefb */
  0xfb,
  0x1f, 0x05F2, 0x05B7, 0,
  0x2a, 0x05E9, 0x05C1, 0,
  0x2b, 0x05E9, 0x05C2, 0,
  0x2c, 0x05E9, 0x05BC, 0x05C1, 0,
  0x2d, 0x05E9, 0x05BC, 0x05C2, 0,
  0x2e, 0x05D0, 0x05B7, 0,
  0x2f, 0x05D0, 0x05B8, 0,
  0x30, 0x05D0, 0x05BC, 0,
  0x31, 0x05D1, 0x05BC, 0,
  0x32, 0x05D2, 0x05BC, 0,
  0x33, 0x05D3, 0x05BC, 0,
  0x34, 0x05D4, 0x05BC, 0,
  0x35, 0x05D5, 0x05BC, 0,
  0x36, 0x05D6, 0x05BC, 0,
  0x38, 0x05D8, 0x05BC, 0,
  0x39, 0x05D9, 0x05BC, 0,
  0x3a, 0x05DA, 0x05BC, 0,
  0x3b, 0x05DB, 0x05BC, 0,
  0x3c, 0x05DC, 0x05BC, 0,
  0x3e, 0x05DE, 0x05BC, 0,
  0x40, 0x05E0, 0x05BC, 0,
  0x41, 0x05E1, 0x05BC, 0,
  0x43, 0x05E3, 0x05BC, 0,
  0x44, 0x05E4, 0x05BC, 0,
  0x46, 0x05E6, 0x05BC, 0,
  0x47, 0x05E7, 0x05BC, 0,
  0x48, 0x05E8, 0x05BC, 0,
  0x49, 0x05E9, 0x05BC, 0,
  0x4a, 0x05EA, 0x05BC, 0,
  0x4b, 0x05D5, 0x05B9, 0,
  0x4c, 0x05D1, 0x05BF, 0,
  0x4d, 0x05DB, 0x05BF, 0,
  0x4e, 0x05E4, 0x05BF, 0,
  0x00,

  /* end of list */
  0x00
};

#define HFSPLUS_DECOMPOSE_PAGE_SIZE (256 * (HFSPLUS_MAX_DECOMPOSE_LEN + 1))

static uint16_t decompose_pages[16 * 256][HFSPLUS_MAX_DECOMPOSE_LEN + 1];

uint16_t (*hfsplus_decompose_pages[256])[HFSPLUS_MAX_DECOMPOSE_LEN + 1];


void make_hfsplus_decompose_pages()
{
    int page_idx = -1, char_idx, i;
    uint16_t *rpt, *page_pt, *value_pt;
    int page_count = 0;
   
    memset(decompose_pages, 0, 16 * HFSPLUS_DECOMPOSE_PAGE_SIZE);
    for (i = 0; i < 256; i++)
        hfsplus_decompose_pages[i] = NULL; 
   
    rpt = (uint16_t *) decompose_page_data;
    page_pt = (uint16_t *) decompose_pages;
    while (1) {
        if (*rpt <= page_idx)
    break;
        page_count++;
        page_idx = *(rpt++);
        char_idx = -1;
        while (1) {
            if(*rpt <= char_idx)
        break;
            char_idx = *(rpt++);
            value_pt = page_pt + char_idx * (HFSPLUS_MAX_DECOMPOSE_LEN + 1);
            while (1) {
                if(*rpt == 0)
            break;
                *(value_pt++) = *(rpt++);
            }
            rpt++;
        }
        rpt++;
        hfsplus_decompose_pages[page_idx] =
                                      decompose_pages + (page_count - 1) * 256;
        page_pt += HFSPLUS_DECOMPOSE_PAGE_SIZE;
    }
}
