
/* Result Sets Interface */
#ifndef SQL_CRSR
#  define SQL_CRSR
  struct sql_cursor
  {
    unsigned int curocn;
    void *ptr1;
    void *ptr2;
    unsigned int magic;
  };
  typedef struct sql_cursor sql_cursor;
  typedef struct sql_cursor SQL_CURSOR;
#endif /* SQL_CRSR */

/* Thread Safety */
typedef void * sql_context;
typedef void * SQL_CONTEXT;

/* Object support */
struct sqltvn
{
  unsigned char *tvnvsn; 
  unsigned short tvnvsnl; 
  unsigned char *tvnnm;
  unsigned short tvnnml; 
  unsigned char *tvnsnm;
  unsigned short tvnsnml;
};
typedef struct sqltvn sqltvn;

struct sqladts
{
  unsigned int adtvsn; 
  unsigned short adtmode; 
  unsigned short adtnum;  
  sqltvn adttvn[1];       
};
typedef struct sqladts sqladts;

static struct sqladts sqladt = {
  1,1,0,
};

/* Binding to PL/SQL Records */
struct sqltdss
{
  unsigned int tdsvsn; 
  unsigned short tdsnum; 
  unsigned char *tdsval[1]; 
};
typedef struct sqltdss sqltdss;
static struct sqltdss sqltds =
{
  1,
  0,
};

/* File name & Package Name */
struct sqlcxp
{
  unsigned short fillen;
           char  filnam[10];
};
static const struct sqlcxp sqlfpn =
{
    9,
    "bridge.pc"
};


static unsigned int sqlctx = 36339;


static struct sqlexd {
   unsigned int   sqlvsn;
   unsigned int   arrsiz;
   unsigned int   iters;
   unsigned int   offset;
   unsigned short selerr;
   unsigned short sqlety;
   unsigned int   occurs;
      const short *cud;
   unsigned char  *sqlest;
      const char  *stmt;
   sqladts *sqladtp;
   sqltdss *sqltdsp;
            void  **sqphsv;
   unsigned int   *sqphsl;
            int   *sqphss;
            void  **sqpind;
            int   *sqpins;
   unsigned int   *sqparm;
   unsigned int   **sqparc;
   unsigned short  *sqpadto;
   unsigned short  *sqptdso;
   unsigned int   sqlcmax;
   unsigned int   sqlcmin;
   unsigned int   sqlcincr;
   unsigned int   sqlctimeout;
   unsigned int   sqlcnowait;
              int   sqfoff;
   unsigned int   sqcmod;
   unsigned int   sqfmod;
            void  *sqhstv[52];
   unsigned int   sqhstl[52];
            int   sqhsts[52];
            void  *sqindv[52];
            int   sqinds[52];
   unsigned int   sqharm[52];
   unsigned int   *sqharc[52];
   unsigned short  sqadto[52];
   unsigned short  sqtdso[52];
} sqlstm = {12,52};

// Prototypes
extern "C" {
  void sqlcxt (void **, unsigned int *,
               struct sqlexd *, const struct sqlcxp *);
  void sqlcx2t(void **, unsigned int *,
               struct sqlexd *, const struct sqlcxp *);
  void sqlbuft(void **, char *);
  void sqlgs2t(void **, char *);
  void sqlorat(void **, unsigned int *, void *);
}

// Forms Interface
static const int IAPSUCC = 0;
static const int IAPFAIL = 1403;
static const int IAPFTL  = 535;
extern "C" { void sqliem(unsigned char *, signed int *); }

typedef struct { unsigned short len; unsigned char arr[1]; } VARCHAR;
typedef struct { unsigned short len; unsigned char arr[1]; } varchar;

#define SQLCODE sqlca.sqlcode

/* cud (compilation unit data) array */
static const short sqlcud0[] =
{12,4130,852,0,0,
5,0,0,1,0,0,544,188,0,0,0,0,0,1,0,
20,0,0,2,0,0,544,195,0,0,0,0,0,1,0,
35,0,0,0,0,0,539,203,0,0,4,4,0,1,0,1,97,0,0,1,10,0,0,1,10,0,0,1,10,0,0,
66,0,0,4,0,0,542,210,0,0,0,0,0,1,0,
81,0,0,5,109,0,516,224,0,0,2,1,0,1,0,2,97,0,0,1,97,0,0,
104,0,0,6,124,0,529,231,0,0,0,0,0,1,0,
119,0,0,6,0,0,557,233,0,0,1,1,0,1,0,1,97,0,0,
138,0,0,6,0,0,525,234,0,0,1,0,0,1,0,2,97,0,0,
157,0,0,6,0,0,527,235,0,0,0,0,0,1,0,
172,0,0,6,0,0,527,240,0,0,0,0,0,1,0,
187,0,0,7,85,0,516,248,0,0,2,1,0,1,0,1,3,0,0,2,97,0,0,
210,0,0,8,100,0,516,252,0,0,1,0,0,1,0,2,97,0,0,
229,0,0,9,1855,0,518,318,0,0,52,52,0,1,0,2,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,
3,0,0,1,3,0,0,1,3,0,0,1,3,0,0,1,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,3,0,0,1,3,0,
0,1,3,0,0,1,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,
0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,
0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,3,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,
97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,
1,97,0,0,1,97,0,0,1,97,0,0,
452,0,0,10,0,0,529,382,0,0,1,1,0,1,0,1,97,0,0,
471,0,0,10,0,0,557,384,0,0,4,4,0,1,0,1,97,0,0,1,97,0,0,1,97,0,0,1,97,0,0,
502,0,0,10,0,0,525,387,0,0,23,0,0,1,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,
0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,
97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,97,0,0,2,3,0,0,
2,3,0,0,
609,0,0,10,0,0,527,390,0,0,0,0,0,1,0,
};


#include "stdafx.h"
#include <sqlcpr.h>
#include "bridge.h"

static const char BRIDGE[] = "bridge", BRIDGE_GetManageNumber[] = "bridge.getManageNumber", 
  BRIDGE_InsertImageInfoToDB[] = "bridge.insertImageInfoToDB", 
  BRIDGE_GetWorklistFromDB[] = "bridge.GetWorklistFromDB";
static const char *ErrorModuleName = NULL;
static char ErrorMessage[1024];
static size_t bufferSize = 1023;
static size_t messageLength = 0;

static bool connected = false;

/* EXEC SQL BEGIN DECLARE SECTION; */ 

const unsigned char connection[] = "dicom/dicom@dicomdb";

// ---------StoreSCP-----------
char oldStudyUid[65] = "";
char imageManageNumber[15];
int dateNumber;

//dataset begin
char paramHddRoot[65];
char paramPath[257];
char paramImgManageNum[15];
int paramFileDate, paramFileTime, paramInsertDate, paramInsertTime;
__int64 paramFileSize;
char paramTransferSyntaxUid[65];
char paramSopClaUid[65];
char paramSopInsUid[65];
int paramStuDat;
int paramSerDat;
int paramStuTim;
int paramSerTim;
char paramAccNum[17];
char paramSeriesModality[17];
char paramStudyModality[65];
char paramManufacturer[65];
char paramInstNam[65];
char paramRefPhyNam[65];
char paramStationName[17];
char paramStuDes[65];
char paramSerDes[65];
char paramInstDepartName[65];
char paramPhyRecNam[65];
char paramPerformPhyName[65];
char paramPhyReadStuName[65];
char paramOperateName[65];
char paramManufactModNam[65];
char paramPatNam[65];
char paramPatNamKan[65];
char paramPatNamKat[65];
char paramPatId[65];
int paramPatBirDat;
char paramPatSex[17];
char paramPatAge[5];
char paramPatSiz[17];
char paramPatWei[17];
char paramContBolus[65];
char paramBodParExam[17];
char paramProtocolNam[65];
char paramPatPos[17];
char paramViewPos[17];
char paramStuInsUid[65];
char paramSerInsUid[65];
char paramStuId[17];
char paramSerNum[13];
char paramImaNum[13];
char paramReqPhysician[65];
char paramReqService[65];
//dataset end
int packageId;

// ---------WLM Condition-----------
const char *pWorklistStatement;

#ifndef __BRIDGE_DEFINE

#define DIC_UI_LEN		64
#define DIC_AE_LEN		16
#define DIC_SH_LEN		16
#define DIC_PN_LEN		64
#define DIC_LO_LEN		64
#define DIC_CS_LEN		16
#define DIC_AS_LEN		4
#define DIC_DA_LEN		8
#define DIC_TM_LEN		16
#define DIC_DS_LEN		16

typedef struct tagWlmCondition
{
  const char *pScheduleStationAE;
  const char *pModality;
  const char *pLowerScheduleDate;
  const char *pUpperScheduleDate;
} WlmCondition, *PWlmCondition;	

typedef struct tagWorklistRecord
{
  char ScheduledStationAETitle[DIC_AE_LEN + 1];
  char SchdldProcStepStartDate[DIC_DA_LEN + 1];
  char SchdldProcStepStartTime[DIC_TM_LEN + 1];
  char Modality[DIC_CS_LEN + 1];
  char SchdldProcStepDescription[DIC_LO_LEN + 1];
  char SchdldProcStepLocation[DIC_SH_LEN + 1];
  char SchdldProcStepID[DIC_SH_LEN + 1];
  char RequestedProcedureID[DIC_SH_LEN + 1];
  char RequestedProcedureDescription[DIC_LO_LEN + 1];
  char StudyInstanceUID[DIC_UI_LEN + 1];
  char AccessionNumber[DIC_SH_LEN + 1];
  char RequestingPhysician[DIC_PN_LEN + 1];
  char AdmissionID[DIC_LO_LEN + 1];
  char PatientsNameEn[DIC_PN_LEN + 1];
  char PatientsNameCh[DIC_PN_LEN + 1];
  char PatientID[DIC_LO_LEN + 1];
  char PatientsBirthDate[DIC_DA_LEN + 1];
  char PatientsSex[DIC_CS_LEN + 1];
  char PatientsWeight[DIC_DS_LEN + 1];
  char AdmittingDiagnosesDescription[DIC_LO_LEN + 1];
  char PatientsAge[DIC_AS_LEN + 1];
  int SupportChinese;
  int DicomPersonName;
} WorklistRecord, *PWorklistRecord;

typedef struct tagIndicatorWorklistRecord
{
  short ScheduledStationAETitle;
  short SchdldProcStepStartDate;
  short SchdldProcStepStartTime;
  short Modality;
  short SchdldProcStepDescription;
  short SchdldProcStepLocation;
  short SchdldProcStepID;
  short RequestedProcedureID;
  short RequestedProcedureDescription;
  short StudyInstanceUID;
  short AccessionNumber;
  short RequestingPhysician;
  short AdmissionID;
  short PatientsNameEn;
  short PatientsNameCh;
  short PatientID;
  short PatientsBirthDate;
  short PatientsSex;
  short PatientsWeight;
  short AdmittingDiagnosesDescription;
  short PatientsAge;
  short SupportChinese;
  short DicomPersonName;
} IndicatorWorklistRecord, *PIndicatorWorklistRecord;
#endif

