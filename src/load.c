/**
 * Load Post Data
 *
 * Copyright (C) 2002-2016 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
 * This file is distributed as part of Siege
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --
 *
 */ 
#include <stdio.h> 
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <load.h>
#include <notify.h>
#include <memory.h>
#include <perl.h>
#include <errno.h>
#include <util.h>
#include <http.h>
#include <joedog/boolean.h>

struct ContentType {
  char    *ext;
  BOOLEAN ascii;
  char    *type;
};

static const struct ContentType tmap[] = {
  {"default", TRUE,  "application/x-www-form-urlencoded"},
  {"ai",      FALSE, "application/postscript"},
  {"aif",     FALSE, "audio/x-aiff"},
  {"aifc",    FALSE, "audio/x-aiff"},
  {"aiff",    FALSE, "audio/x-aiff"},
  {"asc",     TRUE,  "text/plain"},
  {"au",      FALSE, "audio/basic"},
  {"avi",     FALSE, "video/x-msvideo"},
  {"bcpio",   FALSE, "application/x-bcpio"},
  {"bin",     FALSE, "application/octet-stream"},
  {"c",       TRUE,  "text/plain"},
  {"cc",      TRUE,  "text/plain"},
  {"ccad",    FALSE, "application/clariscad"},
  {"cdf",     FALSE, "application/x-netcdf"},
  {"class",   FALSE, "application/octet-stream"},
  {"cpio",    FALSE, "application/x-cpio"},
  {"cpt",     FALSE, "application/mac-compactpro"},
  {"csh",     FALSE, "application/x-csh"},
  {"css",     TRUE,  "text/css"},
  {"csv",     TRUE,  "text/csv"},
  {"dcr",     FALSE, "application/x-director"},
  {"dir",     FALSE, "application/x-director"},
  {"dms",     FALSE, "application/octet-stream"},
  {"doc",     FALSE, "application/msword"},
  {"drw",     FALSE, "application/drafting"},
  {"dvi",     FALSE, "application/x-dvi"},
  {"dwg",     FALSE, "application/acad"},
  {"dxf",     FALSE, "application/dxf"},
  {"dxr",     FALSE, "application/x-director"},
  {"eps",     FALSE, "application/postscript"},
  {"etx",     TRUE,  "text/x-setext"},
  {"exe",     FALSE, "application/octet-stream"},
  {"ez",      FALSE, "application/andrew-inset"},
  {"f",       TRUE,  "text/plain"},
  {"f90",     TRUE,  "text/plain"},
  {"fli",     FALSE, "video/x-fli"},
  {"gif",     FALSE, "image/gif"},
  {"gtar",    FALSE, "application/x-gtar"},
  {"gz",      FALSE, "application/x-gzip"},
  {"h",       TRUE,  "text/plain"},
  {"hdf",     FALSE, "application/x-hdf"},
  {"hh",      TRUE,  "text/plain"},
  {"hqx",     FALSE, "application/mac-binhex40"},
  {"htm",     TRUE,  "text/html"},
  {"html",    TRUE,  "text/html"},
  {"ice",     FALSE, "x-conference/x-cooltalk"},
  {"ico",     FALSE, "image/x-icon"},
  {"ief",     FALSE, "image/ief"},
  {"iges",    FALSE, "model/iges"},
  {"igs",     FALSE, "model/iges"},
  {"ips",     FALSE, "application/x-ipscript"},
  {"ipx",     FALSE, "application/x-ipix"},
  {"jpe",     FALSE, "image/jpeg"},
  {"jpeg",    FALSE, "image/jpeg"},
  {"jpg",     FALSE, "image/jpeg"},
  {"js",      FALSE, "application/x-javascript"},
  {"json",    FALSE, "application/json"},
  {"kar",     FALSE, "audio/midi"},
  {"latex",   FALSE, "application/x-latex"},
  {"lha",     FALSE, "application/octet-stream"},
  {"lsp",     FALSE, "application/x-lisp"},
  {"lzh",     FALSE, "application/octet-stream"},
  {"m",       TRUE,  "text/plain"},
  {"man",     FALSE, "application/x-troff-man"},
  {"md",      TRUE,  "text/x-markdown"},
  {"me",      FALSE, "application/x-troff-me"},
  {"mesh",    FALSE, "model/mesh"},
  {"mid",     FALSE, "audio/midi"},
  {"midi",    FALSE, "audio/midi"},
  {"mif",     FALSE, "application/vnd.mif"},
  {"mime",    FALSE, "www/mime"},
  {"mov",     FALSE, "video/quicktime"},
  {"movie",   FALSE, "video/x-sgi-movie"},
  {"mp2",     FALSE, "audio/mpeg"},
  {"mp3",     FALSE, "audio/mpeg"},
  {"mpe",     FALSE, "video/mpeg"},
  {"mpeg",    FALSE, "video/mpeg"},
  {"mpg",     FALSE, "video/mpeg"},
  {"mpga",    FALSE, "audio/mpeg"},
  {"ms",      FALSE, "application/x-troff-ms"},
  {"msh",     FALSE, "model/mesh"},
  {"nc",      FALSE, "application/x-netcdf"},
  {"oda",     FALSE, "application/oda"},
  {"pbm",     FALSE, "image/x-portable-bitmap"},
  {"pdb",     FALSE, "chemical/x-pdb"},
  {"pdf",     FALSE, "application/pdf"},
  {"pgm",     FALSE, "image/x-portable-graymap"},
  {"pgn",     FALSE, "application/x-chess-pgn"},
  {"png",     FALSE, "image/png"},
  {"pnm",     FALSE, "image/x-portable-anymap"},
  {"pot",     FALSE, "application/mspowerpoint"},
  {"ppm",     FALSE, "image/x-portable-pixmap"},
  {"pps",     FALSE, "application/mspowerpoint"},
  {"ppt",     FALSE, "application/mspowerpoint"},
  {"ppz",     FALSE, "application/mspowerpoint"},
  {"pre",     FALSE, "application/x-freelance"},
  {"prt",     FALSE, "application/pro_eng"},
  {"ps",      FALSE, "application/postscript"},
  {"qt",      FALSE, "video/quicktime"},
  {"ra",      FALSE, "audio/x-realaudio"},
  {"ram",     FALSE, "audio/x-pn-realaudio"},
  {"ras",     FALSE, "image/cmu-raster"},
  {"rgb",     FALSE, "image/x-rgb"},
  {"rm",      FALSE, "audio/x-pn-realaudio"},
  {"roff",    FALSE, "application/x-troff"},
  {"rpm",     FALSE, "audio/x-pn-realaudio-plugin"},
  {"rtf",     FALSE, "text/rtf"},
  {"rtx",     FALSE, "text/richtext"},
  {"scm",     FALSE, "application/x-lotusscreencam"},
  {"set",     FALSE, "application/set"},
  {"sgm",     TRUE,  "text/sgml"},
  {"sgml",    TRUE,  "text/sgml"},
  {"sh",      FALSE, "application/x-sh"},
  {"shar",    FALSE, "application/x-shar"},
  {"silo",    FALSE, "model/mesh"},
  {"sit",     FALSE, "application/x-stuffit"},
  {"skd",     FALSE, "application/x-koan"},
  {"skm",     FALSE, "application/x-koan"},
  {"skp",     FALSE, "application/x-koan"},
  {"skt",     FALSE, "application/x-koan"},
  {"smi",     FALSE, "application/smil"},
  {"smil",    FALSE, "application/smil"},
  {"snd",     FALSE, "audio/basic"},
  {"sol",     FALSE, "application/solids"},
  {"spl",     FALSE, "application/x-futuresplash"},
  {"src",     FALSE, "application/x-wais-source"},
  {"step",    FALSE, "application/STEP"},
  {"stl",     FALSE, "application/SLA"},
  {"stp",     FALSE, "application/STEP"},
  {"sv4cpio", FALSE, "application/x-sv4cpio"},
  {"sv4crc",  FALSE, "application/x-sv4crc"},
  {"svg",     TRUE,  "image/svg+xml"},
  {"swf",     FALSE, "application/x-shockwave-flash"},
  {"t",       FALSE, "application/x-troff"},
  {"tar",     FALSE, "application/x-tar"},
  {"tcl",     FALSE, "application/x-tcl"},
  {"tex",     FALSE, "application/x-tex"},
  {"texi",    FALSE, "application/x-texinfo"},
  {"texinfo", FALSE, "application/x-texinfo"},
  {"tif",     FALSE, "image/tiff"},
  {"tiff",    FALSE, "image/tiff"},
  {"tr",      FALSE, "application/x-troff"},
  {"tsi",     FALSE, "audio/TSP-audio"},
  {"tsp",     FALSE, "application/dsptype"},
  {"tsv",     TRUE,  "text/tab-separated-values"},
  {"txt",     TRUE,  "text/plain"},
  {"unv",     FALSE, "application/i-deas"},
  {"ustar",   FALSE, "application/x-ustar"},
  {"vcd",     FALSE, "application/x-cdlink"},
  {"vda",     FALSE, "application/vda"},
  {"viv",     FALSE, "video/vnd.vivo"},
  {"vivo",    FALSE, "video/vnd.vivo"},
  {"vrml",    FALSE, "model/vrml"},
  {"wav",     FALSE, "audio/x-wav"},
  {"wrl",     FALSE, "model/vrml"},
  {"xbm",     FALSE, "image/x-xbitmap"},
  {"xlc",     FALSE, "application/vnd.ms-excel"},
  {"xll",     FALSE, "application/vnd.ms-excel"},
  {"xlm",     FALSE, "application/vnd.ms-excel"},
  {"xls",     FALSE, "application/vnd.ms-excel"},
  {"xlw",     FALSE, "application/vnd.ms-excel"},
  {"xml",     TRUE,  "text/xml"},
  {"xpm",     FALSE, "image/x-xpixmap"},
  {"xwd",     FALSE, "image/x-xwindowdump"},
  {"xyz",     FALSE, "chemical/x-pdb"},
  {"yml",     TRUE,  "application/x-yaml"},
  {"zip",     FALSE, "application/zip"}
};

