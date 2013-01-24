/**
 * test-ole.c: OLE2 file format helper program,
 *             good for dumping OLE streams, and
 * corresponding biff records, and hopefuly
 * some more ...
 *
 * Author:
 *    Michael Meeks (michael@imaginator.com)
 **/

#define TEST_DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/* For ctime() */
#include <time.h>

#include <libole2/ms-ole.h>
#include <libole2/ms-ole-vba.h>
#include <libole2/ms-ole-summary.h>

#define BIFF_TYPES_FILE    "biff-types.h"

#if TEST_DEBUG > 0
static char delim[]=":";
#else
static char delim[]=" ";
#endif
static char **arg_data = NULL;
static int    arg_cur  = 0;

typedef struct {
	guint16 opcode;
	char *name;
} GENERIC_TYPE;

static GPtrArray *biff_types   = NULL;

static char *cur_dir = NULL;

static void
read_types (char *fname, GPtrArray **types)
{
	FILE *file = fopen(fname, "r");
	char buffer[1024];
	*types = g_ptr_array_new ();
	if (!file) {
		char *newname = g_strconcat ("../", fname, NULL);
		file = fopen (newname, "r");
	}
	if (!file) {
		printf ("Can't find vital file '%s'\n", fname);
		return;
	}
	while (!feof(file)) {
		char *p;
		fgets(buffer,1023,file);
		for (p=buffer;*p;p++)
			if (*p=='0' && *(p+1)=='x') {
				GENERIC_TYPE *bt = g_new (GENERIC_TYPE,1);
				char *name, *pt;
				bt->opcode=strtol(p+2,0,16);
				pt = buffer;
				while (*pt && *pt != '#') pt++;      /* # */
				while (*pt && !isspace(*pt)) pt++;  /* define */
				while (*pt &&  isspace(*pt)) pt++;  /* '   ' */
				while (*pt && *pt != '_') pt++;     /* BIFF_ */
				name = *pt?pt+1:pt;
				while (*pt && !isspace(*pt)) pt++;
				bt->name=g_strndup(name, (pt-name));
				g_ptr_array_add (*types, bt);
				break;
			}
	}
	fclose (file);
}

static char*
get_biff_opcode_name (guint16 opcode)
{
	int lp;
	if (!biff_types)
		read_types (BIFF_TYPES_FILE, &biff_types);
	/* Count backwars to give preference to non-filtered record types */
	for (lp=biff_types->len; --lp >= 0 ;) {
		GENERIC_TYPE *bt = g_ptr_array_index (biff_types, lp);
		if (bt->opcode>0xff) {
			if (bt->opcode == opcode)
				return bt->name;
		} else {
			if (bt->opcode == (opcode&0xff))
				return bt->name;
		}
	}
	return "Unknown";
}

static void
list_files (MsOle *ole)
{
	char     **names;
	MsOleErr   result;
	int        lp;

	result = ms_ole_directory (&names, ole, cur_dir);
	if (result != MS_OLE_ERR_OK) {
		g_warning ("Failed dir");
		return;
	}

	if (!names[0])
		printf ("Empty directory\n");

	for (lp = 0; names[lp]; lp++) {
		MsOleStat s;
		result = ms_ole_stat (&s, ole, cur_dir, names[lp]);

		if (s.type == MsOleStreamT)
			if (names[lp][0] < 0x30) {
				printf ("'\\%x%s' : length %d bytes\n",
					names[lp][0], names[lp] + 1, s.size);
			} else {
				printf ("'%s : length %d bytes\n",
					names[lp], s.size);
			}
		else
			if (names[lp][0] < 0x30) {
				printf ("'\\%d[%s]' : Storage ( directory )\n",
					names[lp][0], names [lp] + 1);
			} else {
				printf ("'[%s] : Storage ( directory )\n",
					names [lp]);
			}
		
	}
}