WlmCondition SearchCondition;
WorklistRecord WorklistInBridge;
IndicatorWorklistRecord IndicatorWorklist;

/* EXEC SQL END DECLARE SECTION; */ 


/* EXEC SQL INCLUDE SQLCA.H;
 */ 
/*
 * $Header: sqlca.h 24-apr-2003.12:50:58 mkandarp Exp $ sqlca.h 
 */

/* Copyright (c) 1985, 2003, Oracle Corporation.  All rights reserved.  */
 
/*
NAME
  SQLCA : SQL Communications Area.
FUNCTION
  Contains no code. Oracle fills in the SQLCA with status info
  during the execution of a SQL stmt.
NOTES
  **************************************************************
  ***                                                        ***
  *** This file is SOSD.  Porters must change the data types ***
  *** appropriately on their platform.  See notes/pcport.doc ***
  *** for more information.                                  ***
  ***                                                        ***
  **************************************************************

  If the symbol SQLCA_STORAGE_CLASS is defined, then the SQLCA
  will be defined to have this storage class. For example:
 
    #define SQLCA_STORAGE_CLASS extern
 
  will define the SQLCA as an extern.
 
  If the symbol SQLCA_INIT is defined, then the SQLCA will be
  statically initialized. Although this is not necessary in order
  to use the SQLCA, it is a good pgming practice not to have
  unitialized variables. However, some C compilers/OS's don't
  allow automatic variables to be init'd in this manner. Therefore,
  if you are INCLUDE'ing the SQLCA in a place where it would be
  an automatic AND your C compiler/OS doesn't allow this style
  of initialization, then SQLCA_INIT should be left undefined --
  all others can define SQLCA_INIT if they wish.

  If the symbol SQLCA_NONE is defined, then the SQLCA variable will
  not be defined at all.  The symbol SQLCA_NONE should not be defined
  in source modules that have embedded SQL.  However, source modules
  that have no embedded SQL, but need to manipulate a sqlca struct
  passed in as a parameter, can set the SQLCA_NONE symbol to avoid
  creation of an extraneous sqlca variable.
 
MODIFIED
    lvbcheng   07/31/98 -  long to int
    jbasu      12/12/94 -  Bug 217878: note this is an SOSD file
    losborne   08/11/92 -  No sqlca var if SQLCA_NONE macro set 
  Clare      12/06/84 - Ch SQLCA to not be an extern.
  Clare      10/21/85 - Add initialization.
  Bradbury   01/05/86 - Only initialize when SQLCA_INIT set
  Clare      06/12/86 - Add SQLCA_STORAGE_CLASS option.
*/
 
#ifndef SQLCA
#define SQLCA 1
 
struct   sqlca
         {
         /* ub1 */ char    sqlcaid[8];
         /* b4  */ int     sqlabc;
         /* b4  */ int     sqlcode;
         struct
           {
           /* ub2 */ unsigned short sqlerrml;
           /* ub1 */ char           sqlerrmc[70];
           } sqlerrm;
         /* ub1 */ char    sqlerrp[8];
         /* b4  */ int     sqlerrd[6];
         /* ub1 */ char    sqlwarn[8];
         /* ub1 */ char    sqlext[8];
         };

#ifndef SQLCA_NONE 
#ifdef   SQLCA_STORAGE_CLASS
SQLCA_STORAGE_CLASS struct sqlca sqlca
#else
         struct sqlca sqlca
#endif
 
#ifdef  SQLCA_INIT
         = {
         {'S', 'Q', 'L', 'C', 'A', ' ', ' ', ' '},
         sizeof(struct sqlca),
         0,
         { 0, {0}},
         {'N', 'O', 'T', ' ', 'S', 'E', 'T', ' '},
         {0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0, 0, 0, 0}
         }
#endif
         ;
#endif
 
#endif
 
/* end SQLCA */


const char *GetErrorMessage()
{
  return ErrorMessage;
}

const char *GetErrorModuleName()
{
  return ErrorModuleName;
}

void logError(std::ostream &outputStream)
{
  outputStream << GetErrorModuleName() << ':' << GetErrorMessage() << std::endl;
}

bool SqlError( const char* moduleName )
{
  ErrorModuleName = moduleName;
  if(sqlca.sqlcode < 0)
  {
	sqlglm(reinterpret_cast<unsigned char *>(ErrorMessage), &bufferSize, &messageLength);
	ErrorMessage[messageLength] = '\0';
  }
  else
	strncpy(ErrorMessage, sqlca.sqlerrm.sqlerrmc, sizeof(ErrorMessage));
  /* EXEC SQL  WHENEVER SQLERROR CONTINUE; */ 

  /* EXEC SQL ROLLBACK RELEASE; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 0;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )5;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
}


  connected = false;
  return FALSE;
}

bool rollbackDicomDB()
{
  /* EXEC SQL ROLLBACK RELEASE; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 0;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )20;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
}


  connected = false;
  return TRUE;
}

static bool connectDicomDB()
{
  /* EXEC SQL  WHENEVER SQLERROR DO return( SqlError( BRIDGE ) ); */ 

  /* EXEC SQL CONNECT :connection; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 4;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.iters = (unsigned int  )10;
  sqlstm.offset = (unsigned int  )35;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlstm.sqhstv[0] = (         void  *)connection;
  sqlstm.sqhstl[0] = (unsigned int  )0;
  sqlstm.sqhsts[0] = (         int  )0;
  sqlstm.sqindv[0] = (         void  *)0;
  sqlstm.sqinds[0] = (         int  )0;
  sqlstm.sqharm[0] = (unsigned int  )0;
  sqlstm.sqadto[0] = (unsigned short )0;
  sqlstm.sqtdso[0] = (unsigned short )0;
  sqlstm.sqphsv = sqlstm.sqhstv;
  sqlstm.sqphsl = sqlstm.sqhstl;
  sqlstm.sqphss = sqlstm.sqhsts;
  sqlstm.sqpind = sqlstm.sqindv;
  sqlstm.sqpins = sqlstm.sqinds;
  sqlstm.sqparm = sqlstm.sqharm;
  sqlstm.sqparc = sqlstm.sqharc;
  sqlstm.sqpadto = sqlstm.sqadto;
  sqlstm.sqptdso = sqlstm.sqtdso;
  sqlstm.sqlcmax = (unsigned int )100;
  sqlstm.sqlcmin = (unsigned int )2;
  sqlstm.sqlcincr = (unsigned int )1;
  sqlstm.sqlctimeout = (unsigned int )0;
  sqlstm.sqlcnowait = (unsigned int )0;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE));
}


  connected = true;
  return TRUE;
}

bool commitDicomDB()
{
  /* EXEC SQL COMMIT WORK RELEASE; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 4;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )66;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE));
}


  connected = false;
  return TRUE;
}

bool getManageNumber(char * const outImageManageNum, const char * const studyUid, int currentStudyDateNumber, ostream *pLogStream)
{
  if( ! (connected ? true : connectDicomDB()) ) return FALSE;
  /* EXEC SQL  WHENEVER SQLERROR DO return( SqlError( BRIDGE_GetManageNumber ) ); */ 

  if(studyUid)
  {
	// select image manage number from old study level, imange manage number including '_' is invalid.
    strncpy(oldStudyUid, studyUid, sizeof(oldStudyUid));
    /* EXEC SQL WHENEVER NOT FOUND GOTO NO_STUDY_MANAGE_NUMBER; */ 

    /* EXEC SQL SELECT STU_IMGMANAGENUM INTO :imageManageNumber FROM STUDYLEVEL WHERE STU_STUINSUID=:oldStudyUid and INSTR(STU_IMGMANAGENUM, '_')=0; */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 4;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "select STU_IMGMANAGENUM into :b0  from STUDYLEVEL where \
(STU_STUINSUID=:b1 and INSTR(STU_IMGMANAGENUM,'_')=0)";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )81;
    sqlstm.selerr = (unsigned short)1;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlstm.sqhstv[0] = (         void  *)imageManageNumber;
    sqlstm.sqhstl[0] = (unsigned int  )15;
    sqlstm.sqhsts[0] = (         int  )0;
    sqlstm.sqindv[0] = (         void  *)0;
    sqlstm.sqinds[0] = (         int  )0;
    sqlstm.sqharm[0] = (unsigned int  )0;
    sqlstm.sqadto[0] = (unsigned short )0;
    sqlstm.sqtdso[0] = (unsigned short )0;
    sqlstm.sqhstv[1] = (         void  *)oldStudyUid;
    sqlstm.sqhstl[1] = (unsigned int  )65;
    sqlstm.sqhsts[1] = (         int  )0;
    sqlstm.sqindv[1] = (         void  *)0;
    sqlstm.sqinds[1] = (         int  )0;
    sqlstm.sqharm[1] = (unsigned int  )0;
    sqlstm.sqadto[1] = (unsigned short )0;
    sqlstm.sqtdso[1] = (unsigned short )0;
    sqlstm.sqphsv = sqlstm.sqhstv;
    sqlstm.sqphsl = sqlstm.sqhstl;
    sqlstm.sqphss = sqlstm.sqhsts;
    sqlstm.sqpind = sqlstm.sqindv;
    sqlstm.sqpins = sqlstm.sqinds;
    sqlstm.sqparm = sqlstm.sqharm;
    sqlstm.sqparc = sqlstm.sqharc;
    sqlstm.sqpadto = sqlstm.sqadto;
    sqlstm.sqptdso = sqlstm.sqtdso;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
    if (sqlca.sqlcode == 1403) goto NO_STUDY_MANAGE_NUMBER;
    if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


	if(pLogStream) *pLogStream << oldStudyUid << " found old image manage number in studylevel: " << imageManageNumber << endl;
    goto ReturnOutputString;

NO_STUDY_MANAGE_NUMBER:
	// select image manage number from old image level, imange manage number including '_' is invalid.
	/* EXEC SQL WHENEVER NOT FOUND GOTO NO_IMAGE_MANAGE_NUMBER; */ 

	/* EXEC SQL PREPARE StmtGetImgMngNum FROM SELECT DISTINCT SUBSTR(PATH,8,14) FROM IMAGELEVEL WHERE STUINSUID=:oldStudyInstanceUID AND INSTR(SUBSTR(PATH,8,14), '_')=0; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 4;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.stmt = "select distinct SUBSTR(PATH,8,14)  from IMAGELEVEL where (S\
TUINSUID=:oldStudyInstanceUID and INSTR(SUBSTR(PATH,8,14),'_')=0)";
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )104;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


	/* EXEC SQL DECLARE CurImgMngNum CURSOR FOR StmtGetImgMngNum; */ 

	/* EXEC SQL OPEN CurImgMngNum USING :oldStudyUid; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 4;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.stmt = "";
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )119;
 sqlstm.selerr = (unsigned short)1;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlstm.sqcmod = (unsigned int )0;
 sqlstm.sqhstv[0] = (         void  *)oldStudyUid;
 sqlstm.sqhstl[0] = (unsigned int  )65;
 sqlstm.sqhsts[0] = (         int  )0;
 sqlstm.sqindv[0] = (         void  *)0;
 sqlstm.sqinds[0] = (         int  )0;
 sqlstm.sqharm[0] = (unsigned int  )0;
 sqlstm.sqadto[0] = (unsigned short )0;
 sqlstm.sqtdso[0] = (unsigned short )0;
 sqlstm.sqphsv = sqlstm.sqhstv;
 sqlstm.sqphsl = sqlstm.sqhstl;
 sqlstm.sqphss = sqlstm.sqhsts;
 sqlstm.sqpind = sqlstm.sqindv;
 sqlstm.sqpins = sqlstm.sqinds;
 sqlstm.sqparm = sqlstm.sqharm;
 sqlstm.sqparc = sqlstm.sqharc;
 sqlstm.sqpadto = sqlstm.sqadto;
 sqlstm.sqptdso = sqlstm.sqtdso;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


	/* EXEC SQL FETCH CurImgMngNum INTO :imageManageNumber; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 4;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )138;
 sqlstm.selerr = (unsigned short)1;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlstm.sqfoff = (           int )0;
 sqlstm.sqfmod = (unsigned int )2;
 sqlstm.sqhstv[0] = (         void  *)imageManageNumber;
 sqlstm.sqhstl[0] = (unsigned int  )15;
 sqlstm.sqhsts[0] = (         int  )0;
 sqlstm.sqindv[0] = (         void  *)0;
 sqlstm.sqinds[0] = (         int  )0;
 sqlstm.sqharm[0] = (unsigned int  )0;
 sqlstm.sqadto[0] = (unsigned short )0;
 sqlstm.sqtdso[0] = (unsigned short )0;
 sqlstm.sqphsv = sqlstm.sqhstv;
 sqlstm.sqphsl = sqlstm.sqhstl;
 sqlstm.sqphss = sqlstm.sqhsts;
 sqlstm.sqpind = sqlstm.sqindv;
 sqlstm.sqpins = sqlstm.sqinds;
 sqlstm.sqparm = sqlstm.sqharm;
 sqlstm.sqparc = sqlstm.sqharc;
 sqlstm.sqpadto = sqlstm.sqadto;
 sqlstm.sqptdso = sqlstm.sqtdso;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode == 1403) goto NO_IMAGE_MANAGE_NUMBER;
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


	/* EXEC SQL CLOSE CurImgMngNum; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 4;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )157;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


	if(pLogStream) *pLogStream << oldStudyUid << " found old image manage number in imagelevel: " << imageManageNumber << endl;
    goto ReturnOutputString;

NO_IMAGE_MANAGE_NUMBER:
	/* EXEC SQL CLOSE CurImgMngNum; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 4;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )172;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


  }

  // no old image manage number, create new number.
  /* EXEC SQL WHENEVER NOT FOUND continue; */ 

  if(currentStudyDateNumber > 0)
  {
    dateNumber = currentStudyDateNumber;
    /* EXEC SQL SELECT TO_CHAR(:dateNumber) || TO_CHAR(LOCIMGDIRSEQ.NEXTVAL, 'FM000000') INTO :imageManageNumber FROM DUAL; */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 4;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "select (TO_CHAR(:b0)||TO_CHAR(LOCIMGDIRSEQ.nextval ,'FM0\
00000')) into :b1  from DUAL ";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )187;
    sqlstm.selerr = (unsigned short)1;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlstm.sqhstv[0] = (         void  *)&dateNumber;
    sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
    sqlstm.sqhsts[0] = (         int  )0;
    sqlstm.sqindv[0] = (         void  *)0;
    sqlstm.sqinds[0] = (         int  )0;
    sqlstm.sqharm[0] = (unsigned int  )0;
    sqlstm.sqadto[0] = (unsigned short )0;
    sqlstm.sqtdso[0] = (unsigned short )0;
    sqlstm.sqhstv[1] = (         void  *)imageManageNumber;
    sqlstm.sqhstl[1] = (unsigned int  )15;
    sqlstm.sqhsts[1] = (         int  )0;
    sqlstm.sqindv[1] = (         void  *)0;
    sqlstm.sqinds[1] = (         int  )0;
    sqlstm.sqharm[1] = (unsigned int  )0;
    sqlstm.sqadto[1] = (unsigned short )0;
    sqlstm.sqtdso[1] = (unsigned short )0;
    sqlstm.sqphsv = sqlstm.sqhstv;
    sqlstm.sqphsl = sqlstm.sqhstl;
    sqlstm.sqphss = sqlstm.sqhsts;
    sqlstm.sqpind = sqlstm.sqindv;
    sqlstm.sqpins = sqlstm.sqinds;
    sqlstm.sqparm = sqlstm.sqharm;
    sqlstm.sqparc = sqlstm.sqharc;
    sqlstm.sqpadto = sqlstm.sqadto;
    sqlstm.sqptdso = sqlstm.sqtdso;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
    if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


  }
  else
  {
    /* EXEC SQL SELECT TO_CHAR(SYSDATE, 'YYYYMMDD') || TO_CHAR(LOCIMGDIRSEQ.NEXTVAL, 'FM000000') INTO :imageManageNumber FROM DUAL; */ 

{
    struct sqlexd sqlstm;
    sqlstm.sqlvsn = 12;
    sqlstm.arrsiz = 4;
    sqlstm.sqladtp = &sqladt;
    sqlstm.sqltdsp = &sqltds;
    sqlstm.stmt = "select (TO_CHAR(SYSDATE,'YYYYMMDD')||TO_CHAR(LOCIMGDIRSE\
Q.nextval ,'FM000000')) into :b0  from DUAL ";
    sqlstm.iters = (unsigned int  )1;
    sqlstm.offset = (unsigned int  )210;
    sqlstm.selerr = (unsigned short)1;
    sqlstm.cud = sqlcud0;
    sqlstm.sqlest = (unsigned char  *)&sqlca;
    sqlstm.sqlety = (unsigned short)4352;
    sqlstm.occurs = (unsigned int  )0;
    sqlstm.sqhstv[0] = (         void  *)imageManageNumber;
    sqlstm.sqhstl[0] = (unsigned int  )15;
    sqlstm.sqhsts[0] = (         int  )0;
    sqlstm.sqindv[0] = (         void  *)0;
    sqlstm.sqinds[0] = (         int  )0;
    sqlstm.sqharm[0] = (unsigned int  )0;
    sqlstm.sqadto[0] = (unsigned short )0;
    sqlstm.sqtdso[0] = (unsigned short )0;
    sqlstm.sqphsv = sqlstm.sqhstv;
    sqlstm.sqphsl = sqlstm.sqhstl;
    sqlstm.sqphss = sqlstm.sqhsts;
    sqlstm.sqpind = sqlstm.sqindv;
    sqlstm.sqpins = sqlstm.sqinds;
    sqlstm.sqparm = sqlstm.sqharm;
    sqlstm.sqparc = sqlstm.sqharc;
    sqlstm.sqpadto = sqlstm.sqadto;
    sqlstm.sqptdso = sqlstm.sqtdso;
    sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
    if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetManageNumber));
}


  }
  if(pLogStream) *pLogStream << oldStudyUid << " create a new image manage number: " << imageManageNumber << endl;