char *
get_file_extension(char *file) 
{
  char *dot = strrchr(file, '.');
  if (!dot || dot == file)
    return "";
  return dot + 1;
}

char * 
get_content_type(char *file)
{
  int   i;
  char *ext;

  ext = get_file_extension(file);
  for (i=0; i < (int)sizeof(tmap) / (int)sizeof(tmap[0]); i++) {
    if (strmatch(ext, tmap[i].ext)) {
      return tmap[i].type;
    }
  }
  return tmap[0].type;
}

BOOLEAN
is_ascii(char *file) 
{
  int   i;
  char *ext;

  ext = get_file_extension(file);
  for (i=0; i < (int)sizeof(tmap) / (int)sizeof(tmap[0]); i++) {
    if (strmatch(ext, tmap[i].ext)) {
      return tmap[i].ascii;
    }
  }
  return tmap[0].ascii;
}

/**
 * maps a file to our address space 
 * and returns it the calling function.
 */
void
load_file(URL U, char *file)
{
  FILE     *fp;
  size_t   len;
  char     *buf;
  char     *filename;
  char     mode[8];

  filename = trim(file);

  memset(mode, '\0', sizeof(mode));
  printf("IS ASCII: %s\n", is_ascii(filename) ? "TRUE" : "FALSE");
  snprintf(mode, sizeof(mode), "%s", (is_ascii(filename))?"r":"rb");
  fp = fopen(filename, mode);
  if (! fp) {
    NOTIFY(ERROR, "unable to open file: %s", filename );
    return;
  }
 
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  buf = (char *)xmalloc(len+1);

  if ((fread(buf, 1, len, fp )) == len) {
    if (is_ascii(filename)) {
      buf[len] = '\0';
      trim(buf);
      len = strlen(buf);
    }
  } else {
    NOTIFY(ERROR, "unable to read file: %s", filename );
  }
  fclose(fp); 

  if (len > 0) {
    url_set_conttype(U, get_content_type(filename));
    url_set_postdata(U, buf, len);
  } 

  xfree(buf);
  return;
}