static void
list_commands ()
{
	printf ("command can be one or all of:\n");
	printf (" * ls:                   list files\n");
	printf (" * cd:                   enter storage\n");
	printf (" * biff    <stream name>:   dump biff records, merging continues\n");
	printf (" * biffraw <stream name>:   dump biff records no merge + raw data\n");
	printf (" * dump    <stream name>:   dump stream\n");
	printf (" * summary              :   dump summary info\n");
	printf (" * docsummary           :   dump document summary info\n");
	printf (" * debug                :   dump internal ole library status\n");
	printf (" * tree                 :   dump the tree\n");
	printf (" * vba                  :   attempt to dump vba \n");
	printf (" Raw transfer commands\n");
	printf (" * get     <stream name> <fname>\n");
	printf (" * put     <fname> <stream name>\n");
	printf (" * copyin  [<fname>,]...\n");
	printf (" * copyout [<fname>,]...\n");
	printf (" * quit,exit,bye:        exit\n");
}

static void
syntax_error (char *err)
{
	if (err) {
		printf("Error: '%s'\n",err);
		exit(1);
	}
		
	printf ("Sytax:\n");
	printf (" ole <ole-file> [-i] [commands...]\n\n");
	printf (" -i: Interactive, queries for fresh commands\n\n");

	list_commands ();
	exit(1);
}

/* ---------------------------- End cut ---------------------------- */

static gboolean
simple_regexp (const char *regexp, const char *fname)
{
	int      i;
	gboolean ret = TRUE;

	g_return_val_if_fail (fname != NULL, FALSE);
	g_return_val_if_fail (regexp != NULL, FALSE);

	for (i = 0; regexp [i] && fname [i]; i++) {
		if (regexp [i] == '.')
			continue;

		if (toupper (regexp [i]) != toupper (fname [i])) {
			ret = FALSE;
			break;
		}
	}

	if (regexp [i] && regexp [i] == '*')
		ret = TRUE;

	else if (!regexp [i] && fname [i])
		ret = FALSE;

/*	if (ret)
	printf ("'%s' matched '%s'\n", regexp, fname);*/

	return ret;
}

/*
 * This should take a path to check the directory out there.
 */ 
static char *
get_regexp_name (const char *regexp, const char *path, MsOle *ole)
{
	char      *res = NULL;
	char     **names;
	MsOleErr   result;
	int        lp;

	result = ms_ole_directory (&names, ole, path);
	if (result != MS_OLE_ERR_OK) {
		g_warning ("Failed dir");
		return NULL;
	}

	if (!names [0])
		printf ("Empty directory\n");

	for (lp = 0; names[lp]; lp++) {
		if (simple_regexp (regexp, names [lp])) {
			res = g_strdup (names [lp]);
			break;
		}
	}

	return res;
}

static void
enter_dir (MsOle *ole)
{
	char *newpath, *ptr, *p;

	p = arg_data [arg_cur++];

	if (!p) {
		printf ("Takes a directory argument\n");
		return;
	}

	if (!g_strcasecmp (p, "..")) {
		guint lp;
		char **tmp;
		GString *newp = g_string_new ("");

		tmp = g_strsplit (cur_dir, "/", -1);
		lp  = 0;
		if (!tmp[lp])
			return;

		while (tmp[lp+1]) {
			g_string_sprintfa (newp, "%s/", tmp[lp]);
			lp++;
		}
		g_free (cur_dir);
		cur_dir = newp->str;
		g_string_free (newp, FALSE);
	} else {
		MsOleStat s;
		MsOleErr  result;

		ptr = get_regexp_name (p, cur_dir, ole);
		if (!ptr)
			return;

		newpath = g_strconcat (cur_dir, ptr, "/", NULL);

		result = ms_ole_stat (&s, ole, newpath, "");
		if (result == MS_OLE_ERR_EXIST) {
			printf ("Storage '%s' not found\n", ptr);
			g_free (newpath);
			return;
		}
		if (result != MS_OLE_ERR_OK) {
			g_warning ("internal error");
			g_free (newpath);
			return;
		}
		if (s.type == MsOleStreamT) {
			printf ("Trying to enter a stream. (%d)\n", s.type);
			g_free (newpath);
			return;
		}

		g_free (cur_dir);
		cur_dir = newpath;
	}
}

