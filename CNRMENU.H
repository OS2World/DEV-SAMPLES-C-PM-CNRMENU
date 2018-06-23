/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrmenu.h              AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-24-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  Common definitions and function prototypes for CNRMENU.EXE       *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-24-92 - File copied from CNRBASE.EXE sample.                  *
 *             Added ctxtmenu.c definitions, function prototypes.    *
 *             Changed CreateDirectoryWin proto - added 2 parms.     *
 *             Changed CreateContainer proto - added 2 parms.        *
 *             Added pciSelected to INSTANCE struct.                 *
 *             Added WINCREATE struct.                               *
 *             Added 2 variables to THREADPARMS struct.              *
 *             Added edit.c definitions, function prototypes.        *
 *             Added new submenus, menu items.                       *
 *             Added iDirPosition to CNRITEM struct.                 *
 *             Added OWNER macro.                                    *
 *             Added CRA_SOURCE definition.                          *
 *             Added fDirSelected to INSTANCE struct.                *
 *             Added fSelected to CNRITEM struct.                    *
 *             Added sort.c definitions, function prototypes.        *
 *             Added hwndFrame array to INSTANCE struct.             *
 *             Added FullyQualify to common.c function prototypes.   *
 *             Added achWorkBuf to INSTANCE data structure.          *
 *  01-01-93   Took out pszFileName and hptrIcon from CNRITEM -      *
 *               will use the fields in the MINIRECORDCORE struct.   *
 *             Added fTrue to satisfy new C Set/2++ compiler in      *
 *               while( fTrue ) statements. The new compiler does    *
 *               not allow while( TRUE ) or for( ; ; ) statements.   *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define ID_FIRST_DIRWINDOW   1         // Id of first Directory frame window

#define ID_RESOURCES         1         // Id of frame wnd resources in .RC file

#define CNR_DIRECTORY        30        // The start of our show

#define ID_CONTEXT_MENU      1000      // Id of the context menu
#define IDM_VIEW_SUBMENU     1100      // View Submenu id
#define IDM_VIEW_ICON        1101      // View Submenu item ids
#define IDM_VIEW_NAME        1102
#define IDM_VIEW_TREE        1103
#define IDM_VIEW_DETAILS     1104
#define IDM_VIEW_TEXT        1105
#define IDM_SORT_SUBMENU     1200      // Sort Submenu id
#define IDM_SORT_NAME        1201      // Sort Submenu item ids
#define IDM_SORT_DATETIME    1202
#define IDM_SORT_DIRORDER    1203
#define IDM_OTHERWIN_SUBMENU 1300      // "Other Window" Submenu
#define IDM_OTHERWIN_ITEM1   1301      // ID of first "Other Window" menu item
#define IDM_OTHERWIN_LASTITEM 1399     // ID of last "Other Window" menu item
#define IDM_CREATE_NEWWIN    1400      // CreateNewWindow menu item
#define IDM_ARRANGE          1500      // Arrange menu item

#define UM_CONTAINER_FILLED  WM_USER   // Posted by Fill thread to primary thrd

#define DEBUG_FILENAME       "cnrmenu.dbg"

#define DIRECTORY_WINCLASS   "CnrMenuSample"

#define PROGRAM_TITLE        "CNRMENU"

#define HABERR( hab )        (ERRORIDERROR( WinGetLastError( hab ) ))

#define ANCHOR( hwnd )       (WinQueryAnchorBlock( hwnd ))

#define HWNDERR( hwnd )       \
            (ERRORIDERROR( WinGetLastError( WinQueryAnchorBlock( hwnd ) ) ))

#define INSTDATA( hwnd )      (WinQueryWindowPtr( hwnd, 0 ))

#define PARENT( hwnd )        (WinQueryWindow( hwnd, QW_PARENT ))

#define OWNER( hwnd )         (WinQueryWindow( hwnd, QW_OWNER ))

#define GRANDPARENT( hwnd )   (WinQueryWindow( WinQueryWindow(hwnd, QW_PARENT)))

