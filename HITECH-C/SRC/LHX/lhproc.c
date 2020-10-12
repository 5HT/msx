/* LHPROC.C -- Test/List/Extract routine for LHX
 *
 * version 1.0 by Mark Armbrust, 15 April 1985   (IRS? Just say NO!)
 *
 * The code in this module is public domain -- do whatever you want with it */

#include "stdio.h"
#include "ctype.h"
#define SEEK_SET 0
#define SEEK_CUR 1

#include "lhx.h"

/* private functions */

PRIVATE int  	ProcessData ();
PRIVATE int  	ProcessCompData();

/* private data */

PRIVATE char month[16][4] = {"0??","Jan","Feb","Mar","Apr","May","Jun","Jul",
							 "Aug","Sep","Oct","Nov","Dec","13?","14?","15?"};


int	Process(arcfile, filenames)

FILE	*arcfile;
struct fname	*filenames;
{
  struct lzhdir		header;
  unsigned char		sum;
  char				temp[100];
  unsigned		int	errors, i;
  unsigned long		next_hdr;
  FILE			*file;
  char			*p;
  char			*q;


  switch (mode & MODES)
  {
    case TEST:                                           
	  	printf ("\nTesting archive %s\n", name);           
	  	break;                                             
	  case LIST:                                           
	  	printf ("\nListing archive %s\n", name);           
	  	break;                                             
	  case EXTRACT:                                        
	  	printf ("\nExtracting archive %s\n", name);        
	  	break;                                             
  }

  errors = 0;

  if (header_start != -1)
	  fseek (arcfile, header_start,SEEK_SET); 

  while ( ! feof (arcfile) )
  {
	  header.header_len = (char) fgetc (arcfile);
	  if (feof (arcfile))
		  break;
	  if ( header_start == -1 && header.header_len == 0)
		  continue;

  	if ( header_start == -1 && (header.header_len < 21 || header.header_len > 98))
	  {
		  printf ("  invalid file header -- skipping remainder of file\n");
		  ++errors;
		  break;
	  }

  	if (header.header_len > 98)
	  	header.header_len = 98;

  	if ( ! fread (&header.header_sum, header.header_len + 1, 1, arcfile) &&
			 header_start == -1) 
	  {
		  printf ("  unexpected eof\n");
		  ++errors;
		  break;
	  }

    i = header.name_len;
	  if (i > header.header_len - 22)
		  i = header.header_len - 22;
	  strncpy (temp, header.filename, i);
	  temp[i] = '\0';

  	next_hdr = ftell (arcfile) + header.data_size;

  	if (file_name[0] || MatchAny (temp, filenames))
	  {

		  for (sum=0, i=0; i<header.header_len; ++i)
			  sum += ((char *) &header)[i+2];

		  header.file_CRC = *((int *)&header.filename[header.name_len]);

		  if (mode & LIST)
		  {
			  printf ("%c %c%cr%c%c%8ld /%8ld  %2d %3s %.2d  %.2d:%.2d:%.2d   %s\n",
					(header.header_sum == sum) ? ' ' : '?',
					'-' ,
					'-',
					'-',
					'-',
					header.file_size,
					header.data_size,
					header.file_date & 0x001F,
					month[(header.file_date & 0x01E0) >> 5],
					(((header.file_date & 0xFE00) >> 9) + 80) % 100,
					(header.file_time & 0xF800) >> 11,
					(header.file_time & 0x07E0) >> 5,
					(header.file_time & 0x001F) << 1,
					temp);
		  }
		  else
		  {
			  if (file_name[0])
				  printf ("  %s -- ", file_name);
			  else
				  printf ("  %s -- ", temp);

			  if ((mode & TEST) && header.header_sum != sum)
			  {
				  printf ("bad file header, ");
				  ++errors;
			  }

			  if (mode & TEST)
				  file = 0;
			  else
			  {
				  p = temp;
				  if (q = strrchr (p, '\\')) p = q + 1;
				  if (q = strrchr (p, '/')) p = q + 1;

				  if (file_name[0])
					  p = file_name;

				  if ( file = fopen (p, "r"))
				  {
					  fclose (file);
					  do {
						  fprintf (stderr, "\nfile exists; overwrite (y/n)? \007");
						  i = getkbd();
						  fprintf (stderr, "\n");
						  if ( (i=toupper(i)) == 'N')
							  goto NEXT_FILE;
					  } while (i != 'Y');
				  }

				  if ( ! (file = fopen (p, "w")) ) 
				  {
					  printf ("could not create file '%s'\n", p);
					  goto NEXT_FILE;
				  }
			  }

			  if (file_start != -1)
				  fseek (arcfile, file_start, SEEK_SET);
			  if (file_size != -1)
				  header.data_size = header.file_size = file_size;

			  if (file_type == 0 || strncmp (header.method, "-lh0-", 5) == 0)
			  {
				  errors += ProcessData (arcfile, header.file_size, header.file_CRC, file);
			  }
			  else if (file_type == 1 || strncmp (header.method, "-lh1-", 5) == 0)
			  {
				  errors += ProcessCompData (arcfile, header.data_size, header.file_size,
										header.file_CRC, file);
			  }
			  else 
			  {
				  printf ("unknown compression method -- file skipped\n");
				  ++errors;
			  }
			  if (file)
				  fclose (file);
		  }
	  }
NEXT_FILE:
  	if (file_start != -1)				/* if we had to tell it where the data starts,	*/
	  	fseek (arcfile, -1L, SEEK_CUR);	/* assume the next header immediatly follows.	*/
	  else
		  fseek (arcfile, next_hdr, SEEK_SET);
	  header_start = -1;
	  file_start = -1;
	  file_size = -1;
	  file_type = -1;

  	if (file_name[0])
	  	break;
  }
  if ((mode & TEST) || errors)
	  printf ("%d error%s detected\n", errors, (errors==1)?"":"s");
  return (errors);
}



PRIVATE int  	ProcessData (arcfile, data_size, file_CRC, file)

FILE			*	arcfile;
unsigned long		data_size;
unsigned int  		file_CRC;
FILE			*	file;
{
int  	count;

crccode = 0;
while (data_size) {
	if (data_size < BUFF_LEN)
		count = (int) data_size;
	else
		count = BUFF_LEN;

	if ( ! fread (buffer, count, 1, arcfile) ) {
		printf ("unexpected eof\n");
		return 1;
		}

	addbfcrc (buffer, count);

	if (file)
		if ( ! fwrite (buffer, count, 1, file) ) {
			printf ("write error; disk full?\n");
			return 1;
			}

	data_size -= count;
	}

if (crccode == file_CRC) {
	printf ("%s OK\n", (mode&TEST) ? "data" : "extracted");
	return 0;
	}
printf ("%s CRC error\n", (mode&TEST) ? "data" : "extracted with");
return 1;
}



PRIVATE int  	ProcessCompData (arcfile, data_size, file_size, file_CRC, file)

FILE			*arcfile;
unsigned long		data_size;
unsigned long 		file_size;
unsigned int  		file_CRC;
FILE			*	file;
{

crccode = 0;
textsize = file_size;
infile = arcfile;
outfile = file;

Decode ();
if (crccode == file_CRC) 
{
	printf ("%s OK\n", (mode&TEST) ? "data" : "extracted");
	return 0;
}
printf ("%s CRC error\n", (mode&TEST) ? "data" : "extracted with");
return 1;
}

static int getkbd()
{
  return bdos(1,0);  /* Get a character from the keyboard */
}  