static MsOleErr
test_stream_open (MsOleStream ** const stream, MsOle *f,
		  const char *path, const char *fname, char mode)
{
	MsOleErr err;
	char    *name;

	name = get_regexp_name (fname, path, f);
	if (name) {
		err = ms_ole_stream_open (stream, f, path, name, mode);
		g_free (name);
	} else /* Fall back to original */
		err = ms_ole_stream_open (stream, f, fname, name, mode);

	return err;
}


static void
do_dump (MsOle *ole)
{
	char        *ptr;
	MsOleStream *stream;
	guint8      *buffer;
	gchar	    *tname;

	ptr = arg_data [arg_cur++];
	if (!ptr) {
		printf ("Need a stream name\n");
		return;
	}

	tname = g_strdup (ptr);
	if (strcmp(tname, "SummaryInformation") == 0) {
	        printf ("Changing name to prepend 005\n");
		tname = "\05SummaryInformation";
	}
	if (strcmp(tname, "DocumentSummaryInformation") == 0) {
	        printf ("Changing name to prepend 005\n");
		tname = "\05DocumentSummaryInformation";
	}
	
	if (test_stream_open (&stream, ole, cur_dir, tname, 'r') !=
	    MS_OLE_ERR_OK) {
		printf ("Error opening '%s'\n", ptr);
		return;
	}
	buffer = g_malloc (stream->size);
	stream->read_copy (stream, buffer, stream->size);
	printf ("Stream : '%s' length 0x%x\n", ptr, stream->size);
	if (buffer)
		ms_ole_dump (buffer, stream->size);
	else
		printf ("Failed read\n");
	ms_ole_stream_close (&stream);
}

/* 
 * This is a massively cut down version ...
 */
typedef struct {
	guint16  opcode;
	guint32  length;        /* NB. can be extended by a continue opcode */
	guint8  *data;
	guint32  streamPos;
	MsOleStream *pos;
} BiffQuery;

static BiffQuery *
ms_biff_query_new (MsOleStream *ptr)
{
	BiffQuery *bq   ;
	if (!ptr)
		return 0;
	bq = g_new0 (BiffQuery, 1);
	bq->opcode = 0;
	bq->length = 0;
	bq->pos    = ptr;
	return bq;
}

static int
ms_biff_query_next (BiffQuery *bq)
{
	guint8  tmp[4];
	int ans=1;

	if (!bq || bq->pos->position >= bq->pos->size)
		return 0;
	if (bq->data)
		g_free (bq->data);

	bq->streamPos = bq->pos->position;
	if (!bq->pos->read_copy (bq->pos, tmp, 4))
		return 0;
	bq->opcode = MS_OLE_GET_GUINT16 (tmp);
	bq->length = MS_OLE_GET_GUINT16 (tmp+2);

	if (bq->length > 0) {
		bq->data = g_new0 (guint8, bq->length);
		if (!bq->pos->read_copy(bq->pos, bq->data, bq->length)) {
			ans = 0;
			g_free (bq->data);
			bq->length = 0;
		}
	}

	if (!bq->length) {
		bq->data = NULL;
		return 1;
	}

	return (ans);
}

