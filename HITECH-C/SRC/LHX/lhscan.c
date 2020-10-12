/* LHSCAN.C -- Scan routine for LHX
 *
 * version 1.0 by Mark Armbrust, 15 April 1985   (IRS? Just say NO!)
 *
 * The code in this module is public domain -- do whatever you want with it */

#include "stdio.h"

#include "lhx.h"

/* private functions */

PRIVATE void	CheckHeader ();

void		Scan	(arcfile)

FILE			*arcfile;
{
unsigned int  		offset, buff_len;
unsigned long		file_offset;


printf ("\nScanning file %s\n", name);

offset = 0;
file_offset = 0;

for (;;) {
	buff_len = offset + fread (&buffer[offset], 1, BUFF_LEN - offset, arcfile);

	offset = 0;
	while (offset + 100 < buff_len) {
		CheckHeader (offset, file_offset);
		++offset;
		++file_offset;
		}

	if (feof (arcfile))
		break;

	memcpy (buffer, &buffer[offset], buff_len - offset);
	offset = buff_len - offset;
	}

while (offset + 22 < buff_len) {
	CheckHeader (offset, file_offset);
	++offset;
	++file_offset;
	}

}



PRIVATE void	CheckHeader (offset, file_offset)

unsigned int  			offset;
unsigned long			file_offset;
{
struct lzhdir		*	header;
char					name[40];
unsigned int  			n, i;
char					type;
unsigned char			sum, points;


header = (struct lzhdir *) &buffer[offset];

points = 0;

if ( (n = header->header_len) > 22 && header->header_len <= 98) {
	++points;
	for (sum = 0, i = 0; i < n; ++i)
		sum += buffer[offset + 2 + i];
	if (header->header_sum == sum)
		++points;
	}

if (n - 22 == header->name_len) ++ points;

if (header->method[0] == '-') ++points;
if (header->method[1] == 'l') ++points;
if (header->method[2] == 'h') ++points;
if ((type = header->method[3]) == '0' || type == '1' )
    ++points;
else
	type = '?';
if (header->method[4] == '-') ++points;

if (header->file_attr & 0xFF00 == 0) ++points;
if (header->data_size <= header->file_size) ++points;

n = header->name_len;
if (n > 39) n = 39;
memcpy (name, header->filename, n);
name[n] = 0;
for (i = 0; i<n; ++i)
	if (name[i] < ' ') name[i] = '?';

if (points >= 5) {
	
	printf ( "  @%ld,{%ld | %ld},%ld,%c  %s\n",
			file_offset,
			file_offset + header->header_len + 2,
			file_offset + header->name_len + 24,
			(header->file_size <= 99999999) ? header->file_size : 0,
			type,
			name);
	}

}
