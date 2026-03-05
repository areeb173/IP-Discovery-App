/*************************************************************************/
/*                                                                       */
/* Analyze Load Modules:                                                 */
/*                                                                       */
/*  analyze_load_modules [-f filename] [-list] [-d 0xNNNN] [-r rptname]  */
/*                                                                       */
/*  No parms, will analyze all source members of type 'LOAD'.            */
/*                                                                       */
/*        -f -> process only filename                                    */
/*     -list -> list all amblist type information                        */
/*        -r -> Report filename                                          */
/*        -d -> debugging options.                                       */
/*                                                                       */
/* Determine the names of the CSECTS within load modules.                */
/* and build exec bin and compile unit cross_reference.                  */
/*                                                                       */
/* Modifications:                                                        */
/*    12/6/90 -     - Original Coding                                    */
/*   12/18/90 -     - Add Csect xref report.                             */
/*   05/13/92 -     - Add handing of PLI CSECTS. added routine           */
/*                    b318_check_for_plip_mod                            */
/*   05/30/92 -     - change page length                                 */
/*   05/30/92 -     - add noupdt option to read all load modules from    */
/*                    the database but only product the reports          */
/*   05/25/95 -      - change labels to "ASA" from "SCI".		 */
/*                                                                       */
/*************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>
#include <stdarg.h>
#include <fcntl.h>
#include <re.h>
#include <err_msg.h>
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif 

/******************************************************************/
/*                                                                */
/*  EBCDIC to ascii translation tables.                           */
/*                                                                */
/******************************************************************/
#define etoa(x)     (x = asc[(unsigned char) x] & 0x7f)
static unsigned char asc[] =
    {   /*asc table*/
        /*    0     1     2     3     4     5     6     7   */
/* 00 */    0x20, 0x01, 0x02, 0x03, 0x9c, 0x09, 0x86, 0x7f, /*00 was 00   */
/* 08 */    0x97, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
/* 10 */    0x10, 0x11, 0x12, 0x13, 0x9d, 0x0a, 0x08, 0x87,
/* 18 */    0x18, 0x19, 0x92, 0x8f, 0x1c, 0x1d, 0x1e, 0x1f,
/* 20 */    0x80, 0x81, 0x82, 0x83, 0x84, 0x0a, 0x17, 0x1b,
/* 28 */    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07,
/* 30 */    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
/* 38 */    0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a,
/* 40 */    0x20, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
/* 48 */    0xa7, 0xa8, 0xd5, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,
/* 50 */    0x26, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
/* 58 */    0xb0, 0xb1, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x5e,/*5H entry was 7e*/
/* 60 */    0x2d, 0x2f, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
/* 68 */    0xb8, 0xb9, 0xcb, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
/* 70 */    0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1,
/* 78 */    0xc2, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,
/* 80 */    0xc3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
/* 88 */    0x68, 0x69, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
/* 90 */    0xca, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
/* 98 */    0x71, 0x72, 0x7e, 0x7e, 0xcd, 0xce, 0xcf, 0xd0,
/* a0 */    0xd1, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,/*A1 entry was e5*/
/* a8 */    0x79, 0x7a, 0xd2, 0xd3, 0xd4, 0x5b, 0xd6, 0xd7,
/* b0 */    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
/* b8 */    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x5d, 0xe6, 0xe7,
/* c0 */    0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
/* c8 */    0x48, 0x49, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed,
/* d0 */    0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
/* d8 */    0x51, 0x52, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3,
/* e0 */    0x5c, 0x9f, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
/* e8 */    0x59, 0x5a, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
/* f0 */    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
/* f8 */    0x38, 0x39, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
/*************************************************************************/
/*                                                                       */
/* Sybase Variables.                                                     */
/*                                                                       */
/*************************************************************************/
#define MAX_DB_OPEN_RETRIES  5
#define DB_USER "sa"
#define DB_PASSWORD ""
#define DB_NAME "oredre"
static  LOGINREC   *login;
static  DBPROCESS  *dbproc;
static  long    in_open = FALSE;
static	long	no_database = FALSE;
static	long	update = TRUE;

/*****************************************************************/
/*                                                               */
/*  Various processing areas.                                    */
/*                                                               */
/*****************************************************************/
static  long    debug = 0;
static  long    nbr_error_messages = 0;
static  long    nbr_warn_messages = 0;
static  char    xlate[256];
#define MSG_ALWAYS    0x0000
#define MSG_TRACE     MSG_ALWAYS /*0x0001*/
#define MSG_LIST      0x0002  /* List all amblist information */
#define MSG_LISTRC    0x0004  /* List amblist ctl rec info    */
#define MSG_CSECTS    0x0008  /* Print all csects encountered */
#define MSG_DB        0x0010
#define MSG_DBMSGS    0x0020
#define MSG_UNK3      0x0040
#define MSG_UNK4      0x0080
#define MSG_UNK5      0x0100

extern  int errno;
static  char    member_name[512];
static  char    filename[1024];
static  long    len_added;
#define INPUT_BUF_SIZE 65536
static  unsigned char    input_rec[INPUT_BUF_SIZE];
static  long    rec_len;
static  int     inputfd;
static  FILE    *input;
static	long    exec_bin_key;
static	long	pli_load_module = FALSE;
static	FILE	*report_file;
static	char	report_extract_filename[256];
static	char	report_filename[256] = "./load_module_reports";
struct	esd_entry
  {
	struct	esd_entry	*next;
	struct	esd_entry	*prev;
	long	nbr;
	char    name[12];
	char	product_name[32];
	char	product_ver[16];
	char	build_date_mmddyy[12];
	char	build_date_yymmdd[12];
	long	unknown_source;
	long	pli_load_module;
  };
static	struct	esd_entry	*esd_head,*esd_tail;
#define ADD_TO_ESD_LIST(NODE) ADD_TO_LIST(NODE,esd_head,esd_tail)
#define FOR_EACH_ESD_NODE(NODE) FOR_EACH_NODE(NODE,esd_head)

/*****************************************************************/
/*                                                               */
/*  IBM Product line information.                                */
/*                                                               */
/*****************************************************************/
struct	product_entry
  {
	 char	name[20];
	 char	type[32];
  };
static	struct	product_entry product[] = {
{  	"5601037",   "Fortran VS",   },
{	"5601038",   "Pascal VS",    },
{	"5665948",   "Basic/MVS",    },
{	"5666274",   "RPG II", },
{	"5666276",   "PLI Optimizing", },
{	"5688022",   "COBOL II", },
{	"5688023",   "COBOL II", },
{	"5668958",   "COBOL II R3.0", },
{	"5713AAH",   "C for S/370", },
{	"5740CB1",   "COBOL OS/VS", },
{	"5746CB1",   "COBOL OS/VS", },
{	"5740CB2",   "COBOL 3650",    },
{	"5668962",   "Assembler H - V2", },
{	"5734-PL1",  "PLI Optimizing",  },
{	"5734-PL3",  "PLI Optimizing",  },
{	"5734-PL2",  "PLI Checkout",  },
{	".",         "Unknown Product", },
  };
static	long	nbr_product_names = sizeof(product)/sizeof(product[0]);

/*****************************************************************/
/*                                                               */
/*  Report print line information.                               */
/*                                                               */
/*****************************************************************/
static	long	rpt1_page_nbr = 0;
static	char	rpt1_hdr1 [] = 
"\n\n         ASA Reverse Engineering         Missing Source Report        Page %d\n";

static	long	rpt2_page_nbr = 0;
static	char	rpt2_hdr1 [] = 
"\n         ASA Reverse Engineering           PLI Module Report          Page %d\n";

static	long	rpt3_page_nbr = 0;
static	char	rpt3_hdr1 [] = 
"\n         ASA Reverse Engineering      CSECT Inconsistency Report      Page %d\n";
static	long	rpt4_page_nbr = 0;
static	char	rpt4_hdr1 [] = 
"\n         ASA Reverse Engineering     CSECT Cross Reference Report     Page %d\n";
static	char	rpt4_hdr2 [] = 
"*-CSECT*   *------------------------- Load Modules -------------------------*\n";
/*****************************************************************/
/*                                                               */
/*  Strucutures contained in a load module                       */
/*                                                               */
/*****************************************************************/
#define BLOCK_CONTROL 0x01
#define BLOCK_RLD     0x02
#define BLOCK_CTLRLD  0x03
#define BLOCK_RLD1    0x0E
#define BLOCK_CESD    0x20
#define BLOCK_IDR     0x80

struct	cesd_base_entry
  {
	unsigned char   block_type;
	unsigned char   unknown;
	unsigned short	unknown1;
	unsigned short	esd_id;
	unsigned short	esd_size;
  };
static	struct	cesd_base_entry *cesd_base;
#define CESD_TYPE_SD   0x00    /* Section Definition */
#define CESD_TYPE_ER   0x02    /* External Reference */
#define CESD_TYPE_LR   0x03    /*                    */
#define CESD_TYPE_PR   0x06    /* Pseudo-Register    */
#define CESD_TYPE_WX   0x0a    /* Weak External      */
#define CESD_TYPE_CM   0x17    /* Common Area        */
#define CESD_TYPE_LD   0x18    /* Label Definition   */
#define CESD_TYPE_NULL 0x07

struct	cesd_entry
  {
	char name[8];
	unsigned char type;
	unsigned char addr[3];
	unsigned char rra;
	unsigned char id_length[3];
  };

#define IDR_TYPE_ZAP     0x01
#define IDR_TYPE_LINKER  0x02
#define IDR_TYPE_XLATER  0x04
#define IDR_TYPE_USER    0x08
struct	idr_base_entry
  {
	unsigned char   block_type;
	unsigned char   idr_length;  /* This length includes itself and the subtype */
	unsigned char   idr_subtype;
  };
static	struct	idr_base_entry *idr_base;
struct	idr_zap
  {
	unsigned short	nbr;
  };
struct	idr_linker
  {
	char linkage_editor[10];
	unsigned char level;
	unsigned char version;
	unsigned char yyddd_packed[3];
  };
struct	idr_xlater
  {
	char name[10];
	unsigned char version;
	unsigned char level;
	unsigned char yyddd_packed[3];
  };
#define IDR_BUFFER_SIZE 12800
static	char	*idr_rec;
static	unsigned char	idr_rec_subtype;
static	long	idr_rec_length;

struct	ctl_base_entry
  {
	unsigned char   block_type;
	unsigned char   unknown1;
	unsigned short	rld_ctl_cnt;
	unsigned short	ctl_size;
	unsigned short  rld_size;
	double  ccw;
  };

static	struct	ctl_base_entry *ctl_base;
struct	ctl_entry
  {
	unsigned short cesd_nbr;
	unsigned short cesd_length;
  };


struct rld_base_entry
  {
	unsigned char   block_type;
	unsigned char   unknown1;
	short	unknown2;    
	short	unknown3;
	unsigned short	rld_size;
	short   unknown4;
	short   unknown5;
	short   unknown6;
	short   unknown7;
  };
static	struct	rld_base_entry *rld_base;
struct	rld_entry
  {
	unsigned short r_ptr;
	unsigned short p_ptr;
  };
struct	rld_fl_entry
  {
	unsigned char	fl;
	unsigned char  addr[3];
  };


struct	cesd_type_entry
  {
	unsigned char type;
	char	*tag_name;
	char    *description;
  };

static	struct cesd_type_entry cesd_type[] = {
  { CESD_TYPE_SD      , "SD  ", "Control Section Definition" },
  { CESD_TYPE_ER      , "ER  ", "External Reference        " },
  { CESD_TYPE_LR      , "LR  ", "Label Reference ??        " },
  { CESD_TYPE_PR      , "PR  ", "Pseudo-Register           " },
  { CESD_TYPE_WX      , "WX  ", "Weak External             " },
  { CESD_TYPE_CM      , "CM  ", "Common Area               " },
  { CESD_TYPE_LD      , "LD  ", "Label Definition/Entry Pts" },
  { CESD_TYPE_NULL    , "NULL", "Null                      " },
  };
static	long	nbr_esd_types = sizeof(cesd_type)/sizeof(cesd_type[0]);
static	long	total_nbr_cesd_entries;

void a100_init (int argc, char **argv);
int a500_analyze_load_module (void);
void a400_process_all_load_modules (void);
void a200_final (void);
extern char *strncpy (char *, const char *, size_t);
extern int strncmp (const char *, const char *, size_t);
extern int isxdigit (int);
extern pid_t getpid (void);
extern int unlink (const char *);
extern int fprintf (FILE *, const char *, ...);
extern char *strcat (char *, const char *);
extern int fclose (FILE *);
extern int system (const char *);
extern int sscanf (const char *, const char *, ...);
extern int close (int);
extern int fflush (FILE *);
extern void *calloc (size_t, size_t);
extern void free (void *);
extern void *memalign (size_t, size_t);
extern int read (int, void *, unsigned int);
extern char *getenv (const char *);
extern unsigned int sleep (unsigned int);

void s100_open_db_connection (void);
void printmsg (int ctl, char *vals,...);
void s900_close_db_connection (void);
void a300_produce_reports (void);
void a310_unknown_report (char *buf);
void a320_pli_report (char *buf);
void a330_csect_inconsistency_report (char *buf);
void a340_csect_xref_report (char *buf);
void b202_delete_xrefs (void);
void b300_process_idr (void);
char *a311_product_type (char *product_name);
void build_xlate_table (void);
int read_record (char *input_buf);
int b316_get_esd_entry (long int nbr);
int char3addr_to_long (char *addr);
void b100_process_control (void);
void b200_process_cesd (void);
void b302_table_idr_record (void);
void b500_process_ctlrld (void);
void b400_process_rld (void);
void b317_free_esd_entries (void);
void ebcdic_to_ascii (register unsigned char *c, long int nbr);
void b201_process_section_definition (struct esd_entry *cur_esd);
void b305_process_idr_zap (void);
void b310_process_idr_linker (void);
void b315_process_idr_xlater (void);
void b320_process_idr_user (void);
void packed_char3_to_mmddyy (char *date_data, short int *mm, short int *dd, short int *yy);
void b318_check_for_plip_mod (struct esd_entry *cur_esd);
int packed_short_to_binary (short int from);
int internaldate (short int *month, short int *day, short int *year);
int my_dysize(short year);

/*************************************************************************/
/*                                                                       */
/*  Process our arguments, then analyze the load module.                 */
/*                                                                       */
/*************************************************************************/

int
main(int argc, char **argv)
{
	a100_init(argc,argv);

	if (*filename != '\0')
	  a500_analyze_load_module();
    else
      a400_process_all_load_modules();

	a200_final();

	exit(nbr_error_messages);
}
/*********************************************************************/
/*                                                                   */
/*  Scan arguments and perform other initialization.                 */
/*                                                                   */
/*********************************************************************/
void a100_init(int argc, char **argv)
{
    char *c;
	struct	timeval	cur_time;
	struct	tm	*local_time;
	char	cur_date[12];

	e001_install_pgm_id(ANALYZE_LOAD_MODULES);

	gettimeofday(&cur_time);
	local_time = localtime(&cur_time.tv_sec);
	sprintf(cur_date,"%02d/%02d/%02d",
	            local_time->tm_mon+1,
                local_time->tm_mday,
                local_time->tm_year);

	strncpy(rpt1_hdr1+2,cur_date,8);
	strncpy(rpt2_hdr1+2,cur_date,8);
	strncpy(rpt3_hdr1+2,cur_date,8);
	strncpy(rpt4_hdr1+2,cur_date,8);

    build_xlate_table();
    argc--; argv++;
    while (argc--)
      {
		if (strcmp(*argv,"-nodb") == 0)
		  no_database = TRUE;
		else
		if (strcmp(*argv,"-noupdt") == 0)
		  update = FALSE;
		else
		if (strcmp(*argv,"-list") == 0)
		  debug |= (MSG_LIST | MSG_LISTRC);
		else
		if ((strcmp(*argv,"-f") == 0) && argc)
		  {
			argc--; argv++;
	        strcpy(filename,*argv);
			c = (char *)strrchr(filename,'/');
			if (c)
			  strcpy(member_name,c+1);
			else
			  strcpy(member_name,filename);
		  }
		else
		if ((strcmp(*argv,"-r") == 0) && argc)
		  {
			argc--; argv++;
	        strcpy(report_filename,*argv);
		  }
		else
        if ((strcmp(*argv,"-d") == 0) && argc)
          {
            argc--;
            argv++;
            if (strncmp(*argv,"0x",2) == 0)
              {
                c = (char *)((*argv)+2);
                while (isxdigit(*c) && isxdigit(*(c+1)))
                  {
                    debug += xlate[(int)(*c++)] * 16;
                    debug += xlate[(int)(*c++)];
                  }
              }    
          }    
        argv++;
      }
	if (no_database && (strlen(filename) == 0))
	  {
		if(v100_errmsg(178,
			"analyze_load_modules [-f filename] [-list] [-d 0xNNNN] [-r rptname]") == ESFAT)
		exit(ESFAT);
	  }

    if (!no_database)
      s100_open_db_connection();

	sprintf(report_extract_filename,"/tmp/load_module_extract.%d",getpid());

    unlink(report_extract_filename);

	report_file = fopen(report_extract_filename,"w");
	if (report_file == NULL)
	  {
	  if(v100_errmsg(1, report_extract_filename, "writing") == ESFAT)
		exit(ESFAT);
	  }
	else
	  {
		fprintf(report_file,"01\n");
		fprintf(report_file,"02\n");
		fprintf(report_file,"03\n");
		fprintf(report_file,"04\n");
	  }

    if (debug != 0)
      printmsg(MSG_ALWAYS,"Debug level 0x%x.",debug);
}
/*************************************************************************/
/*                                                                       */
/*   Perform any clean up operations.                                    */
/*                                                                       */
/*************************************************************************/
void a200_final(void)
{
    if (!no_database)
      s900_close_db_connection();

	if (report_file)
	  {
		fclose(report_file);
		a300_produce_reports();
	  }

    if(v100_errmsg(234,nbr_warn_messages,nbr_warn_messages==1?"":"s") == ESFAT)
	exit(ESFAT);
    if(v100_errmsg(229,nbr_error_messages,nbr_error_messages==1?"":"s") == ESFAT)
		exit(ESFAT);
}
/*************************************************************************/
/*                                                                       */
/*  Produce all the final reports.                                       */
/*                                                                       */
/*************************************************************************/
void a300_produce_reports(void)
{
	char	buf[4096];
	char	report_file_sorted[512];
	FILE	*report_extracts;
	long	nbr_rpt1_records = 0;
	long	nbr_rpt2_records = 0;
	long	nbr_rpt3_records = 0;
	long	nbr_rpt4_records = 0;
	long	report_code;

	sprintf(report_file_sorted,"%s.sorted",report_extract_filename);

	sprintf(buf,"sort < %s | uniq > %s",
		report_extract_filename, report_file_sorted);

	if(v100_errmsg(235) == ESFAT)
		exit(ESFAT);

	system(buf);

    unlink(report_extract_filename);

	report_extracts = fopen(report_file_sorted,"r");
	if (report_extracts == 0)
	  {
		if(v100_errmsg(1, report_file_sorted, "reading") == ESFAT)
			exit(ESFAT);
		return;
	  }

	if(v100_errmsg(236) == ESFAT)
		exit(ESFAT);

	report_file = fopen(report_filename,"w");
	if (report_file == NULL)
	  report_file = stdout;
	while (fgets(buf,sizeof(buf),report_extracts) != NULL)
	  {
		sscanf(buf,"%d",&report_code);
		switch (report_code)
		  {
			case 1:
			  nbr_rpt1_records++;
			  a310_unknown_report(buf);
			  break;
			case 2:
			  if (nbr_rpt2_records == 0)
				{
		          fprintf(report_file,"\n");
				}
			  nbr_rpt2_records++;
			  a320_pli_report(buf);
			  break;
			case 3:
			  if (nbr_rpt3_records == 0)
				{
		          fprintf(report_file,"\n");
				}
			  nbr_rpt3_records++;
			  a330_csect_inconsistency_report(buf);
			  break;
			case 4:
			  if (nbr_rpt4_records == 0)
				{
		          fprintf(report_file,"\n");
				}
			  nbr_rpt4_records++;
			  a340_csect_xref_report(buf);
			  break;
		  }
	  }

   fprintf(report_file,"\n");

   unlink(report_file_sorted);

   if (report_file != stdout)
	 fclose(report_file);
}
/*************************************************************************/
/*                                                                       */
/*  Produce the csects without source file report.                       */
/*                                                                       */
/*************************************************************************/
void a310_unknown_report(char *buf)
{
static	char	hold_csect_name[12];
static	char	hold_product_name[12];
static	char	hold_bin_name[12];
static	long	max_lines = 58;
static	long	nbr_lines = 59;
static	long	nbr_modules_across;
static	char	csect_name[12];
	char	bin_name[12];
	char	product_name[12];
	char	product_ver[12];
	char	build_date[12];

	sscanf(buf,"%*d %s %s %s %s %s",
		csect_name, bin_name, product_name, product_ver, build_date);

	if (nbr_lines >= max_lines)
	  {
		fprintf(report_file,rpt1_hdr1,++rpt1_page_nbr);
		fprintf(report_file,"\n\n\n");
		nbr_lines = 5;
	  }

	if (strlen(csect_name) == 0)
	  return;

	if (strcmp(hold_csect_name,csect_name) != 0)
	  {
		if ((nbr_lines > 5) || (nbr_modules_across > 0))
		  {
		    fprintf(report_file,"\n\n\n");
		    nbr_lines+=3;
		  }
		nbr_modules_across = 0;
		strcpy(hold_csect_name,csect_name);
		strcpy(hold_product_name,product_name);
		*hold_bin_name = '\0';
		fprintf(report_file,"Csect: %-10.10s    Type: %-32.32s\n",
		 		hold_csect_name, a311_product_type(hold_product_name));
		nbr_lines+=1;
	    if (nbr_lines >= max_lines-1)
	      {
		    fprintf(report_file,rpt1_hdr1,++rpt1_page_nbr);
		    fprintf(report_file,"\n\n\n");
		    nbr_lines = 5;
	      }
		fprintf(report_file,"                     Load Modules: %-8.8s ",bin_name);
		nbr_modules_across++;
	  }
	else
	  {
		if (strcmp(hold_bin_name,bin_name) != 0)
		  {
		    if (nbr_modules_across >= 5)
		      {
			    fprintf(report_file,"\n");
			    nbr_lines++;
	            if (nbr_lines >= max_lines-1)
	              {
		            fprintf(report_file,rpt1_hdr1,++rpt1_page_nbr);
		            fprintf(report_file,"\n\n\n");
		            nbr_lines = 5;
	              }
			    fprintf(report_file,"                                   ");
				nbr_modules_across = 0;
		      }
		    fprintf(report_file,"%-8.8s ",bin_name);
		    nbr_modules_across++;
		  }
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Translate a product code to a product description.                   */
/*                                                                       */
/*************************************************************************/
char *a311_product_type(char *product_name)
{
	long	i;

	for (i = 0; i < nbr_product_names; i++)
	  {
		if (strncmp(product_name,product[i].name,strlen(product[i].name)) == 0)
		  return((char *)product[i].type);
	  }

    return((char *)product_name);

}
/*************************************************************************/
/*                                                                       */
/*  Produce the PLI module report.                                       */
/*                                                                       */
/*************************************************************************/
void a320_pli_report(char *buf)
{
static	char	bin_name[12];
static	long	max_lines = 58;
static	long	nbr_lines = 59;
static	long	nbr_modules_across;

	sscanf(buf,"%*d %s",bin_name);
	
	if (nbr_lines >= max_lines)
	  {
		fprintf(report_file,rpt2_hdr1,++rpt2_page_nbr);
		fprintf(report_file,"\n\n\n");
		nbr_lines = 5;
	  }

	if (strlen(bin_name) == 0)
	  return;

	if (nbr_modules_across >= 7)
	  {
		fprintf(report_file,"\n");
		nbr_lines++;
		nbr_modules_across = 0;
	  }
	fprintf(report_file,"%-10.10s ",bin_name);
	nbr_modules_across++;
}
/*************************************************************************/
/*                                                                       */
/*  Produce the Csect inconsistency report.                              */
/*                                                                       */
/*************************************************************************/
void a330_csect_inconsistency_report(char *buf)
{
static	char	hold_csect_name[12];
static	char	hold_bin_name[12];
static	char	hold_build_date[12];
static	char	hold_product_name[12];
static	char	hold_product_ver[12];
static	long	max_lines = 58;
static	long	nbr_lines = 59;
static	char	csect_name[12];
	char	bin_name[12];
	char	product_name[12];
	char	product_ver[12];
	char	build_date[12];
static	long	first_printed = FALSE;

	sscanf(buf,"%*d %s %s %s %s %s",
		csect_name, build_date, product_name, product_ver, bin_name);

	if (nbr_lines >= max_lines)
	  {
		fprintf(report_file,rpt3_hdr1,++rpt3_page_nbr);
		fprintf(report_file,"\n\n\n");
		nbr_lines = 5;
	  }

	if (strlen(csect_name) == 0)
	  return;

	if (strcmp(csect_name,hold_csect_name) == 0)
	  {
		if ((strcmp(hold_build_date,build_date) != 0) ||
		    (strcmp(hold_product_name,product_name) != 0) ||
		    (strcmp(hold_product_ver,product_ver) != 0))
		  {
			if (first_printed == FALSE)
			  {
			    fprintf(report_file,"Csect: %-8.8s in module %-8.8s built %-8.8s by product: %-10.10s %-5.5s\n",
					hold_csect_name, hold_bin_name, hold_build_date,
					hold_product_name, hold_product_ver);
			    first_printed = TRUE;
				nbr_lines++;
			  }
			fprintf(report_file,"Csect: %-8.8s in module %-8.8s built %-8.8s by product: %-10.10s %-5.5s\n",
					csect_name, bin_name, build_date,
					product_name, product_ver);
			nbr_lines++;
		    strcpy(hold_csect_name,csect_name);
		    strcpy(hold_build_date,build_date);
		    strcpy(hold_product_name,product_name);
		    strcpy(hold_product_ver,product_ver);
		    strcpy(hold_bin_name,bin_name);
		  }
	  }
	else
	  {
		if (first_printed && (nbr_lines != 5))
		  {
		    fprintf(report_file,"\n\n");
		    nbr_lines+=2;
		  }
	    first_printed = FALSE;
		strcpy(hold_csect_name,csect_name);
		strcpy(hold_build_date,build_date);
		strcpy(hold_product_name,product_name);
		strcpy(hold_product_ver,product_ver);
		strcpy(hold_bin_name,bin_name);
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Produce the Csect inconsistency report.                              */
/*                                                                       */
/*************************************************************************/
void a340_csect_xref_report(char *buf)
{
static	char	hold_csect_name[12];
static	long	max_lines = 58;
static	long	nbr_lines = 59;
static	long	nbr_modules_across;
static	char	csect_name[12];
	char	bin_name[12];

	sscanf(buf,"%*d %s %s",
		csect_name, bin_name);

	if (nbr_lines >= max_lines)
	  {
		fprintf(report_file,rpt4_hdr1,++rpt4_page_nbr);
		fprintf(report_file,rpt4_hdr2);
		fprintf(report_file,"\n");
		nbr_lines = 5;
	  }

	if (strlen(csect_name) == 0)
	  return;

	if (strcmp(csect_name,hold_csect_name) == 0)
	  {
	    if (nbr_modules_across >= 7)
	      {
		    fprintf(report_file,"\n");
		    nbr_lines++;
		    nbr_modules_across = 0;
	        if (nbr_lines >= max_lines)
	          {
		        fprintf(report_file,rpt4_hdr1,++rpt4_page_nbr);
		        fprintf(report_file,rpt4_hdr2);
		        fprintf(report_file,"\n");
		        nbr_lines = 5;
		      }
			fprintf(report_file,"           ");
	      }
		fprintf(report_file,"%-8.8s ",bin_name);
		nbr_modules_across++;
	  }
	else
	  {
		if (nbr_modules_across > 0)
		  {
			fprintf(report_file,"\n\n");
			nbr_lines += 2;
		  }
	    if (nbr_lines >= max_lines)
	      {
		    fprintf(report_file,rpt4_hdr1,++rpt4_page_nbr);
		    fprintf(report_file,rpt4_hdr2);
		    fprintf(report_file,"\n");
		    nbr_lines = 5;
		  }
		fprintf(report_file,"%-8.8s   %-8.8s ",csect_name,bin_name);
		nbr_modules_across = 1;
		strcpy(hold_csect_name,csect_name);
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Process all load modules.                                            */
/*                                                                       */
/*************************************************************************/
void a400_process_all_load_modules(void)
{
    char    inout_filename[512];
    char    srcname[512];
    long rc;
    char    buf[4000];
    long    next_row;
    FILE    *inout;
	char	*c;

    sprintf(inout_filename,"%s.%d","/tmp/analyze_load_modules",getpid());
    inout = fopen(inout_filename,"w");
    if (inout == 0)
      {
        if(v100_errmsg(179,inout_filename, "writing") == ESFAT)
        	exit(ESFAT);
      } 
 
	if (no_database)
	  {
		fclose(inout);
	    return;
	  }

    sprintf(buf,"%s %s %s %s",
        "select exec_bin_key, unix_dir, srcfile_name",
        "from source_file, exec_bin",
        "where srcfile_type = 'LOAD'",
        "  and srcfile_name = exec_bin_name");
    dbcancel(dbproc);
    dbfreebuf(dbproc);
    dbcmd(dbproc,buf);
    rc = dbsqlexec(dbproc);
    if (rc == FAIL)
      {
        if(v100_errmsg(10,"Read of LOAD source files.") == ESFAT)
			exit(ESFAT);
        goto end_routine;
      }
    while ((rc = dbresults(dbproc)) != NO_MORE_RESULTS)
      {
        if (dbnumcols(dbproc) > 0)
          {
            dbbind(dbproc, 1, INTBIND,       4,                &exec_bin_key);
            dbbind(dbproc, 2, NTBSTRINGBIND, sizeof(filename), filename);
            dbbind(dbproc, 3, NTBSTRINGBIND, sizeof(srcname), srcname);
          }
        while ((next_row = dbnextrow(dbproc)) != NO_MORE_ROWS)
          {
             strcat(filename,"/");
             strcat(filename,srcname);
             fprintf(inout,"%d %s\n",exec_bin_key,filename);
          }
      }    
    fclose(inout);

    inout = fopen(inout_filename,"r");
    if (inout == 0)
      {
        if(v100_errmsg(179,inout_filename,"reading") == ESFAT)
        	exit(ESFAT);
      }
    while (fgets(buf,sizeof(buf),inout) != NULL)
      {
        sscanf(buf,"%d %s",&exec_bin_key,filename);
		c = (char *)strrchr(filename,'/');
		if (c)
		  strcpy(member_name,c+1);
		else
		  strcpy(member_name,filename);
        b202_delete_xrefs();
	    a500_analyze_load_module();
      }
    fclose(inout);
    unlink(inout_filename);
end_routine:;
}
/*************************************************************************/
/*                                                                       */
/*  Analyze the load module, find all the section definitions, and       */
/*  create a compile_unix xref record for all matching pgm_id cards.     */
/*                                                                       */
/*************************************************************************/
int a500_analyze_load_module(void)
{
    long    nbr_records = 0;
	struct	esd_entry	*cur_esd;
	int idr_valid, esd_valid;

    /***********************************************************/
    /* Make the parms global.                                  */
    /***********************************************************/
    len_added = TRUE;
    input = 0;
    inputfd = 0;
	if(v100_errmsg(222,member_name) == ESFAT)
		exit(ESFAT);

    pli_load_module = FALSE;

    /***********************************************************/
    /* If the half-word length was added, we need to use open  */
    /* otherwise, we can use the much easier fopen.            */
    /***********************************************************/
    if (len_added)
      {
        inputfd = open(filename,O_RDONLY,0777);
        if (inputfd < 0)
          {
          if(v100_errmsg(1, filename, "reading") == ESFAT)
		exit(ESFAT);
          goto error_return;
          }
      }
    else
      {
        input = fopen(filename,"r");
        if (input == NULL)
          {
          if(v100_errmsg(1, filename, "reading") == ESFAT)
		exit(ESFAT);
          goto error_return;
          }
      }

	cesd_base = (struct cesd_base_entry *)input_rec;
	total_nbr_cesd_entries = 0;

    /***********************************************************/
    /* Read and process every record.                          */
    /***********************************************************/
	esd_valid = TRUE;
	idr_valid = TRUE;
    while ((rec_len = read_record((char *)input_rec)) != 0)
      {
        nbr_records++;
		if ((cesd_base->block_type != BLOCK_IDR) &&
			(idr_rec != 0))
		  b300_process_idr();
		switch (cesd_base->block_type)
	  		{
			case BLOCK_CONTROL:
				esd_valid = FALSE;
				idr_valid = FALSE;
		  		b100_process_control();
		  		break;
			case BLOCK_CESD:
				if(esd_valid)
		  			b200_process_cesd();
				else
          			printmsg(MSG_LIST,"Text Block type of %01X",cesd_base->block_type);
		  		break;
			case BLOCK_IDR:
				if(idr_valid)
					{
					esd_valid = FALSE;
		  			b302_table_idr_record();
					}
				else
          			printmsg(MSG_LIST,"Text Block type of %01X",cesd_base->block_type);
		  		break;
           	case BLOCK_CTLRLD:
				esd_valid = FALSE;
				idr_valid = FALSE;
		  		b500_process_ctlrld();
             		break;
			case BLOCK_RLD:
			case BLOCK_RLD1:
				esd_valid = FALSE;
				idr_valid = FALSE;
		  		b400_process_rld();
		  		break;
			default:
          		printmsg(MSG_LIST,"Text Block type of %01X",cesd_base->block_type);
		  	break;
	  		}
      }

    /***********************************************************/
    /* Since we table the idr records, make sure we have       */
    /* processed the last one.                                 */
    /***********************************************************/
	if (idr_rec != 0)
	  b300_process_idr();

    /***********************************************************/
    /* Build the report extract of unknown source csects       */
    /* and other types of programs.                            */
    /***********************************************************/
	if (pli_load_module)
	  {
		if (report_file) 
		  {
	        fprintf(report_file,"02 %-10.10s\n",member_name);
			fflush(report_file);
			FOR_EACH_ESD_NODE(cur_esd)
			  if (*cur_esd->name)
	            fprintf(report_file,"04 %-10.10s %-10.10s\n",
		          cur_esd->name, member_name);
		  }
	  }
	else
	FOR_EACH_ESD_NODE(cur_esd)
	  {
		if (*cur_esd->name)
	      fprintf(report_file,"04 %-10.10s %-10.10s\n",
		        cur_esd->name, member_name);
		if (cur_esd->unknown_source)
		  {
			if (report_file)
			  {
				if (*cur_esd->name)
				  {
			       fprintf(report_file,"01 %-10.10s %-10.10s %-10.10s %-10.10s %-10.10s\n",
			        cur_esd->name, member_name, 
					*cur_esd->product_name ? cur_esd->product_name : ".",
			        *cur_esd->product_ver ? cur_esd->product_ver : ".",
					*cur_esd->build_date_mmddyy ? cur_esd->build_date_mmddyy : ".");
				   fprintf(report_file,"03 %-10.10s %-10.10s %-10.10s %-10.10s %-10.10s\n",
					cur_esd->name,
					*cur_esd->build_date_yymmdd ? cur_esd->build_date_yymmdd : ".",
					*cur_esd->product_name ? cur_esd->product_name : ".",
			        *cur_esd->product_ver ? cur_esd->product_ver : ".",
					member_name);
			       fflush(report_file);
				  }
			  }
		  }
	  }
    /***********************************************************/
    /* Free our esd linked list.                               */
    /***********************************************************/
	b317_free_esd_entries();

    /***********************************************************/
    /* Close the input file.                                   */
    /***********************************************************/
    if (len_added)
      close(inputfd);
    else
      fclose(input);
    return(0);

error_return:
    /***********************************************************/
    /* If we had a problem, make sure we close open files.     */
    /***********************************************************/
    if (len_added && (inputfd != 0))
      close(inputfd);
    if (!len_added && (input != 0))
      fclose(input);
    return(1);
}
/*************************************************************************/
/*                                                                       */
/*  Process a control block.                                             */
/*     This control block contains CESD entry numbers and the length     */
/*     of each executable CESD.                                          */
/*                                                                       */
/*************************************************************************/
void b100_process_control(void)
{
	long	nbr_ctl_entries;
	struct	ctl_entry	*ctl;
	long	i;
	struct esd_entry *cur_esd;

	ctl_base = (struct ctl_base_entry *)input_rec;
	ctl = (struct  ctl_entry *)((char *)input_rec + sizeof(struct ctl_base_entry) );

	nbr_ctl_entries = ctl_base->ctl_size / sizeof(struct ctl_entry);

	printmsg(MSG_LISTRC,"Control Record Found - Size %03d - RLD/CTL Cnt %d - CCW %04X %04X",
			ctl_base->ctl_size, ctl_base->rld_ctl_cnt, ctl_base->ccw,ctl_base->ccw+4);

    /***********************************************************/
    /* Loop through all the control entries.                   */
    /***********************************************************/
	for (i = 0; i < nbr_ctl_entries; i++,ctl++)
	  {
		cur_esd = (struct esd_entry *)b316_get_esd_entry(ctl->cesd_nbr);
		if (cur_esd == 0)
		  printmsg(MSG_LIST  ,"     CESD#: %d  Length: %06X",
				ctl->cesd_nbr, ctl->cesd_length);
		else
		  printmsg(MSG_LIST  ,"      %-8.8s  Length: %06X",
				cur_esd->name, ctl->cesd_length);
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Process a cesd block.                                                */
/*     This control block contains CESD entries.  They are numbers       */
/*     sequentially as they are encountered because other control        */
/*     blocks refer to them by this sequential number.                   */
/*                                                                       */
/*************************************************************************/
void b200_process_cesd(void)
{
	struct	cesd_entry	*cesd = (struct cesd_entry *)input_rec;
	long	nbr_cesd_entries = cesd_base->esd_size / sizeof(struct cesd_entry);
	char    esd_name[10];
	long	i,j;
	struct	esd_entry	*cur_esd;



	printmsg(MSG_LISTRC,"CESD Record Found - ESD Id %03d  Length %03d",
				cesd_base->esd_id, cesd_base->esd_size);
				
	/* (char *)cesd += sizeof(struct cesd_base_entry); */

	cesd = (struct  cesd_entry *)((char *)cesd + sizeof(struct cesd_base_entry));

    /***********************************************************/
    /* Loop through all the cesd entries and build our         */
    /* linked list.                                            */
    /***********************************************************/
	/* if (strcmp(member_name,"PGM42040") == 0)
	  i = i; */
	for (i = 0;  i < nbr_cesd_entries; i++,cesd++)
	  {
		total_nbr_cesd_entries++;
        ebcdic_to_ascii((unsigned char *)cesd->name,sizeof(cesd->name));
		copynull(esd_name,cesd->name,sizeof(esd_name),sizeof(cesd->name));
        cur_esd = (struct esd_entry *)calloc(1,sizeof(struct esd_entry));
        if (cur_esd != 0)
		  {
			ADD_TO_ESD_LIST(cur_esd);
            strcpy(cur_esd->name,esd_name);
			cur_esd->nbr = total_nbr_cesd_entries;
		  }
		cesd->type &= 0x7f;  /* Turn off high order bit */
		for (j = 0; j < nbr_esd_types; j++)
		  {
			if (cesd_type[j].type == cesd->type)
			  break;
		  }
		if (j < nbr_esd_types)
		  {
            if ((cesd->type == CESD_TYPE_SD) &&
				(*cur_esd->name))
              b201_process_section_definition(cur_esd);
	        printmsg(MSG_LIST,"ESD %3d Name: %-8.8s Type: %s (%s) Length: %06X",
					total_nbr_cesd_entries,
					esd_name,cesd_type[j].tag_name,
					cesd_type[j].description, char3addr_to_long((char *)cesd->id_length));
		  }
		else
	      printmsg(MSG_LIST  ,"ESD %3d Name: %-8.8s Type: %x",
					total_nbr_cesd_entries,esd_name,cesd->type);
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Try to add a exec_bin to compile_unit xref.                          */
/*                                                                       */
/*************************************************************************/
void b201_process_section_definition(struct esd_entry *cur_esd)
{
    long rc;
    char    buf[4000];
    long    next_row;
	long	xref_cnt = 0;
	long	nbr_cols;

	if (no_database || update == FALSE)
	  goto produce_reports;

    sprintf(buf,"exec create_cu_exec_bin_csect_xref %d,'%s'",
        exec_bin_key,
        cur_esd->name);

    dbcancel(dbproc);
    dbfreebuf(dbproc);
    dbcmd(dbproc,buf);
    rc = dbsqlexec(dbproc);
    if (rc == FAIL)
      {
        if(v100_errmsg(10,"b201_process_section_definition") == ESFAT)
		exit(ESFAT);
        goto end_routine;
      }

    while ((rc = dbresults(dbproc)) != NO_MORE_RESULTS)
      {
		nbr_cols = dbnumcols(dbproc);
		if (nbr_cols > 0)
		  dbbind(dbproc, 1, INTBIND,  4,  &xref_cnt);
        while ((next_row = dbnextrow(dbproc)) != NO_MORE_ROWS)
          {
          }
      }    

	if (xref_cnt > 0)
	  printmsg(MSG_CSECTS,"Csect %-8.8s: cross-referenced.",cur_esd->name);
	else
	  printmsg(MSG_CSECTS,"Csect %-8.8s: not matched with a program.",
			cur_esd->name);

produce_reports:;

	if (strcmp(cur_esd->name,"PLISTART") == 0)
	  {
	    printmsg(MSG_CSECTS,"Probable PLI module linked into load module %s.",member_name);
		pli_load_module = TRUE;
	  }

	if ((strncmp(cur_esd->name,"ILB",3) == 0) ||
	    (strncmp(cur_esd->name,"DFH",3) == 0) ||
	    (strncmp(cur_esd->name,"IBM",3) == 0) ||
	    (strncmp(cur_esd->name,"DFS",3) == 0))
	  goto end_routine;

    cur_esd->pli_load_module = pli_load_module;

	if (xref_cnt == 0)
	  {
		cur_esd->unknown_source = TRUE;
	    if (pli_load_module == FALSE)
	      {
	        printmsg(MSG_CSECTS,"Probable missing source for control section %-8.8s.",
		        cur_esd->name,member_name);
	      }
	  }

end_routine:;

	return;
}
/*************************************************************************/
/*                                                                       */
/*  Delete xrefs for this load modules                                   */
/*                                                                       */
/*************************************************************************/
void b202_delete_xrefs(void)
{
    long rc;
    char    buf[4000];
    long    next_row;
	long	xref_cnt = 0;
	long	nbr_cols;

	if (no_database || update == FALSE)
	  return;

    sprintf(buf,"exec delete_cu_exec_bin_csect_xref %d",
        exec_bin_key);

    dbcancel(dbproc);
    dbfreebuf(dbproc);
    dbcmd(dbproc,buf);
    rc = dbsqlexec(dbproc);
    if (rc == FAIL)
      {
        if(v100_errmsg(10,"b202_delete_xrefs") == ESFAT)
		exit(ESFAT);
        goto end_routine;
      }

    while ((rc = dbresults(dbproc)) != NO_MORE_RESULTS)
      {
		nbr_cols = dbnumcols(dbproc);
		if (nbr_cols > 0)
		  dbbind(dbproc, 1, INTBIND,  4,  &xref_cnt);
        while ((next_row = dbnextrow(dbproc)) != NO_MORE_ROWS)
          {
          }
      }    
end_routine:;
}
/*************************************************************************/
/*                                                                       */
/*  Determine what type of IDR record exists in the buffer, and          */
/*  process it.                                                          */
/*                                                                       */
/*************************************************************************/
void b300_process_idr(void)
{
	idr_rec_subtype &= 0x7f;  /* Zap high order bit */
	switch (idr_rec_subtype)
	  {
		case IDR_TYPE_ZAP:
		  b305_process_idr_zap();
		  break;
		case IDR_TYPE_LINKER:
		  b310_process_idr_linker();
		  break;
		case IDR_TYPE_XLATER:
		  b315_process_idr_xlater();
		  break;
		case IDR_TYPE_USER:
		  b320_process_idr_user();
		  break;
	  }

    /***********************************************************/
    /* When we are done, free the IDR buffer.                  */
    /***********************************************************/
	if (idr_rec)
	  free(idr_rec);
	idr_rec = 0;
	idr_rec_length = 0;
}
/*************************************************************************/
/*                                                                       */
/*  Since each individual IDR record does not stand alone, we must       */
/*  table them all up, the process them.                                 */
/*                                                                       */
/*  We need to process the idr batch whenever the IDR subtype field      */
/*  changes, or when the subtype field has the high-order bit on.        */
/*                                                                       */
/*************************************************************************/
void b302_table_idr_record(void)
{
	char	*idr_start;
	struct	idr_base_entry *idr_table_base;

    /***********************************************************/
    /* Setup to point to the new record.                       */
    /***********************************************************/
	idr_base = (struct idr_base_entry *)input_rec;
	idr_base->idr_length -= 2;
	idr_start = (char *)idr_base + sizeof(struct idr_base_entry);

    /***********************************************************/
    /* See if we have an IDR break and need to process the     */
    /* current batch.                                          */
    /***********************************************************/
	if (idr_rec != 0)
	  {
		idr_table_base = (struct idr_base_entry *)idr_rec;
		if ((idr_rec_subtype&0x7f) != (idr_base->idr_subtype&0x7f))
		  b300_process_idr();
	  }

	printmsg(MSG_LISTRC,"IDR Record Found - Subtype %03d Length %03d",
			idr_base->idr_subtype, idr_base->idr_length);

    /***********************************************************/
    /* Allocate space for a bunch of IDR records.              */
    /***********************************************************/
	if (idr_rec == 0)
	  {
	    idr_rec = (char *)memalign(8,IDR_BUFFER_SIZE);
	    idr_rec_length = 0;
	  }

    /***********************************************************/
    /* Copy the idr record into our buffer.                    */
    /***********************************************************/
	if (idr_rec != 0)
	  {
		idr_rec_subtype = idr_base->idr_subtype;
	    if ((int)(IDR_BUFFER_SIZE - idr_rec_length) >= (int)(idr_base->idr_length))
	      {
		    memcpy(idr_rec+idr_rec_length,idr_start,idr_base->idr_length);
		    idr_rec_length += idr_base->idr_length;
	      }
		else
		  printmsg(MSG_LIST  ,"Throwing away some IDR data;");
	  }

    /***********************************************************/
    /* If the high order bit is on in the subtype field,       */
    /* we need to process this IDR batch.                      */
    /***********************************************************/
	if ((idr_rec_subtype&0x80) != 0)
	  b300_process_idr();
}
/*************************************************************************/
/*                                                                       */
/*  Super Zap also includes IDR records.  I don't have an example of     */
/*  any so I don't know how to format them.  I'm just guesing that       */
/*  somewhere in the record will be a count field that indicates         */
/*  whether anything is there.                                           */
/*                                                                       */
/*************************************************************************/
void b305_process_idr_zap(void)
{
	struct	idr_zap *idr;

	idr = (struct  idr_zap *)idr_rec;

	if (idr->nbr == 0)
	  printmsg(MSG_LIST  ,"No information supplied by IMASPZAP");
	else
	  printmsg(MSG_LIST  ,"**** Information from IMASPZAP exists.");

}
/*************************************************************************/
/*                                                                       */
/*  The linker places an IDR record into the load module.  Print the     */
/*  contents of that record.                                             */
/*                                                                       */
/*************************************************************************/
void b310_process_idr_linker(void)
{
	struct	idr_linker *idr;
	short	mm,dd,yy;

	idr = (struct idr_linker *)idr_rec;  
	ebcdic_to_ascii((unsigned char *)idr->linkage_editor,sizeof(idr->linkage_editor));
    packed_char3_to_mmddyy((char *)idr->yyddd_packed,&mm,&dd,&yy);
	printmsg(MSG_LIST  ,"Produced by Linkage Editor %-10.10s level %d.%d on %02d/%02d/%02d",
		idr->linkage_editor, idr->level, idr->version,
		mm,dd,yy);
}
/*************************************************************************/
/*                                                                       */
/*  Certain IDR records are place in the load module by compilers and    */
/*  assemblers.  Extract this information and print it.                  */
/*                                                                       */
/*  We have to jump through a few hoops here to insure that we don't     */
/*  violate the sun4 boundary restrictions for shorts.                   */
/*                                                                       */
/*************************************************************************/
void b315_process_idr_xlater(void)
{
	unsigned char	*cnt,*last_cnt;
	unsigned char	*cnt_start;
	struct	idr_xlater	*idr;
	struct	idr_xlater	*next_idr;
	struct	idr_xlater	*last_idr;
	long	nbr_cesd_entries;
	short	mm,dd,yy;
	struct	esd_entry	*cur_esd;
	unsigned char *idr_cnt;
	long	i;
	short	s_cnt;
	unsigned char	*hs = (unsigned char *)&s_cnt;

	cnt = (unsigned char *)idr_rec;  
	cnt_start = cnt;

    /***********************************************************/
    /* Process all the record.  We first have a variable       */
    /* number of esd numbers (ending in a high-order bit).     */
    /* After that, there is a 1 byte count that tell use how   */
    /* many additional xlater records we have (we always have  */
    /* one.                                                    */
    /***********************************************************/
	while (cnt - cnt_start < idr_rec_length)
	  {
	    last_cnt = cnt;
	    nbr_cesd_entries = 1;
	    while ((*last_cnt & 0x80) == 0)
	      {
		    nbr_cesd_entries++;
	        last_cnt += 2;
	      }
	    idr_cnt = (unsigned char *)last_cnt + 2;
	    idr = (struct idr_xlater *)((unsigned char *)idr_cnt + 1);
	    ebcdic_to_ascii((unsigned char *)idr->name,sizeof(idr->name));
        packed_char3_to_mmddyy((char *)idr->yyddd_packed,&mm,&dd,&yy);
		cnt -= 2;
        /***********************************************************/
        /* For each esd entry valid for this xlater record         */
        /***********************************************************/
		do
	      {
			cnt += 2;
            /***********************************************************/
            /* Print this guy.                                         */
            /***********************************************************/
			hs[0] = cnt[0]&0x7f; hs[1] = cnt[1];
			cur_esd = (struct esd_entry *)b316_get_esd_entry(s_cnt);
			if (cur_esd == 0)
			  {
		        printmsg(MSG_LIST , 
					"ESD# %03d produced by %-10.10s %d.%d on %02d/%02d/%02d",
			        s_cnt, idr->name, idr->version, idr->level,
				    mm,dd,yy);
			  }
			else
			  {
				strncpy(cur_esd->product_name,idr->name,10);
				cur_esd->product_name[10]='\0';
				sprintf(cur_esd->build_date_mmddyy,"%02d/%02d/%02d",mm,dd,yy);
				sprintf(cur_esd->build_date_yymmdd,"%02d/%02d/%02d",yy,mm,dd);
				sprintf(cur_esd->product_ver,"%d.%d",idr->version,idr->level);
				b318_check_for_plip_mod(cur_esd);
		        printmsg(MSG_LIST , 
				  "%-8.8s produced by %-10.10s %d.%d on %02d/%02d/%02d",
			      cur_esd->name, idr->name, idr->version, idr->level,
				  mm,dd,yy);
			  }
            /***********************************************************/
            /* If we have any additional for this guy, print them.     */
            /***********************************************************/
			next_idr = last_idr = idr;
			for (i = 0; i < (int)(*idr_cnt); i++)
			  {
				next_idr++;
	            ebcdic_to_ascii((unsigned char *)next_idr->name,sizeof(next_idr->name));
                packed_char3_to_mmddyy((char *)next_idr->yyddd_packed,&mm,&dd,&yy);
			    if (cur_esd == 0)
		          printmsg(MSG_LIST , 
					"                     %-10.10s %d.%d on %02d/%02d/%02d",
			        next_idr->name, next_idr->version, next_idr->level,
				    mm,dd,yy);
			    else
		          printmsg(MSG_LIST , "                     %-10.10s %d.%d on %02d/%02d/%02d",
			                       next_idr->name, next_idr->version, next_idr->level,
				    mm,dd,yy);
			  }
			last_idr = next_idr;
	      } while ((*cnt & 0x80) != 0x80);
	    idr = last_idr+1;
	    cnt = (unsigned char *)idr;
	  }
}
/*************************************************************************/
/*                                                                       */
/*  Find the esd entry with the associated number.                       */
/*                                                                       */
/*************************************************************************/
int b316_get_esd_entry(long int nbr)
{
	struct	esd_entry	*cur_esd;
	FOR_EACH_ESD_NODE(cur_esd)
	  {
		if (cur_esd->nbr == nbr)
		  break;
	  }
	return((int)cur_esd);
}
/*************************************************************************/
/*                                                                       */
/*  Free the esd link list.                                              */
/*                                                                       */
/*************************************************************************/
void b317_free_esd_entries(void)
{
	struct	esd_entry	*cur_esd,*tmp_esd;
	for (cur_esd = esd_head; cur_esd; cur_esd = tmp_esd)
	  {
		tmp_esd = cur_esd->next;
		free(cur_esd);
	  }
	esd_head = esd_tail = 0;
}
/*************************************************************************/
/*                                                                       */
/*  see if this exec_bin calls any modules that were taged as plip       */
/*  change those to pli and add compile_units                            */
/*                                                                       */
/*************************************************************************/
void b318_check_for_plip_mod(struct esd_entry *cur_esd)
{
char sect_name[12];
char buf[256];
char *name_ptr;
char *product_type;
int  rc,next_row;

if(no_database == TRUE || update == FALSE)
	return;

memset(sect_name,'\0',12);
/* find the product type based on the product name */
product_type = (char *)a311_product_type(cur_esd->product_name);

/* if we find a product type and it starts with PLI */
if(product_type != cur_esd->product_name &&
	strncmp(product_type,"PLI",3) == 0)
	{
	/* grab the ESD name           */
	/* copy the first 7 characters */
	strncpy(sect_name,cur_esd->name,7);
	/* skip over any leading asteriks */
	for(name_ptr = sect_name; *name_ptr == '*'; name_ptr++);

	sprintf(buf,"exec handle_pli_csect %d,'%s'",exec_bin_key,name_ptr);
	dbcancel(dbproc);
	dbfreebuf(dbproc);
	dbcmd(dbproc,buf);
	rc = dbsqlexec(dbproc);
	if(rc == FAIL)
		{
		if(v100_errmsg(10,"b318_check_for_plip_mod") == ESFAT)
			exit(ESFAT);
		goto end_routine;
		}
	while((rc = dbresults(dbproc) ) != NO_MORE_RESULTS)
		{
		while((next_row = dbnextrow(dbproc)) != NO_MORE_ROWS)
			{
			}
		}
	}
end_routine:;
}

/*************************************************************************/
/*                                                                       */
/*  Process the user IDR records.                                        */
/*                                                                       */
/*************************************************************************/
void b320_process_idr_user(void)
{
	unsigned char	*length;
	char	*text;
	unsigned char	*c = (unsigned char *)idr_rec;
	unsigned char *last_c = (unsigned char *)idr_rec + idr_rec_length;
	unsigned short	esd_id;
	unsigned char *esd_char = (unsigned char *)&esd_id;
	short	mm,dd,yy;
	char	esd_text[258];
	struct	esd_entry	*cur_esd;

	printmsg(MSG_LISTRC,"User Data IDR Record.");

    while (c < last_c)
	  {
	    esd_char[0] = *c++;
	    esd_char[1] = *c++;
        packed_char3_to_mmddyy((char *)c,&mm,&dd,&yy);
	    c += 3;
	    length = c++;
	    text = (char *)c;
	    c += *length;
	    ebcdic_to_ascii((unsigned char *)text,*length);
		strncpy(esd_text,text,*length);
		esd_text[*length] = '\0';
		cur_esd = (struct esd_entry *)b316_get_esd_entry(esd_id);
		if (cur_esd == 0)
		  printmsg(MSG_LIST , "ESD# %03d on %02d/%02d/%02d Data: %s",
			    esd_id, mm,dd,yy, esd_text);
		else
		  printmsg(MSG_LIST , "%s on %02d/%02d/%02d Data: %s",
			    cur_esd->name, mm,dd,yy, esd_text);
	  };
}
/*************************************************************************/
/*                                                                       */
/*  This record combines the RLD entries.                                */
/*                                                                       */
/*************************************************************************/
void b400_process_rld(void)
{
	struct	rld_entry	*rld;
	struct	rld_fl_entry	*rld_fl;
	char	*rld_start;

	rld_base = (struct rld_base_entry *)input_rec;
	rld = (struct rld_entry *)((char *)input_rec + sizeof(struct rld_base_entry));
	rld_start = (char *)rld;

	printmsg(MSG_LISTRC,"RLD Record Found - Size %03d",
			rld_base->rld_size);

	while ((int)((char *)rld - rld_start) < (int)(rld_base->rld_size) )
	  {
	    if (rld->r_ptr <= 255)
	      {
		  rld_fl = (struct rld_fl_entry *)((char *)rld + sizeof(struct rld_entry));
		  printmsg(MSG_LIST  ,"     R-Ptr: %3d  P-Ptr: %3d  FL: %02X  Addr: %06X",
				rld->r_ptr, rld->p_ptr, rld_fl->fl, char3addr_to_long((char *)rld_fl->addr));
			rld++;
	      }
	    else
	      {
			rld_fl = (struct rld_fl_entry *)rld;
		    printmsg(MSG_LIST  ,"                             FL: %02X  Addr: %06X",
                        rld_fl->fl, char3addr_to_long((char *)rld_fl->addr));
	      }
		rld++;
	  }
	 
}
/*************************************************************************/
/*                                                                       */
/*  This record combines CTL and RLD entries.                            */
/*                                                                       */
/*************************************************************************/
void b500_process_ctlrld(void)
{
	long	nbr_ctl_entries;
	struct	rld_entry	*rld;
	struct	ctl_entry	*ctl;
	struct	rld_fl_entry	*rld_fl;
	char	*rld_start;
	long	i;
	struct esd_entry	*cur_esd;

	ctl_base = (struct ctl_base_entry *)input_rec;

	nbr_ctl_entries = ctl_base->ctl_size / sizeof(struct ctl_entry);

	printmsg(MSG_LISTRC,"CTL/RLD Record Found - CTL: %03d  RLD: %03d RLD/CTL Cnt: %d  CCW: %08X %08X",
			ctl_base->ctl_size, ctl_base->rld_size,
			ctl_base->rld_ctl_cnt, ctl_base->ccw, ctl_base->ccw+4);

	rld = (struct rld_entry *)((char *)input_rec + sizeof(struct ctl_base_entry));
	rld_start = (char *)rld;

	while ((int)((char *)rld - rld_start) < (int)(ctl_base->rld_size) )
	  {
	    if (rld->r_ptr <= 255)
	      {
		    rld_fl = (struct rld_fl_entry *)( (char *)rld + sizeof(struct rld_entry) );
		    printmsg(MSG_LIST  ,"     R-Ptr: %3d  P-Ptr: %3d  FL: %02X  Addr: %06X",
				rld->r_ptr, rld->p_ptr, rld_fl->fl, char3addr_to_long((char *)rld_fl->addr));
			rld++;
	      }
	    else
	      {
			rld_fl = (struct rld_fl_entry *)rld;
		    printmsg(MSG_LIST  ,"                             FL: %02X  Addr: %06X",
				                        rld_fl->fl, char3addr_to_long((char *)rld_fl->addr));
	      }
		rld++;
	  }

	ctl = (struct ctl_entry *)rld;
	for (i = 0; i < nbr_ctl_entries; i++,ctl++)
	  {
		cur_esd = (struct esd_entry *)b316_get_esd_entry(ctl->cesd_nbr);
		if (cur_esd == 0)
		  printmsg(MSG_LIST  ,"     CESD#: %d  Length: %06X",
				ctl->cesd_nbr, ctl->cesd_length);
		else
		  printmsg(MSG_LIST  ,"      %-8.8s  Length: %06X",
				cur_esd->name, ctl->cesd_length);
	  }
}
/*************************************************************************/
/*                                                                       */
/*  This routine will read one input record and place it into input_buf. */
/*  It will return the length of the record read.                        */
/*  It will also get rid of the new -line character if appropriate.      */
/*                                                                       */
/*  Return Codes:                                                        */
/*       0 - bad                                                         */
/*      >0 - good, returns the length of the record read.                */
/*                                                                       */
/*************************************************************************/
int read_record(char *input_buf)
{
    long    rc;
    short   cnt;
    char    *c;

    /***********************************************************/
    /* If the file has a two byte length, read it first, then  */
    /* read the remainder of the file.                         */
    /***********************************************************/
    if (len_added)
      {
        rc = read(inputfd,&cnt,2);
        if (rc != 2)
          {
          if (rc == 0)
            return(0);
          if(v100_errmsg(67,filename) == ESFAT)
			exit(ESFAT);
          return(0);
          }
        rc = read(inputfd,input_buf,cnt);
        if (rc != cnt)
          {
          if(v100_errmsg(68,cnt, rc) == ESFAT)
			exit(ESFAT);
           return(0);
          }
        return(rc);
      }

    /***********************************************************/
    /* If the file doesn't have a two byte length, we can use  */
    /* fgets.                                                  */
    /***********************************************************/
    if (!len_added)
      {
        c = fgets(input_buf,INPUT_BUF_SIZE,input);
        if (c == NULL)
          return(0);
        rc = strlen(input_buf);
        /*******************************************/
        /* Zap the new line character if it exists */
        /*******************************************/
        if (rc > 0)
          {
            if (input_buf[rc-1] == '\n')
              {
                input_buf[rc-1] = '\0';     /*  Get rid of the new-line character */
                rc--;
              }
          }
        return(rc);
      }
}
/*************************************************************************/
/*                                                                       */
/*  Open a data base connection.                                         */
/*                                                                       */
/*************************************************************************/
void s100_open_db_connection(void)
{
static   LOGINREC *login;
static struct PDBINFO dbinfo =  {"","","ALSLD ","",""};

if (no_database)
      return;

db_init(&login, &dbinfo, &dbproc, NULL, NULL);
}
/*************************************************************************/
/*                                                                       */
/*  Close the connection to the database.                                */
/*                                                                       */
/*************************************************************************/
void s900_close_db_connection(void)
{
	if (no_database)
	  return;

    dbclose(dbproc);
}

/*************************************************************************/
/*                                                                       */
/*  Print an information message.  The CTL bits tell us whether we       */
/*  should be printing this information.                                 */
/*                                                                       */
/*************************************************************************/
void printmsg(int ctl, char *vals, ...)
{
char    buf[5000];
va_list ap_list;

if ((ctl == MSG_ALWAYS) || (debug & ctl))
      {
	va_start(ap_list,vals);
	vsprintf(buf,vals,ap_list);
	va_end(ap_list);
        fprintf(stderr,"%s",buf);
      }
}
/*************************************************************************/
/*                                                                       */
/*  Convert a packed char3 field in yyddd format to mmddyy.              */
/*                                                                       */
/*************************************************************************/
void packed_char3_to_mmddyy(char *date_data, short int *mm, short int *dd, short int *yy)
{
	unsigned short	hold_short;
	unsigned char	*hold_data = (unsigned char *)&hold_short;
	struct	tm	*local_time;
	unsigned long cur_sec;

    /*  Get the year */
	hold_data[0] = '\0';
    hold_data[1] = date_data[0];
    *yy = packed_short_to_binary(hold_short);

	hold_data[0] = date_data[1];
    hold_data[1] = date_data[2];

	*mm = *dd = 1;
	cur_sec = internaldate(mm,dd,yy);
	cur_sec += (packed_short_to_binary(hold_short) * 86400);
	local_time = localtime((time_t *)(&cur_sec));
	*mm = local_time->tm_mon+1;
    *dd = local_time->tm_mday;
    *yy = local_time->tm_year;
}
/******************************************************/
/*                                                    */
/* Takes a packed two bytes of data and converts it   */
/* into binary.                                       */
/*                                                    */
/******************************************************/
int packed_short_to_binary(short int from)
{
    char    value[10];
    short   hold_from;
    long    i;

    for (i = 0; i < 4; i++)
      {    
        hold_from = 0x0f & from;
        if (hold_from >= 0 && hold_from <= 9)
          value[3-i] = 0x30 | hold_from;
        from = from >> 4;
      }
    value[4] = '\0';
    sscanf(value,"%d",&i);
    return(i);
}
/*************************************************************************/
/*                                                                       */
/*  Build our hexd to decimal conversion table.                          */
/*                                                                       */
/*************************************************************************/
void build_xlate_table(void)
{
    xlate['a'] = 10;
    xlate['b'] = 11;
    xlate['c'] = 12;
    xlate['d'] = 13;
    xlate['e'] = 14;
    xlate['f'] = 15;
    xlate['A'] = 10;
    xlate['B'] = 11;
    xlate['C'] = 12;
    xlate['D'] = 13;
    xlate['E'] = 14;
    xlate['F'] = 15;
    xlate['1'] = 1;
    xlate['2'] = 2;
    xlate['3'] = 3;
    xlate['4'] = 4;
    xlate['5'] = 5;
    xlate['6'] = 6;
    xlate['7'] = 7;
    xlate['8'] = 8;
    xlate['9'] = 9;
    xlate[0] = '0';
    xlate[1] = '1';
    xlate[2] = '2';
    xlate[3] = '3';
    xlate[4] = '4';
    xlate[5] = '5';
    xlate[6] = '6';
    xlate[7] = '7';
    xlate[8] = '8';
    xlate[9] = '9';
    xlate[10] = 'A';
    xlate[11] = 'B';
    xlate[12] = 'C';
    xlate[13] = 'D';
    xlate[14] = 'E';
    xlate[15] = 'F';
}
void ebcdic_to_ascii(register unsigned char *c, long int nbr)
{
    register    int i;
      for (i = 0; i < nbr; i++,c++)
        etoa(*c);
}
/******************************************************/
/*                                                    */
/* converts yymmdd to secs since 1970                 */
/*                                                    */
/******************************************************/
int internaldate(short int *month, short int *day, short int *year)
{
static  int dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    register int i;
    int hour, mins, secs;
    int newsecs;

    if (*month < 1 || *month > 12 ||
       *day < 1 || *day > 31)
      return (0);

    if ((my_dysize(*year) == 366) &&
        (*month == 2))
      {
        if (*day > 29)
          return(0);
      }
    else
      if (dmsize[*month-1] < *day)
        return (0);
    hour = 0;
    mins = 0;
    secs = 1;
    newsecs = 0;
    *year += 1900;
                 /* calculate days since 1970 */
    if (*year < 1970)
      {
        for (i = 1969; i > *year; i--)
          newsecs += my_dysize(i);
                /* adjust for leap year */
        if (my_dysize(*year) == 366 && *month >= 3)
          newsecs--;
        for (i = *month+1; i <= 12; i++)
          newsecs += dmsize[i-1];
        newsecs += (dmsize[*month-1] - *day + 1);
                /* calculate seconds from these days */
        newsecs = 24*newsecs + hour;
        newsecs = 60*newsecs + mins;
        newsecs = 60*newsecs - secs;
        newsecs = 0 - newsecs;
      }
    else
      { 
        for (i = 1970; i < *year; i++)
          newsecs += my_dysize(i);
                /* adjust for leap year */
        if (my_dysize(*year) == 366 && *month >= 3)
          newsecs++;
        while (--*month)
          newsecs += dmsize[*month-1];
        newsecs += *day-1;
                /* calculate seconds from these days */
        newsecs = 24*newsecs + hour;
        newsecs = 60*newsecs + mins;
        newsecs = 60*newsecs + secs;
      }
    return(newsecs);
}
/*************************************************************************/
/*                                                                       */
/*  Take a char[3] field that is in binary and convert it into a long.   */
/*                                                                       */
/*************************************************************************/
int char3addr_to_long(char *addr)
{
	long	value; 
	unsigned char *c = (unsigned char *)&value;
	*(c+0) = 0;
	*(c+1) = addr[0];
	*(c+2) = addr[1];
	*(c+3) = addr[2];
	return(value);
}

my_dysize(short year)
{
short d4,d100,d400;
int days;
 
if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0 )
	days = 366;
else
	days = 365;
	return(days);
}
