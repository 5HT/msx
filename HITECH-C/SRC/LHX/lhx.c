/* LHX.C -- An LHARC archive tester/scanner
 *
 * version 1.0 by Mark Armbrust, 15 April 1985   (IRS? Just say NO!)
 *
 * The code in this module is public domain -- do whatever you want with it */

/* #include <process.h> */
#include "stdio.h"

#define MAIN
#include "lhx.h"

/* private routines */

PRIVATE void	Usage		();
PRIVATE int 	Match		();
PRIVATE int 	WC_Match	();
PRIVATE  char *strpbrk();
PRIVATE  char *strdup();


void		main	(argc, argv)

int argc;
char *argv[];
{
char			*arcname;
FILE			*	arcfile;
struct fname	*fnam,
				*fnams;
char		*p,
				*	q;
int					i;
int					exitcode;


if (argc < 2)
	Usage();

--argc;
++argv;

mode = LIST;					/* set default mode */

if (**argv == '-') {			/* get mode switch if present */
	switch (*++*argv) {

		case 'e':
		case 'E':
			mode = EXTRACT;
			break;

		case 'l':
		case 'L':
			mode = LIST;
			break;

		case 's':
		case 'S':
			mode = SCAN;
			break;

		case 't':
		case 'T':
			mode = TEST;
			break;

		default:
			Usage();

		}
	--argc;
	++argv;
	}

header_start = -1;				/* disable starting offset options */
file_start = -1;
file_size = -1;
file_type = -1;
file_name[0] = 0;

if (argc && **argv == '@') {	/* get starting offsets */
	if (mode & SCAN)
		fprintf (stderr, "lhx:  WARNING -- offsets ignored.\n");
	else {
		mode |= AT;
		i = sscanf (++*argv, "%ld,%ld,%ld,%d,%s", &header_start, &file_start, 
					&file_size, &file_type, file_name);
		if (!i)
			Usage();
		}
	--argc;
	++argv;
	}

if (argc) {						/* get archive name */
	arcname = *argv;
	--argc;
	++argv;
	}
else
	Usage();

if (argc == 0) {				/* get file names if present */
	fnams = malloc (sizeof (struct fname));
	fnams->name = "*.*";
	fnams->next = 0;		/* none specified, assume *.* */
	}
else {
	if (mode & SCAN)
		fprintf (stderr, "lhx:  WARNING -- fnams ignored.\n");
	else {
		fnams = 0;				/* use the rest of the names to build a */
		while (argc--) {			/* linked list of fnams to match.	*/
			fnam = malloc (sizeof (struct fname));
			fnam->name = *argv++;
			fnam->next = fnams;
			fnams = fnam;
			}
		}
	}

strcpy (name, arcname);

p =name;
if ( ! strchr (p, '.'))
	strcat (name, ".lzh");

if (! fopen(name,"r"))
{
	fprintf (stderr, "lhx:  ERROR -- could not find any archive files \
matching '%s'.\n", name);
	exit (-1);
	}

exitcode = 0;

arcfile = fopen (name, "r");
if ( ! arcfile) {
  fprintf (stderr, "lzx:  ERROR -- could not open archive file '%s'.\n",
				name);
	exit (-1);
}

switch (mode & MODES) {

		case EXTRACT:
		case LIST:
		case TEST:
			exitcode += Process (arcfile, fnams);
			break;

		case SCAN:
			Scan(arcfile);
			break;
}

fclose (arcfile);

exit (exitcode);
}



PRIVATE void	Usage	()

{
printf ("\n");
printf ("usage: lhx [-elst] [@hdr,data,size,type,name] arcname [filename ...]\n\n");
printf ("           E => extract files\n");
printf (" (default) L => list archive contents\n");
printf ("           S => scan damaged archive\n");
printf ("           T => test archive\n\n");
printf ("           @hdr,data,size,type,name => where to start in a damaged archive.\n");
exit (1);
}



PUBLIC int	MatchAny (name, fnams)

char			*name;
struct fname	*fnams;
{

do {
	if (Match (name, fnams->name))
		return 1;
	fnams = fnams->next;
	}
while (fnams);

return 0;
}



PRIVATE int 	Match (file, pattern)

	char			*file;
	char			*pattern;
	{
	int				i;
	register char	*fnam;
	register char	*s;


/* strip off the leading path and trailing modifiers (zoo ver. number, etc.) */

	fnam = file;
	if (s = strrchr (fnam, '/')) fnam = s+1;
	if (s = strrchr (fnam, '\\')) fnam = s+1;
/*	fnam = strdup (fnam);
	if (!fnam)
	{
	  fprintf(stderr,"fnam strdup failed in Match"); 
	  exit(1);
	}  
	if (s = strpbrk (fnam, ";*, ")) *s = '\0';
*/

	i = WC_Match (fnam, pattern);

	/* free (fnam); */

	return i;
	}



/*	Returns TRUE if s1 and s2 match under modified MSDOS wildcard rules.
 *
 *	Two strings match if all their substrings between : . / and \ match
 *	using * and ? as wildcards in the substrings.
 *
 *	Note that MSDOS is a bit schitzo about matching strings that end in : and .
 *	This code will only match them 1-to-1.  For example 'DIR X' matches X.* or
 *	X\*.* depending on what X is.  We do, however, match 'X' to 'X.' and 'X.*'
 *
 */

PRIVATE int 	WC_Match (s1, s2)

	register	char	*s1;
	register	char	*s2;
	{

#define IS_SLANT(c)		((c) == '/' || (c) == '\\')
#define IS_FILE_SEP(c)	( (c) == ':' || (c) == '.' || IS_SLANT(c) )

	while (1) {

		if (*s1 == '*' || *s2 == '*') {
			while (*s1 && ! IS_FILE_SEP (*s1))
				++s1;
			while (*s2 && ! IS_FILE_SEP (*s2))
				++s2;
			continue;
			}

		if (*s1 == '?') {
			if (!*s2 || IS_FILE_SEP (*s2))
				return 0;
			++s1;
			++s2;
			continue;
			}

		if (*s2 == '?') {
			if (!*s1 || IS_FILE_SEP (*s1))
				return 0;
			++s1;
			++s2;
			continue;
			}

		if (*s1 == 0 && *s2 == '.') {
			++s2;
			continue;
			}

		if (*s2 == 0 && *s1 == '.') {
			++s1;
			continue;
			}

		if (toupper(*s1) != toupper(*s2))
			return 0;

		if (*s1 == 0)		/* then (*s2 == 0) as well! */
			break;

		++s1;
		++s2;
		}

	return 1;
	}

PRIVATE char *strdup(fname)
char *fname;
{
  char *p;
  p = (char *) malloc(strlen(fname) + 1);
  if (p != NULL)
  {
    strcpy(p,fname);
    return(p);
  }
  else
    return(NULL);
}      

PRIVATE char *strpbrk(string,brkset)
char *string,*brkset;
{
  register char *p;
  do
  {
    for (p=brkset; *p != '\0' && *p != *string; ++p)
      if (*p!= '\0')
        return(string);
  }
  while (*string++);
  return(NULL);       
}