#ifndef CRA_SOURCE            // As of 10/30/92, CRA_SOURCE not in toolkit hdrs
#  define CRA_SOURCE          0x00004000L
#endif

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

typedef struct _CNRITEM               // CONTAINER RECORD STRUCTURE
{
  MINIRECORDCORE rc;                  // Base information
  CDATE          date;                // Date of file
  CTIME          time;                // Time of file
  ULONG          cbFile;              // File size
  ULONG          attrFile;            // Attributes of this file
  INT            iDirPosition;        // Relative position in directory
  BOOL           fSelected;           // Does this record have source emphasis
  CHAR           szFileName[CCHMAXPATH + 1]; // File name

} CNRITEM, *PCNRITEM;

#define EXTRA_RECORD_BYTES (sizeof( CNRITEM ) - sizeof( MINIRECORDCORE ))


typedef struct _INSTANCE              // CONTAINER'S CLIENT WINDOW INSTANCE DATA
{
    BOOL fShutdown;                     // Main thread wants to shut down
    BOOL fContainerFilled;              // Fill thread has completed
    PCNRITEM pciSelected;               // Item selected under context menu
    BOOL fDirSelected;                  // One of the selected recs is a dir

    // Used to keep track of frame windows associated with OtherWindow menuitems
    HWND hwndFrame[ IDM_OTHERWIN_LASTITEM - IDM_OTHERWIN_ITEM1 + 1 ];

    CHAR szCnrTitle[ CCHMAXPATH + 20 ]; // Container title
    CHAR szDirectory[ CCHMAXPATH + 1 ]; // Directory being displayed
    CHAR achWorkBuf[ CCHMAXPATH + 1 ];  // Instance work buffer

} INSTANCE, *PINSTANCE;


typedef struct _THREADPARMS          // PARAMETERS TO PASS TO SECONDARY THREAD
{
    HWND hwndClient;                 // Client window handle
    HWND hwndCnrShare;               // Container hwnd whose recs will be shared
    PCNRITEM pciParent;              // Ptr to parent CNRITEM if using shr recs

} THREADPARMS, *PTHREADPARMS;


typedef struct _WINCREATE            // PARAMETERS TO PASS TO WINDOW ON CREATION
{
    PSZ szDirectory;                 // Pointer to directory name
    HWND hwndCnrShare;               // Container hwnd whose recs will be shared
    PCNRITEM pciParent;              // Ptr to parent CNRITEM if using shr recs

} WINCREATE, *PWINCREATE;

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

// In common.c

VOID SetWindowTitle( HWND hwndClient, PSZ szFormat, ... );
VOID Msg( PSZ szFormat, ... );
VOID FullyQualify( PSZ szDirectory, HWND hwndCnr, PCNRITEM pci );

// In create.c

HWND CreateDirectoryWin( PSZ szDirectory, HWND hwndCnr, PCNRITEM pciFirst );
HWND CreateContainer( HWND hwndClient, PSZ szDirectory, HWND hwndCnrShare,
                      PCNRITEM pciFirst );

// In populate.c

VOID PopulateContainer( PVOID pThreadParms );

// In ctxtmenu.c

VOID CtxtmenuCreate( HWND hwndClient, PCNRITEM pciSelected );
VOID CtxtmenuCommand( HWND hwndClient, ULONG idCommand, ULONG ulCmdSrc );
VOID CtxtmenuSetView( HWND hwndClient, ULONG ulViewType );
VOID CtxtmenuEnd( HWND hwndClient );

// In edit.c

VOID EditBegin( HWND hwndClient, PCNREDITDATA pced );
VOID EditEnd( HWND hwndClient, PCNREDITDATA pced );

// In sort.c

VOID SortContainer( HWND hwndClient, ULONG ulSortType );

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

#ifdef GLOBALS_DEFINED
#  define DATADEF
#else
#  define DATADEF extern
#endif

DATADEF INT iWinCount;           // Number of directory windows created
DATADEF BOOL fTrue;              // Used for while( fTrue ) to satisfy CSet2++

/***********************************************************************
 *                   E N D     O F     S O U R C E                     *
 **********************************************************************/