static void
do_biff (MsOle *ole)
{
	char *ptr;
	MsOleStream *stream;
	
	ptr = arg_data[arg_cur++];
	if (!ptr) {
		printf ("Need a stream name\n");
		return;
	}
       
	if (test_stream_open (&stream, ole, cur_dir, ptr, 'r') !=
	    MS_OLE_ERR_OK) {
		printf ("Error opening '%s'\n", ptr);
		return;
	}
	{
		BiffQuery *q = ms_biff_query_new (stream);
		guint16 last_opcode=0xffff;
		guint32 last_length=0;
		guint32 count=0;
		while (ms_biff_query_next(q)) {
			if (q->opcode == last_opcode &&
			    q->length == last_length)
				count++;
			else {
				if (count>0)
					printf (" x %d\n", count+1);
				else
					printf ("\n");
				count=0;
				printf ("Opcode 0x%3x : %15s, length %d",
					q->opcode, get_biff_opcode_name (q->opcode), q->length);
			}
			last_opcode=q->opcode;
			last_length=q->length;
		}
		printf ("\n");
		ms_ole_stream_close (&stream);
	}
}

static void
do_biff_raw (MsOle *ole)
{
	char *ptr;
	MsOleStream *stream;
	
	ptr = arg_data[arg_cur++];
	if (!ptr) {
		printf ("Need a stream name\n");
		return;
	}
       
	if (test_stream_open (&stream, ole, cur_dir, ptr, 'r') !=
	    MS_OLE_ERR_OK) {
		printf ("Error opening '%s'\n", ptr);
		return;
	}
	{
		guint8 data[4], *buffer;
		
		buffer = g_new (guint8, 65550);
		while (stream->read_copy (stream, data, 4)) {
			guint32 len=MS_OLE_GET_GUINT16(data+2);
/*			printf ("0x%4x Opcode 0x%3x : %15s, length 0x%x (=%d)\n", stream->position,
				MS_OLE_GET_GUINT16(data), get_biff_opcode_name (MS_OLE_GET_GUINT16(data)),
				len, len);*/
			printf ("Opcode 0x%3x : %15s, length 0x%x (=%d)\n",
				MS_OLE_GET_GUINT16(data), get_biff_opcode_name (MS_OLE_GET_GUINT16(data)),
				len, len);
			stream->read_copy (stream, buffer, len);
			ms_ole_dump (buffer, len);
			buffer[0]=0;
			buffer[len-1]=0;
		}
		ms_ole_stream_close (&stream);
	}
}

static void
really_get (MsOle *ole, char *from, char *to)
{
	MsOleStream *stream;
	
	if (test_stream_open (&stream, ole, cur_dir, from, 'r') !=
	    MS_OLE_ERR_OK) {
		printf ("Error opening '%s'\n", from);
		return;
	} else {
		guint8 *buffer = g_malloc (stream->size);
		FILE *f = fopen (to, "w");
		stream->read_copy (stream, buffer, stream->size);
		printf ("Stream : '%s' length 0x%x\n", from, stream->size);
		if (f && buffer) {
			fwrite (buffer, 1, stream->size, f);
			fclose (f);
		} else
			printf ("Failed write to '%s'\n", to);
		ms_ole_stream_close (&stream);

	}
}

static void
do_get (MsOle *ole)
{
	char *from, *to;

	from = arg_data[arg_cur++];
	if (!from)
		to = NULL;
	else
		to = arg_data[arg_cur++];
	really_get (ole, from, to);
}

static void
really_put (MsOle *ole, char *from, char *to)
{
	MsOleStream *stream;
	char buffer[8200];

	if (!from || !to) {
		printf ("Null name\n");
		return;
	}

	if (test_stream_open (&stream, ole, cur_dir, to, 'w') !=
	    MS_OLE_ERR_OK) {
		printf ("Error opening '%s'\n", to);
		return;
	} else {
		FILE *f = fopen (from, "r");
		size_t len;
		int block=0;

		if (!f || !stream) {
			printf ("Failed write\n");
			return;
		}

		stream->lseek (stream, 0, MsOleSeekSet);
	       
		do {
			guint32 lenr = 1+ (int)(8192.0*rand()/(RAND_MAX+1.0));
			len = fread (buffer, 1, lenr, f);
			printf ("Transfering block %d = %d bytes\n", block++, len); 
			stream->write (stream, buffer, len);
		} while (!feof(f) && len>0);

		fclose (f);
		ms_ole_stream_close (&stream);
	}
}