void
write_file(URL U, char *buf, size_t len)
{
  FILE *fp;
  char  mode[8];

  memset(mode, '\0', sizeof(mode));
  snprintf(mode, sizeof(mode), "%s", (url_get_file(U))?"w":"wb");
  fp = fopen(url_get_file(U), mode);

  if (fp) {
    fwrite(buf, len, 1, fp);
  } else {
    NOTIFY(ERROR, "unable to write to file");
  }

  fclose(fp);
}

#if 0
void 
load_file(URL U, char *file)
{
  FILE     *fp;
  size_t   len = 0;
  struct   stat st; 
  char     *filename;
  char     postdata[POSTBUF]; 
  size_t   postlen = 0;

  filename = trim(file);
  memset(postdata, 0, POSTBUF);

  if ((lstat(filename, &st) == 0) || (errno != ENOENT)) { 
    len = (st.st_size >= POSTBUF) ? POSTBUF : st.st_size;  
    if (len < (unsigned)st.st_size) {
      NOTIFY(WARNING, "Truncated file: %s exceeds the post limit of %d bytes.\n", filename, POSTBUF);
    }
    if ((fp = fopen(filename, "r")) == NULL) {
      NOTIFY(ERROR, "could not open file: %s", filename);
      return;
    }
    if ((fread(postdata, 1, len, fp )) == len) {
      if (is_ascii(filename)) {
        trim(postdata);
        postlen = strlen(postdata);
      } else {
        postlen = len;
      }
    } else {
      NOTIFY(ERROR, "unable to read file: %s", filename );
    }
    fclose(fp);
  }

  if (strlen(postdata) > 0) {
    url_set_conttype(U, get_content_type(filename));
    url_set_postdata(U, postdata, postlen);
  } 
  return;
}
#endif