ReturnOutputString:
  strncpy(outImageManageNum, imageManageNumber, sizeof(imageManageNumber));
  return TRUE;
}

bool insertImageInfoToDB(PImgDataset pimg)
{
  if( ! (connected ? true : connectDicomDB()) ) return false;
  /* EXEC SQL  WHENEVER SQLERROR DO return( SqlError( BRIDGE_InsertImageInfoToDB ) ); */ 


  strncpy(paramHddRoot, pimg->pHddRoot, sizeof(paramHddRoot)); paramHddRoot[sizeof(paramHddRoot) - 1] = '\0';
  strncpy(paramPath, pimg->pPath, sizeof(paramPath)); paramPath[sizeof(paramPath) - 1] = '\0';
  strncpy(paramImgManageNum, pimg->pImgManageNum, sizeof(paramImgManageNum)); paramImgManageNum[sizeof(paramImgManageNum) - 1] = '\0';
  paramFileDate = pimg->fileDate;
  paramFileTime = pimg->fileTime;
  paramInsertDate = pimg->insertDate;
  paramInsertTime = pimg->insertTime;
  paramFileSize = pimg->fileSize.QuadPart;
  strncpy(paramTransferSyntaxUid, pimg->pTransferSyntaxUid, sizeof(paramTransferSyntaxUid)); paramTransferSyntaxUid[sizeof(paramTransferSyntaxUid) - 1] = '\0';
  strncpy(paramSopClaUid, pimg->pSopClaUid, sizeof(paramSopClaUid)); paramSopClaUid[sizeof(paramSopClaUid) - 1] = '\0';
  strncpy(paramSopInsUid, pimg->pSopInsUid, sizeof(paramSopInsUid)); paramSopInsUid[sizeof(paramSopInsUid) - 1] = '\0';
  paramStuDat = pimg->stuDat;
  paramSerDat = pimg->serDat;
  paramStuTim = pimg->stuTim;
  paramSerTim = pimg->serTim;
  strncpy(paramAccNum, pimg->pAccNum, sizeof(paramAccNum)); paramAccNum[sizeof(paramAccNum) - 1] = '\0';
  strncpy(paramSeriesModality, pimg->pSeriesModality, sizeof(paramSeriesModality)); paramSeriesModality[sizeof(paramSeriesModality) - 1] = '\0';
  strncpy(paramStudyModality, pimg->pStudyModality, sizeof(paramStudyModality)); paramStudyModality[sizeof(paramStudyModality) - 1] = '\0';
  strncpy(paramManufacturer, pimg->pManufacturer, sizeof(paramManufacturer)); paramManufacturer[sizeof(paramManufacturer) - 1] = '\0';
  strncpy(paramInstNam, pimg->pInstNam, sizeof(paramInstNam)); paramInstNam[sizeof(paramInstNam) - 1] = '\0';
  strncpy(paramRefPhyNam, pimg->pRefPhyNam, sizeof(paramRefPhyNam)); paramRefPhyNam[sizeof(paramRefPhyNam) - 1] = '\0';
  strncpy(paramStationName, pimg->pStationName, sizeof(paramStationName)); paramStationName[sizeof(paramStationName) - 1] = '\0';
  strncpy(paramStuDes, pimg->pStuDes, sizeof(paramStuDes)); paramStuDes[sizeof(paramStuDes) - 1] = '\0';
  strncpy(paramSerDes, pimg->pSerDes, sizeof(paramSerDes)); paramSerDes[sizeof(paramSerDes) - 1] = '\0';
  strncpy(paramInstDepartName, pimg->pInstDepartName, sizeof(paramInstDepartName)); paramInstDepartName[sizeof(paramInstDepartName) - 1] = '\0';
  strncpy(paramPhyRecNam, pimg->pPhyRecNam, sizeof(paramPhyRecNam)); paramPhyRecNam[sizeof(paramPhyRecNam) - 1] = '\0';
  strncpy(paramPerformPhyName, pimg->pPerformPhyName, sizeof(paramPerformPhyName)); paramPerformPhyName[sizeof(paramPerformPhyName) - 1] = '\0';
  strncpy(paramPhyReadStuName, pimg->pPhyReadStuName, sizeof(paramPhyReadStuName)); paramPhyReadStuName[sizeof(paramPhyReadStuName) - 1] = '\0';
  strncpy(paramOperateName, pimg->pOperateName, sizeof(paramOperateName)); paramOperateName[sizeof(paramOperateName) - 1] = '\0';
  strncpy(paramManufactModNam, pimg->pManufactModNam, sizeof(paramManufactModNam)); paramManufactModNam[sizeof(paramManufactModNam) - 1] = '\0';
  strncpy(paramPatNam, pimg->pPatNam, sizeof(paramPatNam)); paramPatNam[sizeof(paramPatNam) - 1] = '\0';
  strncpy(paramPatNamKan, pimg->pPatNamKan, sizeof(paramPatNamKan)); paramPatNamKan[sizeof(paramPatNamKan) - 1] = '\0';
  strncpy(paramPatNamKat, pimg->pPatNamKat, sizeof(paramPatNamKat)); paramPatNamKat[sizeof(paramPatNamKat) - 1] = '\0';
  strncpy(paramPatId, pimg->pPatId, sizeof(paramPatId)); paramPatId[sizeof(paramPatId) - 1] = '\0';
  paramPatBirDat = pimg->patBirDat;
  strncpy(paramPatSex, pimg->pPatSex, sizeof(paramPatSex)); paramPatSex[sizeof(paramPatSex) - 1] = '\0';
  strncpy(paramPatAge, pimg->pPatAge, sizeof(paramPatAge)); paramPatAge[sizeof(paramPatAge) - 1] = '\0';
  strncpy(paramPatSiz, pimg->pPatSiz, sizeof(paramPatSiz)); paramPatSiz[sizeof(paramPatSiz) - 1] = '\0';
  strncpy(paramPatWei, pimg->pPatWei, sizeof(paramPatWei)); paramPatWei[sizeof(paramPatWei) - 1] = '\0';
  strncpy(paramContBolus, pimg->pContBolus, sizeof(paramContBolus)); paramContBolus[sizeof(paramContBolus) - 1] = '\0';
  strncpy(paramBodParExam, pimg->pBodParExam, sizeof(paramBodParExam)); paramBodParExam[sizeof(paramBodParExam) - 1] = '\0';
  strncpy(paramProtocolNam, pimg->pProtocolNam, sizeof(paramProtocolNam)); paramProtocolNam[sizeof(paramProtocolNam) - 1] = '\0';
  strncpy(paramPatPos, pimg->pPatPos, sizeof(paramPatPos)); paramPatPos[sizeof(paramPatPos) - 1] = '\0';
  strncpy(paramViewPos, pimg->pViewPos, sizeof(paramViewPos)); paramViewPos[sizeof(paramViewPos) - 1] = '\0';
  strncpy(paramStuInsUid, pimg->pStuInsUid, sizeof(paramStuInsUid)); paramStuInsUid[sizeof(paramStuInsUid) - 1] = '\0';
  strncpy(paramSerInsUid, pimg->pSerInsUid, sizeof(paramSerInsUid)); paramSerInsUid[sizeof(paramSerInsUid) - 1] = '\0';
  strncpy(paramStuId, pimg->pStuId, sizeof(paramStuId)); paramStuId[sizeof(paramStuId) - 1] = '\0';
  strncpy(paramSerNum, pimg->pSerNum, sizeof(paramSerNum)); paramSerNum[sizeof(paramSerNum) - 1] = '\0';
  strncpy(paramImaNum, pimg->pImaNum, sizeof(paramImaNum)); paramImaNum[sizeof(paramImaNum) - 1] = '\0';
  strncpy(paramReqPhysician, pimg->pReqPhysician, sizeof(paramReqPhysician)); paramReqPhysician[sizeof(paramReqPhysician) - 1] = '\0';
  strncpy(paramReqService, pimg->pReqService, sizeof(paramReqService)); paramReqService[sizeof(paramReqService) - 1] = '\0';

  /* EXEC SQL EXECUTE BEGIN :packageId := INSERTIMAGE(
    paramHddRoot => :paramHddRoot,
    paramPath => :paramPath,
    paramImgManageNum => :paramImgManageNum,
    paramFileDate => :paramFileDate,
    paramFileTime => :paramFileTime,
    paramInsertDate => :paramInsertDate,
    paramInsertTime => :paramInsertTime,
    paramFileSize => :paramFileSize,
    paramTransferSyntaxUid => :paramTransferSyntaxUid,
    paramSopClaUid => :paramSopClaUid,
    paramSopInsUid => :paramSopInsUid,
    paramStuDat => :paramStuDat,
    paramSerDat => :paramSerDat,
    paramStuTim => :paramStuTim,
    paramSerTim => :paramSerTim,
    paramAccNum => :paramAccNum,
    paramSeriesModality => :paramSeriesModality,
    paramStudyModality => :paramStudyModality,
    paramManufacturer => :paramManufacturer,
    paramInstNam => :paramInstNam,
    paramRefPhyNam => :paramRefPhyNam,
    paramStationName => :paramStationName,
    paramStuDes => :paramStuDes,
    paramSerDes => :paramSerDes,
    paramInstDepartName => :paramInstDepartName,
    paramPhyRecNam => :paramPhyRecNam,
    paramPerformPhyName => :paramPerformPhyName,
    paramPhyReadStuName => :paramPhyReadStuName,
    paramOperateName => :paramOperateName,
    paramManufactModNam => :paramManufactModNam,
    paramPatNam => :paramPatNam,
    paramPatNamKan => :paramPatNamKan,
    paramPatNamKat => :paramPatNamKat,
    paramPatId => :paramPatId,
    paramPatBirDat => :paramPatBirDat,
    paramPatSex => :paramPatSex,
    paramPatAge => :paramPatAge,
    paramPatSiz => :paramPatSiz,
    paramPatWei => :paramPatWei,
    paramContBolus => :paramContBolus,
    paramBodParExam => :paramBodParExam,
    paramProtocolNam => :paramProtocolNam,
    paramPatPos => :paramPatPos,
    paramViewPos => :paramViewPos,
    paramStuInsUid => :paramStuInsUid,
    paramSerInsUid => :paramSerInsUid,
    paramStuId => :paramStuId,
    paramSerNum => :paramSerNum,
    paramImaNum => :paramImaNum,
    paramReqPhysician => :paramReqPhysician,
    paramReqService => :paramReqService
    ); END; END-EXEC; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 52;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlbuft((void **)0,
    "begin :packageId := INSERTIMAGE ( paramHddRoot => :paramHddRoot , param\
Path => :paramPath , paramImgManageNum => :paramImgManageNum , paramFileDate\
 => :paramFileDate , paramFileTime => :paramFileTime , paramInsertDate => :p\
aramInsertDate , paramInsertTime => :paramInsertTime , paramFileSize => :par\
amFileSize , paramTransferSyntaxUid => :paramTransferSyntaxUid , paramSopCla\
Uid => :paramSopClaUid , paramSopInsUid => :paramSopInsUid , paramStuDat => \
:paramStuDat , paramSerDat => :paramSerDat , paramStuTim => :paramStuTim , p\
aramSerTim => :paramSerTim , paramAccNum => :paramAccNum , paramSeriesModali\
ty => :paramSeriesModality , paramStudyModality => :paramStudyModality , par\
amManufacturer => :paramManufacturer , paramInstNam => :paramInstNam , param\
RefPhyNam => :paramRefPhyNam , paramStationName => :paramStationName , param\
StuDes => :paramStuDes , paramSerDes => :paramSerDes , paramInstDepartName =\
> :paramInstDepartName , paramPhyRecNam => :paramPhyRecNam , paramPerformPhy\
Name => :paramPerformPhyName , paramPhyRe");
  sqlstm.stmt = "adStuName => :paramPhyReadStuName , paramOperateName => :p\
aramOperateName , paramManufactModNam => :paramManufactModNam , paramPatNam =\
> :paramPatNam , paramPatNamKan => :paramPatNamKan , paramPatNamKat => :param\
PatNamKat , paramPatId => :paramPatId , paramPatBirDat => :paramPatBirDat , p\
aramPatSex => :paramPatSex , paramPatAge => :paramPatAge , paramPatSiz => :pa\
ramPatSiz , paramPatWei => :paramPatWei , paramContBolus => :paramContBolus ,\
 paramBodParExam => :paramBodParExam , paramProtocolNam => :paramProtocolNam \
, paramPatPos => :paramPatPos , paramViewPos => :paramViewPos , paramStuInsUi\
d => :paramStuInsUid , paramSerInsUid => :paramSerInsUid , paramStuId => :par\
amStuId , paramSerNum => :paramSerNum , paramImaNum => :paramImaNum , paramRe\
qPhysician => :paramReqPhysician , paramReqService => :paramReqService ) ; EN\
D ;";
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )229;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlstm.sqhstv[0] = (         void  *)&packageId;
  sqlstm.sqhstl[0] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[0] = (         int  )0;
  sqlstm.sqindv[0] = (         void  *)0;
  sqlstm.sqinds[0] = (         int  )0;
  sqlstm.sqharm[0] = (unsigned int  )0;
  sqlstm.sqadto[0] = (unsigned short )0;
  sqlstm.sqtdso[0] = (unsigned short )0;
  sqlstm.sqhstv[1] = (         void  *)paramHddRoot;
  sqlstm.sqhstl[1] = (unsigned int  )65;
  sqlstm.sqhsts[1] = (         int  )0;
  sqlstm.sqindv[1] = (         void  *)0;
  sqlstm.sqinds[1] = (         int  )0;
  sqlstm.sqharm[1] = (unsigned int  )0;
  sqlstm.sqadto[1] = (unsigned short )0;
  sqlstm.sqtdso[1] = (unsigned short )0;
  sqlstm.sqhstv[2] = (         void  *)paramPath;
  sqlstm.sqhstl[2] = (unsigned int  )257;
  sqlstm.sqhsts[2] = (         int  )0;
  sqlstm.sqindv[2] = (         void  *)0;
  sqlstm.sqinds[2] = (         int  )0;
  sqlstm.sqharm[2] = (unsigned int  )0;
  sqlstm.sqadto[2] = (unsigned short )0;
  sqlstm.sqtdso[2] = (unsigned short )0;
  sqlstm.sqhstv[3] = (         void  *)paramImgManageNum;
  sqlstm.sqhstl[3] = (unsigned int  )15;
  sqlstm.sqhsts[3] = (         int  )0;
  sqlstm.sqindv[3] = (         void  *)0;
  sqlstm.sqinds[3] = (         int  )0;
  sqlstm.sqharm[3] = (unsigned int  )0;
  sqlstm.sqadto[3] = (unsigned short )0;
  sqlstm.sqtdso[3] = (unsigned short )0;
  sqlstm.sqhstv[4] = (         void  *)&paramFileDate;
  sqlstm.sqhstl[4] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[4] = (         int  )0;
  sqlstm.sqindv[4] = (         void  *)0;
  sqlstm.sqinds[4] = (         int  )0;
  sqlstm.sqharm[4] = (unsigned int  )0;
  sqlstm.sqadto[4] = (unsigned short )0;
  sqlstm.sqtdso[4] = (unsigned short )0;
  sqlstm.sqhstv[5] = (         void  *)&paramFileTime;
  sqlstm.sqhstl[5] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[5] = (         int  )0;
  sqlstm.sqindv[5] = (         void  *)0;
  sqlstm.sqinds[5] = (         int  )0;
  sqlstm.sqharm[5] = (unsigned int  )0;
  sqlstm.sqadto[5] = (unsigned short )0;
  sqlstm.sqtdso[5] = (unsigned short )0;
  sqlstm.sqhstv[6] = (         void  *)&paramInsertDate;
  sqlstm.sqhstl[6] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[6] = (         int  )0;
  sqlstm.sqindv[6] = (         void  *)0;
  sqlstm.sqinds[6] = (         int  )0;
  sqlstm.sqharm[6] = (unsigned int  )0;
  sqlstm.sqadto[6] = (unsigned short )0;
  sqlstm.sqtdso[6] = (unsigned short )0;
  sqlstm.sqhstv[7] = (         void  *)&paramInsertTime;
  sqlstm.sqhstl[7] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[7] = (         int  )0;
  sqlstm.sqindv[7] = (         void  *)0;
  sqlstm.sqinds[7] = (         int  )0;
  sqlstm.sqharm[7] = (unsigned int  )0;
  sqlstm.sqadto[7] = (unsigned short )0;
  sqlstm.sqtdso[7] = (unsigned short )0;
  sqlstm.sqhstv[8] = (         void  *)&paramFileSize;
  sqlstm.sqhstl[8] = (unsigned int  )sizeof(long);
  sqlstm.sqhsts[8] = (         int  )0;
  sqlstm.sqindv[8] = (         void  *)0;
  sqlstm.sqinds[8] = (         int  )0;
  sqlstm.sqharm[8] = (unsigned int  )0;
  sqlstm.sqadto[8] = (unsigned short )0;
  sqlstm.sqtdso[8] = (unsigned short )0;
  sqlstm.sqhstv[9] = (         void  *)paramTransferSyntaxUid;
  sqlstm.sqhstl[9] = (unsigned int  )65;
  sqlstm.sqhsts[9] = (         int  )0;
  sqlstm.sqindv[9] = (         void  *)0;
  sqlstm.sqinds[9] = (         int  )0;
  sqlstm.sqharm[9] = (unsigned int  )0;
  sqlstm.sqadto[9] = (unsigned short )0;
  sqlstm.sqtdso[9] = (unsigned short )0;
  sqlstm.sqhstv[10] = (         void  *)paramSopClaUid;
  sqlstm.sqhstl[10] = (unsigned int  )65;
  sqlstm.sqhsts[10] = (         int  )0;
  sqlstm.sqindv[10] = (         void  *)0;
  sqlstm.sqinds[10] = (         int  )0;
  sqlstm.sqharm[10] = (unsigned int  )0;
  sqlstm.sqadto[10] = (unsigned short )0;
  sqlstm.sqtdso[10] = (unsigned short )0;
  sqlstm.sqhstv[11] = (         void  *)paramSopInsUid;
  sqlstm.sqhstl[11] = (unsigned int  )65;
  sqlstm.sqhsts[11] = (         int  )0;
  sqlstm.sqindv[11] = (         void  *)0;
  sqlstm.sqinds[11] = (         int  )0;
  sqlstm.sqharm[11] = (unsigned int  )0;
  sqlstm.sqadto[11] = (unsigned short )0;
  sqlstm.sqtdso[11] = (unsigned short )0;
  sqlstm.sqhstv[12] = (         void  *)&paramStuDat;
  sqlstm.sqhstl[12] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[12] = (         int  )0;
  sqlstm.sqindv[12] = (         void  *)0;
  sqlstm.sqinds[12] = (         int  )0;
  sqlstm.sqharm[12] = (unsigned int  )0;
  sqlstm.sqadto[12] = (unsigned short )0;
  sqlstm.sqtdso[12] = (unsigned short )0;
  sqlstm.sqhstv[13] = (         void  *)&paramSerDat;
  sqlstm.sqhstl[13] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[13] = (         int  )0;
  sqlstm.sqindv[13] = (         void  *)0;
  sqlstm.sqinds[13] = (         int  )0;
  sqlstm.sqharm[13] = (unsigned int  )0;
  sqlstm.sqadto[13] = (unsigned short )0;
  sqlstm.sqtdso[13] = (unsigned short )0;
  sqlstm.sqhstv[14] = (         void  *)&paramStuTim;
  sqlstm.sqhstl[14] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[14] = (         int  )0;
  sqlstm.sqindv[14] = (         void  *)0;
  sqlstm.sqinds[14] = (         int  )0;
  sqlstm.sqharm[14] = (unsigned int  )0;
  sqlstm.sqadto[14] = (unsigned short )0;
  sqlstm.sqtdso[14] = (unsigned short )0;
  sqlstm.sqhstv[15] = (         void  *)&paramSerTim;
  sqlstm.sqhstl[15] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[15] = (         int  )0;
  sqlstm.sqindv[15] = (         void  *)0;
  sqlstm.sqinds[15] = (         int  )0;
  sqlstm.sqharm[15] = (unsigned int  )0;
  sqlstm.sqadto[15] = (unsigned short )0;
  sqlstm.sqtdso[15] = (unsigned short )0;
  sqlstm.sqhstv[16] = (         void  *)paramAccNum;
  sqlstm.sqhstl[16] = (unsigned int  )17;
  sqlstm.sqhsts[16] = (         int  )0;
  sqlstm.sqindv[16] = (         void  *)0;
  sqlstm.sqinds[16] = (         int  )0;
  sqlstm.sqharm[16] = (unsigned int  )0;
  sqlstm.sqadto[16] = (unsigned short )0;
  sqlstm.sqtdso[16] = (unsigned short )0;
  sqlstm.sqhstv[17] = (         void  *)paramSeriesModality;
  sqlstm.sqhstl[17] = (unsigned int  )17;
  sqlstm.sqhsts[17] = (         int  )0;
  sqlstm.sqindv[17] = (         void  *)0;
  sqlstm.sqinds[17] = (         int  )0;
  sqlstm.sqharm[17] = (unsigned int  )0;
  sqlstm.sqadto[17] = (unsigned short )0;
  sqlstm.sqtdso[17] = (unsigned short )0;
  sqlstm.sqhstv[18] = (         void  *)paramStudyModality;
  sqlstm.sqhstl[18] = (unsigned int  )65;
  sqlstm.sqhsts[18] = (         int  )0;
  sqlstm.sqindv[18] = (         void  *)0;
  sqlstm.sqinds[18] = (         int  )0;
  sqlstm.sqharm[18] = (unsigned int  )0;
  sqlstm.sqadto[18] = (unsigned short )0;
  sqlstm.sqtdso[18] = (unsigned short )0;
  sqlstm.sqhstv[19] = (         void  *)paramManufacturer;
  sqlstm.sqhstl[19] = (unsigned int  )65;
  sqlstm.sqhsts[19] = (         int  )0;
  sqlstm.sqindv[19] = (         void  *)0;
  sqlstm.sqinds[19] = (         int  )0;
  sqlstm.sqharm[19] = (unsigned int  )0;
  sqlstm.sqadto[19] = (unsigned short )0;
  sqlstm.sqtdso[19] = (unsigned short )0;
  sqlstm.sqhstv[20] = (         void  *)paramInstNam;
  sqlstm.sqhstl[20] = (unsigned int  )65;
  sqlstm.sqhsts[20] = (         int  )0;
  sqlstm.sqindv[20] = (         void  *)0;
  sqlstm.sqinds[20] = (         int  )0;
  sqlstm.sqharm[20] = (unsigned int  )0;
  sqlstm.sqadto[20] = (unsigned short )0;
  sqlstm.sqtdso[20] = (unsigned short )0;
  sqlstm.sqhstv[21] = (         void  *)paramRefPhyNam;
  sqlstm.sqhstl[21] = (unsigned int  )65;
  sqlstm.sqhsts[21] = (         int  )0;
  sqlstm.sqindv[21] = (         void  *)0;
  sqlstm.sqinds[21] = (         int  )0;
  sqlstm.sqharm[21] = (unsigned int  )0;
  sqlstm.sqadto[21] = (unsigned short )0;
  sqlstm.sqtdso[21] = (unsigned short )0;
  sqlstm.sqhstv[22] = (         void  *)paramStationName;
  sqlstm.sqhstl[22] = (unsigned int  )17;
  sqlstm.sqhsts[22] = (         int  )0;
  sqlstm.sqindv[22] = (         void  *)0;
  sqlstm.sqinds[22] = (         int  )0;
  sqlstm.sqharm[22] = (unsigned int  )0;
  sqlstm.sqadto[22] = (unsigned short )0;
  sqlstm.sqtdso[22] = (unsigned short )0;
  sqlstm.sqhstv[23] = (         void  *)paramStuDes;
  sqlstm.sqhstl[23] = (unsigned int  )65;
  sqlstm.sqhsts[23] = (         int  )0;
  sqlstm.sqindv[23] = (         void  *)0;
  sqlstm.sqinds[23] = (         int  )0;
  sqlstm.sqharm[23] = (unsigned int  )0;
  sqlstm.sqadto[23] = (unsigned short )0;
  sqlstm.sqtdso[23] = (unsigned short )0;
  sqlstm.sqhstv[24] = (         void  *)paramSerDes;
  sqlstm.sqhstl[24] = (unsigned int  )65;
  sqlstm.sqhsts[24] = (         int  )0;
  sqlstm.sqindv[24] = (         void  *)0;
  sqlstm.sqinds[24] = (         int  )0;
  sqlstm.sqharm[24] = (unsigned int  )0;
  sqlstm.sqadto[24] = (unsigned short )0;
  sqlstm.sqtdso[24] = (unsigned short )0;
  sqlstm.sqhstv[25] = (         void  *)paramInstDepartName;
  sqlstm.sqhstl[25] = (unsigned int  )65;
  sqlstm.sqhsts[25] = (         int  )0;
  sqlstm.sqindv[25] = (         void  *)0;
  sqlstm.sqinds[25] = (         int  )0;
  sqlstm.sqharm[25] = (unsigned int  )0;
  sqlstm.sqadto[25] = (unsigned short )0;
  sqlstm.sqtdso[25] = (unsigned short )0;
  sqlstm.sqhstv[26] = (         void  *)paramPhyRecNam;
  sqlstm.sqhstl[26] = (unsigned int  )65;
  sqlstm.sqhsts[26] = (         int  )0;
  sqlstm.sqindv[26] = (         void  *)0;
  sqlstm.sqinds[26] = (         int  )0;
  sqlstm.sqharm[26] = (unsigned int  )0;
  sqlstm.sqadto[26] = (unsigned short )0;
  sqlstm.sqtdso[26] = (unsigned short )0;
  sqlstm.sqhstv[27] = (         void  *)paramPerformPhyName;
  sqlstm.sqhstl[27] = (unsigned int  )65;
  sqlstm.sqhsts[27] = (         int  )0;
  sqlstm.sqindv[27] = (         void  *)0;
  sqlstm.sqinds[27] = (         int  )0;
  sqlstm.sqharm[27] = (unsigned int  )0;
  sqlstm.sqadto[27] = (unsigned short )0;
  sqlstm.sqtdso[27] = (unsigned short )0;
  sqlstm.sqhstv[28] = (         void  *)paramPhyReadStuName;
  sqlstm.sqhstl[28] = (unsigned int  )65;
  sqlstm.sqhsts[28] = (         int  )0;
  sqlstm.sqindv[28] = (         void  *)0;
  sqlstm.sqinds[28] = (         int  )0;
  sqlstm.sqharm[28] = (unsigned int  )0;
  sqlstm.sqadto[28] = (unsigned short )0;
  sqlstm.sqtdso[28] = (unsigned short )0;
  sqlstm.sqhstv[29] = (         void  *)paramOperateName;
  sqlstm.sqhstl[29] = (unsigned int  )65;
  sqlstm.sqhsts[29] = (         int  )0;
  sqlstm.sqindv[29] = (         void  *)0;
  sqlstm.sqinds[29] = (         int  )0;
  sqlstm.sqharm[29] = (unsigned int  )0;
  sqlstm.sqadto[29] = (unsigned short )0;
  sqlstm.sqtdso[29] = (unsigned short )0;
  sqlstm.sqhstv[30] = (         void  *)paramManufactModNam;
  sqlstm.sqhstl[30] = (unsigned int  )65;
  sqlstm.sqhsts[30] = (         int  )0;
  sqlstm.sqindv[30] = (         void  *)0;
  sqlstm.sqinds[30] = (         int  )0;
  sqlstm.sqharm[30] = (unsigned int  )0;
  sqlstm.sqadto[30] = (unsigned short )0;
  sqlstm.sqtdso[30] = (unsigned short )0;
  sqlstm.sqhstv[31] = (         void  *)paramPatNam;
  sqlstm.sqhstl[31] = (unsigned int  )65;
  sqlstm.sqhsts[31] = (         int  )0;
  sqlstm.sqindv[31] = (         void  *)0;
  sqlstm.sqinds[31] = (         int  )0;
  sqlstm.sqharm[31] = (unsigned int  )0;
  sqlstm.sqadto[31] = (unsigned short )0;
  sqlstm.sqtdso[31] = (unsigned short )0;
  sqlstm.sqhstv[32] = (         void  *)paramPatNamKan;
  sqlstm.sqhstl[32] = (unsigned int  )65;
  sqlstm.sqhsts[32] = (         int  )0;
  sqlstm.sqindv[32] = (         void  *)0;
  sqlstm.sqinds[32] = (         int  )0;
  sqlstm.sqharm[32] = (unsigned int  )0;
  sqlstm.sqadto[32] = (unsigned short )0;
  sqlstm.sqtdso[32] = (unsigned short )0;
  sqlstm.sqhstv[33] = (         void  *)paramPatNamKat;
  sqlstm.sqhstl[33] = (unsigned int  )65;
  sqlstm.sqhsts[33] = (         int  )0;
  sqlstm.sqindv[33] = (         void  *)0;
  sqlstm.sqinds[33] = (         int  )0;
  sqlstm.sqharm[33] = (unsigned int  )0;
  sqlstm.sqadto[33] = (unsigned short )0;
  sqlstm.sqtdso[33] = (unsigned short )0;
  sqlstm.sqhstv[34] = (         void  *)paramPatId;
  sqlstm.sqhstl[34] = (unsigned int  )65;
  sqlstm.sqhsts[34] = (         int  )0;
  sqlstm.sqindv[34] = (         void  *)0;
  sqlstm.sqinds[34] = (         int  )0;
  sqlstm.sqharm[34] = (unsigned int  )0;
  sqlstm.sqadto[34] = (unsigned short )0;
  sqlstm.sqtdso[34] = (unsigned short )0;
  sqlstm.sqhstv[35] = (         void  *)&paramPatBirDat;
  sqlstm.sqhstl[35] = (unsigned int  )sizeof(int);
  sqlstm.sqhsts[35] = (         int  )0;
  sqlstm.sqindv[35] = (         void  *)0;
  sqlstm.sqinds[35] = (         int  )0;
  sqlstm.sqharm[35] = (unsigned int  )0;
  sqlstm.sqadto[35] = (unsigned short )0;
  sqlstm.sqtdso[35] = (unsigned short )0;
  sqlstm.sqhstv[36] = (         void  *)paramPatSex;
  sqlstm.sqhstl[36] = (unsigned int  )17;
  sqlstm.sqhsts[36] = (         int  )0;
  sqlstm.sqindv[36] = (         void  *)0;
  sqlstm.sqinds[36] = (         int  )0;
  sqlstm.sqharm[36] = (unsigned int  )0;
  sqlstm.sqadto[36] = (unsigned short )0;
  sqlstm.sqtdso[36] = (unsigned short )0;
  sqlstm.sqhstv[37] = (         void  *)paramPatAge;
  sqlstm.sqhstl[37] = (unsigned int  )5;
  sqlstm.sqhsts[37] = (         int  )0;
  sqlstm.sqindv[37] = (         void  *)0;
  sqlstm.sqinds[37] = (         int  )0;
  sqlstm.sqharm[37] = (unsigned int  )0;
  sqlstm.sqadto[37] = (unsigned short )0;
  sqlstm.sqtdso[37] = (unsigned short )0;
  sqlstm.sqhstv[38] = (         void  *)paramPatSiz;
  sqlstm.sqhstl[38] = (unsigned int  )17;
  sqlstm.sqhsts[38] = (         int  )0;
  sqlstm.sqindv[38] = (         void  *)0;
  sqlstm.sqinds[38] = (         int  )0;
  sqlstm.sqharm[38] = (unsigned int  )0;
  sqlstm.sqadto[38] = (unsigned short )0;
  sqlstm.sqtdso[38] = (unsigned short )0;
  sqlstm.sqhstv[39] = (         void  *)paramPatWei;
  sqlstm.sqhstl[39] = (unsigned int  )17;
  sqlstm.sqhsts[39] = (         int  )0;
  sqlstm.sqindv[39] = (         void  *)0;
  sqlstm.sqinds[39] = (         int  )0;
  sqlstm.sqharm[39] = (unsigned int  )0;
  sqlstm.sqadto[39] = (unsigned short )0;
  sqlstm.sqtdso[39] = (unsigned short )0;
  sqlstm.sqhstv[40] = (         void  *)paramContBolus;
  sqlstm.sqhstl[40] = (unsigned int  )65;
  sqlstm.sqhsts[40] = (         int  )0;
  sqlstm.sqindv[40] = (         void  *)0;
  sqlstm.sqinds[40] = (         int  )0;
  sqlstm.sqharm[40] = (unsigned int  )0;
  sqlstm.sqadto[40] = (unsigned short )0;
  sqlstm.sqtdso[40] = (unsigned short )0;
  sqlstm.sqhstv[41] = (         void  *)paramBodParExam;
  sqlstm.sqhstl[41] = (unsigned int  )17;
  sqlstm.sqhsts[41] = (         int  )0;
  sqlstm.sqindv[41] = (         void  *)0;
  sqlstm.sqinds[41] = (         int  )0;
  sqlstm.sqharm[41] = (unsigned int  )0;
  sqlstm.sqadto[41] = (unsigned short )0;
  sqlstm.sqtdso[41] = (unsigned short )0;
  sqlstm.sqhstv[42] = (         void  *)paramProtocolNam;
  sqlstm.sqhstl[42] = (unsigned int  )65;
  sqlstm.sqhsts[42] = (         int  )0;
  sqlstm.sqindv[42] = (         void  *)0;
  sqlstm.sqinds[42] = (         int  )0;
  sqlstm.sqharm[42] = (unsigned int  )0;
  sqlstm.sqadto[42] = (unsigned short )0;
  sqlstm.sqtdso[42] = (unsigned short )0;
  sqlstm.sqhstv[43] = (         void  *)paramPatPos;
  sqlstm.sqhstl[43] = (unsigned int  )17;
  sqlstm.sqhsts[43] = (         int  )0;
  sqlstm.sqindv[43] = (         void  *)0;
  sqlstm.sqinds[43] = (         int  )0;
  sqlstm.sqharm[43] = (unsigned int  )0;
  sqlstm.sqadto[43] = (unsigned short )0;
  sqlstm.sqtdso[43] = (unsigned short )0;
  sqlstm.sqhstv[44] = (         void  *)paramViewPos;
  sqlstm.sqhstl[44] = (unsigned int  )17;
  sqlstm.sqhsts[44] = (         int  )0;
  sqlstm.sqindv[44] = (         void  *)0;
  sqlstm.sqinds[44] = (         int  )0;
  sqlstm.sqharm[44] = (unsigned int  )0;
  sqlstm.sqadto[44] = (unsigned short )0;
  sqlstm.sqtdso[44] = (unsigned short )0;
  sqlstm.sqhstv[45] = (         void  *)paramStuInsUid;
  sqlstm.sqhstl[45] = (unsigned int  )65;
  sqlstm.sqhsts[45] = (         int  )0;
  sqlstm.sqindv[45] = (         void  *)0;
  sqlstm.sqinds[45] = (         int  )0;
  sqlstm.sqharm[45] = (unsigned int  )0;
  sqlstm.sqadto[45] = (unsigned short )0;
  sqlstm.sqtdso[45] = (unsigned short )0;
  sqlstm.sqhstv[46] = (         void  *)paramSerInsUid;
  sqlstm.sqhstl[46] = (unsigned int  )65;
  sqlstm.sqhsts[46] = (         int  )0;
  sqlstm.sqindv[46] = (         void  *)0;
  sqlstm.sqinds[46] = (         int  )0;
  sqlstm.sqharm[46] = (unsigned int  )0;
  sqlstm.sqadto[46] = (unsigned short )0;
  sqlstm.sqtdso[46] = (unsigned short )0;
  sqlstm.sqhstv[47] = (         void  *)paramStuId;
  sqlstm.sqhstl[47] = (unsigned int  )17;
  sqlstm.sqhsts[47] = (         int  )0;
  sqlstm.sqindv[47] = (         void  *)0;
  sqlstm.sqinds[47] = (         int  )0;
  sqlstm.sqharm[47] = (unsigned int  )0;
  sqlstm.sqadto[47] = (unsigned short )0;
  sqlstm.sqtdso[47] = (unsigned short )0;
  sqlstm.sqhstv[48] = (         void  *)paramSerNum;
  sqlstm.sqhstl[48] = (unsigned int  )13;
  sqlstm.sqhsts[48] = (         int  )0;
  sqlstm.sqindv[48] = (         void  *)0;
  sqlstm.sqinds[48] = (         int  )0;
  sqlstm.sqharm[48] = (unsigned int  )0;
  sqlstm.sqadto[48] = (unsigned short )0;
  sqlstm.sqtdso[48] = (unsigned short )0;
  sqlstm.sqhstv[49] = (         void  *)paramImaNum;
  sqlstm.sqhstl[49] = (unsigned int  )13;
  sqlstm.sqhsts[49] = (         int  )0;
  sqlstm.sqindv[49] = (         void  *)0;
  sqlstm.sqinds[49] = (         int  )0;
  sqlstm.sqharm[49] = (unsigned int  )0;
  sqlstm.sqadto[49] = (unsigned short )0;
  sqlstm.sqtdso[49] = (unsigned short )0;
  sqlstm.sqhstv[50] = (         void  *)paramReqPhysician;
  sqlstm.sqhstl[50] = (unsigned int  )65;
  sqlstm.sqhsts[50] = (         int  )0;
  sqlstm.sqindv[50] = (         void  *)0;
  sqlstm.sqinds[50] = (         int  )0;
  sqlstm.sqharm[50] = (unsigned int  )0;
  sqlstm.sqadto[50] = (unsigned short )0;
  sqlstm.sqtdso[50] = (unsigned short )0;
  sqlstm.sqhstv[51] = (         void  *)paramReqService;
  sqlstm.sqhstl[51] = (unsigned int  )65;
  sqlstm.sqhsts[51] = (         int  )0;
  sqlstm.sqindv[51] = (         void  *)0;
  sqlstm.sqinds[51] = (         int  )0;
  sqlstm.sqharm[51] = (unsigned int  )0;
  sqlstm.sqadto[51] = (unsigned short )0;
  sqlstm.sqtdso[51] = (unsigned short )0;
  sqlstm.sqphsv = sqlstm.sqhstv;
  sqlstm.sqphsl = sqlstm.sqhstl;
  sqlstm.sqphss = sqlstm.sqhsts;
  sqlstm.sqpind = sqlstm.sqindv;
  sqlstm.sqpins = sqlstm.sqinds;
  sqlstm.sqparm = sqlstm.sqharm;
  sqlstm.sqparc = sqlstm.sqharc;
  sqlstm.sqpadto = sqlstm.sqadto;
  sqlstm.sqptdso = sqlstm.sqtdso;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_InsertImageInfoToDB));
}


  return true;
}

// ---------WLM Condition-----------

bool GetWorklistFromDB(FetchWorklistCallback callback, WlmDBInteractionManager *dbim)
{
  if( ! (connected ? true : connectDicomDB()) ) return false;
  /* EXEC SQL WHENEVER SQLERROR DO return( SqlError( BRIDGE_GetWorklistFromDB ) ); */ 

  /* EXEC SQL WHENEVER NOT FOUND DO break; */ 


  /* EXEC SQL PREPARE S FROM :pWorklistStatement; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 52;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.stmt = "";
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )452;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlstm.sqhstv[0] = (         void  *)pWorklistStatement;
  sqlstm.sqhstl[0] = (unsigned int  )0;
  sqlstm.sqhsts[0] = (         int  )0;
  sqlstm.sqindv[0] = (         void  *)0;
  sqlstm.sqinds[0] = (         int  )0;
  sqlstm.sqharm[0] = (unsigned int  )0;
  sqlstm.sqadto[0] = (unsigned short )0;
  sqlstm.sqtdso[0] = (unsigned short )0;
  sqlstm.sqphsv = sqlstm.sqhstv;
  sqlstm.sqphsl = sqlstm.sqhstl;
  sqlstm.sqphss = sqlstm.sqhsts;
  sqlstm.sqpind = sqlstm.sqindv;
  sqlstm.sqpins = sqlstm.sqinds;
  sqlstm.sqparm = sqlstm.sqharm;
  sqlstm.sqparc = sqlstm.sqharc;
  sqlstm.sqpadto = sqlstm.sqadto;
  sqlstm.sqptdso = sqlstm.sqtdso;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetWorklistFromDB));
}


  /* EXEC SQL DECLARE C CURSOR FOR S; */ 

  /* EXEC SQL OPEN C USING :SearchCondition; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 52;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.stmt = "";
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )471;
  sqlstm.selerr = (unsigned short)1;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlstm.sqcmod = (unsigned int )0;
  sqlstm.sqhstv[0] = (         void  *)SearchCondition.pScheduleStationAE;
  sqlstm.sqhstl[0] = (unsigned int  )0;
  sqlstm.sqhsts[0] = (         int  )0;
  sqlstm.sqindv[0] = (         void  *)0;
  sqlstm.sqinds[0] = (         int  )0;
  sqlstm.sqharm[0] = (unsigned int  )0;
  sqlstm.sqadto[0] = (unsigned short )0;
  sqlstm.sqtdso[0] = (unsigned short )0;
  sqlstm.sqhstv[1] = (         void  *)SearchCondition.pModality;
  sqlstm.sqhstl[1] = (unsigned int  )0;
  sqlstm.sqhsts[1] = (         int  )0;
  sqlstm.sqindv[1] = (         void  *)0;
  sqlstm.sqinds[1] = (         int  )0;
  sqlstm.sqharm[1] = (unsigned int  )0;
  sqlstm.sqadto[1] = (unsigned short )0;
  sqlstm.sqtdso[1] = (unsigned short )0;
  sqlstm.sqhstv[2] = (         void  *)SearchCondition.pLowerScheduleDate;
  sqlstm.sqhstl[2] = (unsigned int  )0;
  sqlstm.sqhsts[2] = (         int  )0;
  sqlstm.sqindv[2] = (         void  *)0;
  sqlstm.sqinds[2] = (         int  )0;
  sqlstm.sqharm[2] = (unsigned int  )0;
  sqlstm.sqadto[2] = (unsigned short )0;
  sqlstm.sqtdso[2] = (unsigned short )0;
  sqlstm.sqhstv[3] = (         void  *)SearchCondition.pUpperScheduleDate;
  sqlstm.sqhstl[3] = (unsigned int  )0;
  sqlstm.sqhsts[3] = (         int  )0;
  sqlstm.sqindv[3] = (         void  *)0;
  sqlstm.sqinds[3] = (         int  )0;
  sqlstm.sqharm[3] = (unsigned int  )0;
  sqlstm.sqadto[3] = (unsigned short )0;
  sqlstm.sqtdso[3] = (unsigned short )0;
  sqlstm.sqphsv = sqlstm.sqhstv;
  sqlstm.sqphsl = sqlstm.sqhstl;
  sqlstm.sqphss = sqlstm.sqhsts;
  sqlstm.sqpind = sqlstm.sqindv;
  sqlstm.sqpins = sqlstm.sqinds;
  sqlstm.sqparm = sqlstm.sqharm;
  sqlstm.sqparc = sqlstm.sqharc;
  sqlstm.sqpadto = sqlstm.sqadto;
  sqlstm.sqptdso = sqlstm.sqtdso;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetWorklistFromDB));
}


  while(true)
  {
	/* EXEC SQL FETCH C INTO :WorklistInBridge INDICATOR :IndicatorWorklist; */ 

{
 struct sqlexd sqlstm;
 sqlstm.sqlvsn = 12;
 sqlstm.arrsiz = 52;
 sqlstm.sqladtp = &sqladt;
 sqlstm.sqltdsp = &sqltds;
 sqlstm.iters = (unsigned int  )1;
 sqlstm.offset = (unsigned int  )502;
 sqlstm.selerr = (unsigned short)1;
 sqlstm.cud = sqlcud0;
 sqlstm.sqlest = (unsigned char  *)&sqlca;
 sqlstm.sqlety = (unsigned short)4352;
 sqlstm.occurs = (unsigned int  )0;
 sqlstm.sqfoff = (           int )0;
 sqlstm.sqfmod = (unsigned int )2;
 sqlstm.sqhstv[0] = (         void  *)WorklistInBridge.ScheduledStationAETitle;
 sqlstm.sqhstl[0] = (unsigned int  )17;
 sqlstm.sqhsts[0] = (         int  )0;
 sqlstm.sqindv[0] = (         void  *)&IndicatorWorklist.ScheduledStationAETitle;
 sqlstm.sqinds[0] = (         int  )0;
 sqlstm.sqharm[0] = (unsigned int  )0;
 sqlstm.sqadto[0] = (unsigned short )0;
 sqlstm.sqtdso[0] = (unsigned short )0;
 sqlstm.sqhstv[1] = (         void  *)WorklistInBridge.SchdldProcStepStartDate;
 sqlstm.sqhstl[1] = (unsigned int  )9;
 sqlstm.sqhsts[1] = (         int  )0;
 sqlstm.sqindv[1] = (         void  *)&IndicatorWorklist.SchdldProcStepStartDate;
 sqlstm.sqinds[1] = (         int  )0;
 sqlstm.sqharm[1] = (unsigned int  )0;
 sqlstm.sqadto[1] = (unsigned short )0;
 sqlstm.sqtdso[1] = (unsigned short )0;
 sqlstm.sqhstv[2] = (         void  *)WorklistInBridge.SchdldProcStepStartTime;
 sqlstm.sqhstl[2] = (unsigned int  )17;
 sqlstm.sqhsts[2] = (         int  )0;
 sqlstm.sqindv[2] = (         void  *)&IndicatorWorklist.SchdldProcStepStartTime;
 sqlstm.sqinds[2] = (         int  )0;
 sqlstm.sqharm[2] = (unsigned int  )0;
 sqlstm.sqadto[2] = (unsigned short )0;
 sqlstm.sqtdso[2] = (unsigned short )0;
 sqlstm.sqhstv[3] = (         void  *)WorklistInBridge.Modality;
 sqlstm.sqhstl[3] = (unsigned int  )17;
 sqlstm.sqhsts[3] = (         int  )0;
 sqlstm.sqindv[3] = (         void  *)&IndicatorWorklist.Modality;
 sqlstm.sqinds[3] = (         int  )0;
 sqlstm.sqharm[3] = (unsigned int  )0;
 sqlstm.sqadto[3] = (unsigned short )0;
 sqlstm.sqtdso[3] = (unsigned short )0;
 sqlstm.sqhstv[4] = (         void  *)WorklistInBridge.SchdldProcStepDescription;
 sqlstm.sqhstl[4] = (unsigned int  )65;
 sqlstm.sqhsts[4] = (         int  )0;
 sqlstm.sqindv[4] = (         void  *)&IndicatorWorklist.SchdldProcStepDescription;
 sqlstm.sqinds[4] = (         int  )0;
 sqlstm.sqharm[4] = (unsigned int  )0;
 sqlstm.sqadto[4] = (unsigned short )0;
 sqlstm.sqtdso[4] = (unsigned short )0;
 sqlstm.sqhstv[5] = (         void  *)WorklistInBridge.SchdldProcStepLocation;
 sqlstm.sqhstl[5] = (unsigned int  )17;
 sqlstm.sqhsts[5] = (         int  )0;
 sqlstm.sqindv[5] = (         void  *)&IndicatorWorklist.SchdldProcStepLocation;
 sqlstm.sqinds[5] = (         int  )0;
 sqlstm.sqharm[5] = (unsigned int  )0;
 sqlstm.sqadto[5] = (unsigned short )0;
 sqlstm.sqtdso[5] = (unsigned short )0;
 sqlstm.sqhstv[6] = (         void  *)WorklistInBridge.SchdldProcStepID;
 sqlstm.sqhstl[6] = (unsigned int  )17;
 sqlstm.sqhsts[6] = (         int  )0;
 sqlstm.sqindv[6] = (         void  *)&IndicatorWorklist.SchdldProcStepID;
 sqlstm.sqinds[6] = (         int  )0;
 sqlstm.sqharm[6] = (unsigned int  )0;
 sqlstm.sqadto[6] = (unsigned short )0;
 sqlstm.sqtdso[6] = (unsigned short )0;
 sqlstm.sqhstv[7] = (         void  *)WorklistInBridge.RequestedProcedureID;
 sqlstm.sqhstl[7] = (unsigned int  )17;
 sqlstm.sqhsts[7] = (         int  )0;
 sqlstm.sqindv[7] = (         void  *)&IndicatorWorklist.RequestedProcedureID;
 sqlstm.sqinds[7] = (         int  )0;
 sqlstm.sqharm[7] = (unsigned int  )0;
 sqlstm.sqadto[7] = (unsigned short )0;
 sqlstm.sqtdso[7] = (unsigned short )0;
 sqlstm.sqhstv[8] = (         void  *)WorklistInBridge.RequestedProcedureDescription;
 sqlstm.sqhstl[8] = (unsigned int  )65;
 sqlstm.sqhsts[8] = (         int  )0;
 sqlstm.sqindv[8] = (         void  *)&IndicatorWorklist.RequestedProcedureDescription;
 sqlstm.sqinds[8] = (         int  )0;
 sqlstm.sqharm[8] = (unsigned int  )0;
 sqlstm.sqadto[8] = (unsigned short )0;
 sqlstm.sqtdso[8] = (unsigned short )0;
 sqlstm.sqhstv[9] = (         void  *)WorklistInBridge.StudyInstanceUID;
 sqlstm.sqhstl[9] = (unsigned int  )65;
 sqlstm.sqhsts[9] = (         int  )0;
 sqlstm.sqindv[9] = (         void  *)&IndicatorWorklist.StudyInstanceUID;
 sqlstm.sqinds[9] = (         int  )0;
 sqlstm.sqharm[9] = (unsigned int  )0;
 sqlstm.sqadto[9] = (unsigned short )0;
 sqlstm.sqtdso[9] = (unsigned short )0;
 sqlstm.sqhstv[10] = (         void  *)WorklistInBridge.AccessionNumber;
 sqlstm.sqhstl[10] = (unsigned int  )17;
 sqlstm.sqhsts[10] = (         int  )0;
 sqlstm.sqindv[10] = (         void  *)&IndicatorWorklist.AccessionNumber;
 sqlstm.sqinds[10] = (         int  )0;
 sqlstm.sqharm[10] = (unsigned int  )0;
 sqlstm.sqadto[10] = (unsigned short )0;
 sqlstm.sqtdso[10] = (unsigned short )0;
 sqlstm.sqhstv[11] = (         void  *)WorklistInBridge.RequestingPhysician;
 sqlstm.sqhstl[11] = (unsigned int  )65;
 sqlstm.sqhsts[11] = (         int  )0;
 sqlstm.sqindv[11] = (         void  *)&IndicatorWorklist.RequestingPhysician;
 sqlstm.sqinds[11] = (         int  )0;
 sqlstm.sqharm[11] = (unsigned int  )0;
 sqlstm.sqadto[11] = (unsigned short )0;
 sqlstm.sqtdso[11] = (unsigned short )0;
 sqlstm.sqhstv[12] = (         void  *)WorklistInBridge.AdmissionID;
 sqlstm.sqhstl[12] = (unsigned int  )65;
 sqlstm.sqhsts[12] = (         int  )0;
 sqlstm.sqindv[12] = (         void  *)&IndicatorWorklist.AdmissionID;
 sqlstm.sqinds[12] = (         int  )0;
 sqlstm.sqharm[12] = (unsigned int  )0;
 sqlstm.sqadto[12] = (unsigned short )0;
 sqlstm.sqtdso[12] = (unsigned short )0;
 sqlstm.sqhstv[13] = (         void  *)WorklistInBridge.PatientsNameEn;
 sqlstm.sqhstl[13] = (unsigned int  )65;
 sqlstm.sqhsts[13] = (         int  )0;
 sqlstm.sqindv[13] = (         void  *)&IndicatorWorklist.PatientsNameEn;
 sqlstm.sqinds[13] = (         int  )0;
 sqlstm.sqharm[13] = (unsigned int  )0;
 sqlstm.sqadto[13] = (unsigned short )0;
 sqlstm.sqtdso[13] = (unsigned short )0;
 sqlstm.sqhstv[14] = (         void  *)WorklistInBridge.PatientsNameCh;
 sqlstm.sqhstl[14] = (unsigned int  )65;
 sqlstm.sqhsts[14] = (         int  )0;
 sqlstm.sqindv[14] = (         void  *)&IndicatorWorklist.PatientsNameCh;
 sqlstm.sqinds[14] = (         int  )0;
 sqlstm.sqharm[14] = (unsigned int  )0;
 sqlstm.sqadto[14] = (unsigned short )0;
 sqlstm.sqtdso[14] = (unsigned short )0;
 sqlstm.sqhstv[15] = (         void  *)WorklistInBridge.PatientID;
 sqlstm.sqhstl[15] = (unsigned int  )65;
 sqlstm.sqhsts[15] = (         int  )0;
 sqlstm.sqindv[15] = (         void  *)&IndicatorWorklist.PatientID;
 sqlstm.sqinds[15] = (         int  )0;
 sqlstm.sqharm[15] = (unsigned int  )0;
 sqlstm.sqadto[15] = (unsigned short )0;
 sqlstm.sqtdso[15] = (unsigned short )0;
 sqlstm.sqhstv[16] = (         void  *)WorklistInBridge.PatientsBirthDate;
 sqlstm.sqhstl[16] = (unsigned int  )9;
 sqlstm.sqhsts[16] = (         int  )0;
 sqlstm.sqindv[16] = (         void  *)&IndicatorWorklist.PatientsBirthDate;
 sqlstm.sqinds[16] = (         int  )0;
 sqlstm.sqharm[16] = (unsigned int  )0;
 sqlstm.sqadto[16] = (unsigned short )0;
 sqlstm.sqtdso[16] = (unsigned short )0;
 sqlstm.sqhstv[17] = (         void  *)WorklistInBridge.PatientsSex;
 sqlstm.sqhstl[17] = (unsigned int  )17;
 sqlstm.sqhsts[17] = (         int  )0;
 sqlstm.sqindv[17] = (         void  *)&IndicatorWorklist.PatientsSex;
 sqlstm.sqinds[17] = (         int  )0;
 sqlstm.sqharm[17] = (unsigned int  )0;
 sqlstm.sqadto[17] = (unsigned short )0;
 sqlstm.sqtdso[17] = (unsigned short )0;
 sqlstm.sqhstv[18] = (         void  *)WorklistInBridge.PatientsWeight;
 sqlstm.sqhstl[18] = (unsigned int  )17;
 sqlstm.sqhsts[18] = (         int  )0;
 sqlstm.sqindv[18] = (         void  *)&IndicatorWorklist.PatientsWeight;
 sqlstm.sqinds[18] = (         int  )0;
 sqlstm.sqharm[18] = (unsigned int  )0;
 sqlstm.sqadto[18] = (unsigned short )0;
 sqlstm.sqtdso[18] = (unsigned short )0;
 sqlstm.sqhstv[19] = (         void  *)WorklistInBridge.AdmittingDiagnosesDescription;
 sqlstm.sqhstl[19] = (unsigned int  )65;
 sqlstm.sqhsts[19] = (         int  )0;
 sqlstm.sqindv[19] = (         void  *)&IndicatorWorklist.AdmittingDiagnosesDescription;
 sqlstm.sqinds[19] = (         int  )0;
 sqlstm.sqharm[19] = (unsigned int  )0;
 sqlstm.sqadto[19] = (unsigned short )0;
 sqlstm.sqtdso[19] = (unsigned short )0;
 sqlstm.sqhstv[20] = (         void  *)WorklistInBridge.PatientsAge;
 sqlstm.sqhstl[20] = (unsigned int  )5;
 sqlstm.sqhsts[20] = (         int  )0;
 sqlstm.sqindv[20] = (         void  *)&IndicatorWorklist.PatientsAge;
 sqlstm.sqinds[20] = (         int  )0;
 sqlstm.sqharm[20] = (unsigned int  )0;
 sqlstm.sqadto[20] = (unsigned short )0;
 sqlstm.sqtdso[20] = (unsigned short )0;
 sqlstm.sqhstv[21] = (         void  *)&WorklistInBridge.SupportChinese;
 sqlstm.sqhstl[21] = (unsigned int  )sizeof(int);
 sqlstm.sqhsts[21] = (         int  )0;
 sqlstm.sqindv[21] = (         void  *)&IndicatorWorklist.SupportChinese;
 sqlstm.sqinds[21] = (         int  )0;
 sqlstm.sqharm[21] = (unsigned int  )0;
 sqlstm.sqadto[21] = (unsigned short )0;
 sqlstm.sqtdso[21] = (unsigned short )0;
 sqlstm.sqhstv[22] = (         void  *)&WorklistInBridge.DicomPersonName;
 sqlstm.sqhstl[22] = (unsigned int  )sizeof(int);
 sqlstm.sqhsts[22] = (         int  )0;
 sqlstm.sqindv[22] = (         void  *)&IndicatorWorklist.DicomPersonName;
 sqlstm.sqinds[22] = (         int  )0;
 sqlstm.sqharm[22] = (unsigned int  )0;
 sqlstm.sqadto[22] = (unsigned short )0;
 sqlstm.sqtdso[22] = (unsigned short )0;
 sqlstm.sqphsv = sqlstm.sqhstv;
 sqlstm.sqphsl = sqlstm.sqhstl;
 sqlstm.sqphss = sqlstm.sqhsts;
 sqlstm.sqpind = sqlstm.sqindv;
 sqlstm.sqpins = sqlstm.sqinds;
 sqlstm.sqparm = sqlstm.sqharm;
 sqlstm.sqparc = sqlstm.sqharc;
 sqlstm.sqpadto = sqlstm.sqadto;
 sqlstm.sqptdso = sqlstm.sqtdso;
 sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
 if (sqlca.sqlcode == 1403) break;
 if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetWorklistFromDB));
}


	callback(&WorklistInBridge, &IndicatorWorklist, dbim);
  }
  /* EXEC SQL CLOSE C; */ 

{
  struct sqlexd sqlstm;
  sqlstm.sqlvsn = 12;
  sqlstm.arrsiz = 52;
  sqlstm.sqladtp = &sqladt;
  sqlstm.sqltdsp = &sqltds;
  sqlstm.iters = (unsigned int  )1;
  sqlstm.offset = (unsigned int  )609;
  sqlstm.cud = sqlcud0;
  sqlstm.sqlest = (unsigned char  *)&sqlca;
  sqlstm.sqlety = (unsigned short)4352;
  sqlstm.occurs = (unsigned int  )0;
  sqlcxt((void **)0, &sqlctx, &sqlstm, &sqlfpn);
  if (sqlca.sqlcode < 0) return(SqlError(BRIDGE_GetWorklistFromDB));
}


  return true;
}
