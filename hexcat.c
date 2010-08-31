/*
    hexcat

    ======================================================================

    Writes 'hex-editor-like' representation of file or stdin to stdout.

    Command options:

      -c n    use n columns
      -s n    put n bytes into each column

    ======================================================================

    Copyright 2010 Joerg Haubrichs <joerghaubrichs@googlemail.com>


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>
#include <string.h>


// convert a number between 0 and 15 in hex representation
#define hexchar(c) (((c < 10) ? '0' : 'A' ) + (c % 10))


/* ----------------------------------------------------------------------
 *
 *    Writes the hex representation of an integer to buf.
 *    Used to avoid sprintf.
*/
void writehex(char * buf, unsigned int num)
{
    for(int i = (1 << 8*sizeof(num)-4); i>0; i >>= 4) {
        *(buf++) = hexchar(num / i);
        num %= i;
    }
}


/* ----------------------------------------------------------------------
 *
 *    Output options
*/
typedef struct 
{
    unsigned int col_size;
    unsigned int col_count;
    unsigned int col_space;
    unsigned int byte_space;

    unsigned int rowbytes;
    unsigned int linesize;
} 
flag_options;


/* ----------------------------------------------------------------------
 *
 *    Calculate output line size from given formatting options
*/
int calc_linesize(flag_options o)
{
    return
    sizeof(int)*2                                     // byte offset column
    + o.col_space
    + (o.rowbytes * 2)                                // amount of bytes
    + ((o.col_count-1) * o.col_space)                 // space between columns
    + ((o.col_size-1) * o.col_count * o.byte_space)   // space between bytes
    + o.col_space                 
    + o.rowbytes;                                     // character col_umn
}


/* ----------------------------------------------------------------------
 *
 *    Read command line options
*/
flag_options prepare_options(int argc, char *argv[]) 
{
    flag_options o = {5,4,2,1,
                      0,0};
    int opt;
    optarg = NULL;
    while ((opt = getopt (argc, argv, "s:c:")) != -1) {
        switch (opt) {
            case 's':
                o.col_size = atoi(optarg);
                break;
            case 'c':
                o.col_count = atoi(optarg);
                break;
        }
    }
    o.rowbytes = o.col_size * o.col_count;
    o.linesize = calc_linesize(o);
    return o;
}


/* ----------------------------------------------------------------------
 *
 *    Cat a file stream as hex edit output
*/
void hexcat ( FILE * in_file, 
              FILE * out_file, 
              flag_options o )
{
    unsigned char * data = (unsigned char *) malloc (o.rowbytes);
    char * line = (char *) malloc (o.linesize+1);
    line[o.linesize] = '\n';

    unsigned int offset = 0;
    char *pos, *charspos;
    int blocksize;

    while ((blocksize = fread(data, 1, o.rowbytes, in_file))) {
        memset(line, ' ', o.linesize);
        writehex(line, offset);
        pos = line + 10;
        charspos = line + o.linesize - o.rowbytes;
        
        for (int j = 0; j < blocksize; j++) {
            // write hex byte
            *(pos++) = hexchar(data[j]/16);
            *(pos++) = hexchar(data[j]%16);
            // jump to next position
            pos += (((j+1) % o.col_size)) ? o.byte_space : o.col_space;
            //
            // dump character into character column if printable
            *(charspos++) = (isgraph(data[j])) ? data[j] : '.';
        }

        fwrite(line, o.linesize+1, 1, out_file);
        offset += o.rowbytes;
    } 
}


/* ======================================================================
*/
int main(int argc, char *argv[]) 
{
    flag_options o = prepare_options(argc,argv);

    // TODO filename and parameters are still mixed

    FILE * f;
    if (argc > 1)
        f = fopen(argv[argc-1], "rb");
    else
        f = stdin;
    hexcat(f, stdout, o);
    fclose(f);
    
    return 0;
}