static void
do_summary (MsOle *ole)
{
	MsOleSummary        *si;
	MsOleSummaryPreview  preview;
	gboolean             ok;
	gchar               *txt;
	guint32              num;
	GTimeVal             timeval;

	si = ms_ole_summary_open (ole);
	if (!si) {
		printf ("No summary information\n");
		return;
	}

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_TITLE, &ok);
	if (ok)
		printf ("The title is %s\n", txt);
	else
		printf ("no title found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_SUBJECT, &ok);
	if (ok)
		printf ("The subject is %s\n", txt);
	else
		printf ("no subject found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_AUTHOR, &ok);
	if (ok)
		printf ("The author is %s\n", txt);
	else
		printf ("no author found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_KEYWORDS, &ok);
	if (ok)
		printf ("The keywords are %s\n", txt);
	else
		printf ("no keywords found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_COMMENTS, &ok);
	if (ok)
		printf ("The comments are %s\n", txt);
	else
		printf ("no comments found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_TEMPLATE, &ok);
	if (ok)
		printf ("The template was %s\n", txt);
	else
		printf ("no template found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_LASTAUTHOR, &ok);
	if (ok)
		printf ("The last author was %s\n", txt);
	else
		printf ("no last author found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_REVNUMBER, &ok);
	if (ok)
		printf ("The rev no was %s\n", txt);
	else
		printf ("no rev no found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_APPNAME, &ok);
	if (ok)
		printf ("The app name was %s\n", txt);
	else
		printf ("no app name found\n");
	g_free (txt);

	timeval = ms_ole_summary_get_time (si, MS_OLE_SUMMARY_TOTAL_EDITTIME, &ok);
	if (ok)
		printf ("Total edit time is %ld", timeval.tv_sec);
	else
		printf ("no total edit time found\n");
	
	timeval = ms_ole_summary_get_time (si, MS_OLE_SUMMARY_LASTPRINTED, &ok);
	if (ok)
		printf ("Last printed at %s", ctime (&timeval.tv_sec));
	else
		printf ("no last printed time found\n");
	
	timeval = ms_ole_summary_get_time (si, MS_OLE_SUMMARY_CREATED, &ok);
	if (ok)
		printf ("Was created at %s", ctime (&timeval.tv_sec));
	else
		printf ("no creation time found\n");
	
	timeval = ms_ole_summary_get_time (si, MS_OLE_SUMMARY_LASTSAVED, &ok);
	if (ok)
		printf ("Last saved at %s", ctime (&timeval.tv_sec));
	else
		printf ("no last saved time found\n");
	
	num = ms_ole_summary_get_long (si, MS_OLE_SUMMARY_PAGECOUNT, &ok);
	if (ok)
		printf ("PageCount is %d\n", num);
	else
		printf ("no pagecount found\n");

	num = ms_ole_summary_get_long (si, MS_OLE_SUMMARY_WORDCOUNT, &ok);
	if (ok)
		printf ("WordCount is %d\n", num);
	else
		printf ("no wordcount found\n");

	num = ms_ole_summary_get_long (si, MS_OLE_SUMMARY_CHARCOUNT, &ok);
	if (ok)
		printf ("CharCount is %d\n", num);
	else
		printf ("no charcount found\n");
	
	num = ms_ole_summary_get_long (si, MS_OLE_SUMMARY_SECURITY, &ok);
	if (ok)
		printf ("Security is %d\n", num);
	else
		printf ("no security found\n");

	num = ms_ole_summary_get_short (si, MS_OLE_SUMMARY_CODEPAGE, &ok);
	if (ok)
		printf ("CodePage is %d\n", num);
	else
		printf ("no codepage found\n");

	preview = ms_ole_summary_get_preview (si, MS_OLE_SUMMARY_THUMBNAIL, &ok);
	if (ok) {
		printf ("preview is %d bytes long\n", preview.len);
		ms_ole_summary_preview_destroy (preview);
	} else
		printf ("no preview found\n");

	ms_ole_summary_close (si);
}

static void
do_docsummary (MsOle *ole)
{
	MsOleSummary        *si;
	gboolean             ok;
	gchar               *txt;

	si = ms_ole_docsummary_open (ole);
	if (!si) {
		printf ("No document summary information\n");
		return;
	}

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_CATEGORY, &ok);
	if (ok)
		printf ("The category is %s\n", txt);
	else
		printf ("no category found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_PRESFORMAT, &ok);
	if (ok)
		printf ("The presformat is %s\n", txt);
	else
		printf ("no presformat found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_MANAGER, &ok);
	if (ok)
		printf ("The manager is %s\n", txt);
	else
		printf ("no manager found\n");
	g_free (txt);

	txt = ms_ole_summary_get_string (si, MS_OLE_SUMMARY_COMPANY, &ok);
	if (ok)
		printf ("The company is %s\n", txt);
	else
		printf ("no company found\n");
	g_free (txt);

	ms_ole_summary_close (si);
}

static void
do_put (MsOle *ole)
{
	char *from, *to;

	from = arg_data[arg_cur++];
	if (!from)
		to = NULL;
	else
		to = arg_data[arg_cur++];

	if (!from || !to) {
		printf ("put <filename> <stream>\n");
		return;
	}

	really_put (ole, from, to);
}

static void
do_copyin (MsOle *ole)
{
	char *from;

	do {
		from = arg_data[arg_cur++];
		if (from)
			really_put (ole, from, from);
	} while (from);
}

static void
do_copyout (MsOle *ole)
{
	char *from;

	do {
		from = arg_data[arg_cur++];
		if (from)
			really_get (ole, from, from);
	} while (from);
}

static void
dump_vba (MsOle *f)
{
	const char	*vbapath = "/_VBA_PROJECT_CUR";
	char		**dir;
	char		*txt;

	int		 i;
	int		 module_count;

	MsOleStream	*s;
	MsOleStat	 st;

	if (ms_ole_stat (&st, f, vbapath, "") != MS_OLE_ERR_OK ||
	    st.type == MsOleStreamT) {
		printf ("No valid VBA found\n");
		return;
	}

	if (test_stream_open (&s, f, vbapath, "PROJECT", 'r') !=
	    MS_OLE_ERR_OK) {
		printf ("No project file... wierd\n");
	} else {
		txt = g_new (guint8, s->size);
		if (!s->read_copy (s, txt, s->size))
			printf ("Failed to read project stream\n");
		else {
			printf ("----------\n");
			printf ("Project file:\n");
			printf ("%s", txt);
			printf ("----------\n");
		}
		ms_ole_stream_close (&s);
		g_free (txt);
	}

	txt = g_strconcat (vbapath, "/VBA", NULL);
	if (ms_ole_directory (&dir, f, txt) != MS_OLE_ERR_OK) {
		printf ("No VBA subdirectory found");
		g_free (txt);
		return;
	}

	module_count = 0;
	for (i = 0; dir [i]; i++) {
		if (test_stream_open (&s, f, txt, dir[i], 'r') !=
		    MS_OLE_ERR_OK)
			printf ("Error opening %s\n", dir [i]);
		else {
			MsOleVba	*vba;

			vba = ms_ole_vba_open (s);
			if (!vba)
				g_warning ("Stream '%s' not VBA", dir [i]);
			else {
				module_count++;

				while (!ms_ole_vba_eof (vba))
					printf ("%c", ms_ole_vba_getc (vba));
			}
			ms_ole_vba_close (vba);

			ms_ole_stream_close (&s);
		}
	}

	if (!module_count)
		printf ("Strange no modules found\n");

	g_free (txt);
}

static void
hammer_stream (MsOleStream *stream, gboolean write)
{
	int     i, lp;
	guint8 *mem;

#define BLOCK_LEN  1000
#define NUM_BLOCKS 7100

	for (lp = 0; lp < NUM_BLOCKS; lp++) {

		mem = g_malloc (BLOCK_LEN);

		if (!write)
			g_assert (stream->read_copy (
				stream, mem, BLOCK_LEN));

		for (i = 0; i < BLOCK_LEN; i++) {
			guint8 sig;

			sig = 0; //(i << 3) ^ (lp);
			if (write)
				mem [i] = sig;
			else {
				if (i == 0 && lp == 0) {
					/* Ok, this dump shows how:

					   Old value = 0 '\000'
					   New value = 154 '\232'
					   0x4001bf17 in write_bb (f=0x804e638) at ms-ole.c:916
					   916			SET_BBD_LIST (f, lp, blk);
					   (gdb) bt
					   #0  0x4001bf17 in write_bb (f=0x804e638) at ms-ole.c:916
					   #1  0x4001e55f in ms_ole_cleanup (f=0x804e638) at ms-ole.c:1538
					   #2  0x4001f349 in ms_ole_destroy (ptr=0xbffff6f4) at ms-ole.c:1873
					   #3  0x0804b757 in do_regression_tests () at test-ole.c:953
					   #4  0x0804b9ad in main (argc=2, argv=0xbffff78c) at test-ole.c:985

					   Corrupt the data in the stream - libole2 does not work with
					   large files.
					 */
					ms_ole_dump (mem, 64);
				}
				if (i < 32 && lp == 0)
					continue;

				if (mem [i] != sig)
					g_error ("Mismatch 0x%x 0x%x", mem [i], sig);
			}
		}

		if (write)
			stream->write (stream, mem, BLOCK_LEN);
	}
}

#define REG_NAME "/tmp/ole-file"
#define STREAM_NAME "foo"

static void
do_regression_tests (void)
{
	MsOle       *ole;
	MsOleStream *stream;
	MsOleStat    stat;

	if (ms_ole_create_vfs (&ole, REG_NAME, TRUE, NULL))
		g_error ("Couldn't open '" REG_NAME "'");

	ms_ole_debug (ole, 1);

	g_print ("\n\nwrite tests\n");
	g_assert (!ms_ole_stream_open (&stream, ole, "/", STREAM_NAME, 'w'));
	hammer_stream (stream, TRUE);
	g_assert (!ms_ole_stream_close (&stream));

	ms_ole_debug (ole, 1);

	g_print ("\n\ncheck attrs.\n");
	g_assert (!ms_ole_stat (&stat, ole, "/", STREAM_NAME));
	g_assert (stat.type == MsOleStreamT);
	g_assert (stat.size == BLOCK_LEN * NUM_BLOCKS);

	g_print ("\n\nread tests\n");
	g_assert (!ms_ole_stream_open (&stream, ole, "/", STREAM_NAME, 'r'));
	hammer_stream (stream, FALSE);
	g_assert (!ms_ole_stream_close (&stream));

	g_print ("\n\ncheck attrs.\n");
	g_assert (!ms_ole_stat (&stat, ole, "/", STREAM_NAME));
	g_assert (stat.type == MsOleStreamT);
	g_assert (stat.size == BLOCK_LEN * NUM_BLOCKS);

	g_print ("\n\nclosing\n");
	ms_ole_destroy (&ole);

	g_print ("\n\nre-opening\n");
	g_assert (!ms_ole_open_vfs (&ole, REG_NAME, TRUE, NULL));
	ms_ole_debug (ole, 1);

	g_print ("\n\ncheck attrs.\n");
	g_assert (!ms_ole_stat (&stat, ole, "/", STREAM_NAME));
	g_assert (stat.type == MsOleStreamT);
	g_assert (stat.size == BLOCK_LEN * NUM_BLOCKS);

	g_print ("\n\nre-read tests\n");
	g_assert (!ms_ole_stream_open (&stream, ole, "/", STREAM_NAME, 'r'));
	hammer_stream (stream, FALSE);
	g_assert (!ms_ole_stream_close (&stream));
	ms_ole_debug (ole, 1);

	ms_ole_destroy (&ole);
	g_print ("\nPassed\n\n");
}

int
main (int argc, char **argv)
{
	MsOle *ole;
	int lp, exit = 0, interact = 0;
	char *buffer = g_new (char, 1024) ;

	if (argc<2)
		syntax_error(0);

	if (!g_strcasecmp (argv [1], "regression")) {
		do_regression_tests ();
		return 0;
	}

	printf ("Ole file '%s'\n", argv[1]);
	if (ms_ole_open_vfs (&ole, argv[1], TRUE, NULL)
	    != MS_OLE_ERR_OK) {
		printf ("Creating new file '%s'\n", argv[1]);
		if (ms_ole_create_vfs (&ole, argv[1], TRUE, NULL)
		    != MS_OLE_ERR_OK)
			syntax_error ("Can't open file or create new one");
	}

	if (argc<=2)
		syntax_error ("Need command or -i");

	if (argc>2 && argv[argc-1][0]=='-'
	    && argv[argc-1][1]=='i') 
		interact=1;
	else {
		char *str=g_strdup(argv[2]) ;
		for (lp=3;lp<argc;lp++)
			str = g_strconcat(str," ",argv[lp],NULL); /* FIXME Mega leak :-) */
		buffer = str; /* and again */
	}

	cur_dir = g_strdup ("/");

	do
	{
		char *ptr;

		if (interact) {
			fprintf (stdout,"> ");
			fflush (stdout);
			fgets (buffer, 1023, stdin);
		}

		arg_data = g_strsplit (g_strchomp (buffer), delim, -1);
		arg_cur  = 0;
		if (!arg_data && interact) continue;
		if (!interact)
			printf ("Command : '%s'\n", arg_data[0]);
		ptr = arg_data[arg_cur++];
		if (!ptr)
			continue;

		if (g_strcasecmp (ptr, "ls") == 0)
			list_files (ole);
		else if (g_strcasecmp (ptr, "cd") == 0)
			enter_dir (ole);
		else if (g_strcasecmp (ptr, "dump") == 0)
			do_dump (ole);
		else if (g_strcasecmp (ptr, "biff") == 0)
			do_biff (ole);
		else if (g_strcasecmp (ptr, "biffraw") == 0)
			do_biff_raw (ole);
		else if (g_strcasecmp (ptr, "get") == 0)
			do_get (ole);
		else if (g_strcasecmp (ptr, "put") == 0)
			do_put (ole);
		else if (g_strcasecmp (ptr, "copyin") == 0)
			do_copyin (ole);
		else if (g_strcasecmp (ptr, "copyout") == 0)
			do_copyout (ole);
		else if (g_strcasecmp (ptr, "summary") == 0)
			do_summary (ole);
		else if (g_strcasecmp (ptr, "docsummary") == 0)
			do_docsummary (ole);
		else if (g_strcasecmp (ptr, "debug") == 0)
			ms_ole_debug (ole, 1);
		else if (g_strcasecmp (ptr, "tree") == 0)
			ms_ole_debug (ole, 2);
		else if (g_strcasecmp (ptr, "vba") == 0)
			dump_vba (ole);
		else if (g_strcasecmp (ptr, "help") == 0 ||
			 g_strcasecmp (ptr, "?") == 0 ||
			 g_strcasecmp (ptr, "info") == 0 ||
			 g_strcasecmp (ptr, "man") == 0)
			list_commands ();
		else if (g_strcasecmp (ptr, "exit") == 0 ||
			 g_strcasecmp (ptr, "quit") == 0 ||
			 g_strcasecmp (ptr, "q") == 0 ||
			 g_strcasecmp (ptr, "bye") == 0)
			exit = 1;
	}
	while (!exit && interact);

	ms_ole_destroy (&ole);
	return 0;
}
