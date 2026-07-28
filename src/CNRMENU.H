/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrmenu.h                                          *
 * DATE WRITTEN:  10-24-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  Common definitions, structures, and function prototypes for      *
 *  all modules of CNRMENU.EXE.                                      *
 *                                                                   *
 *  Globals are declared with the DATADEF macro: cnrmenu.c defines   *
 *  GLOBALS_DEFINED before including this header, causing DATADEF to *
 *  expand to nothing (allocating storage). All other modules see    *
 *  DATADEF as 'extern' (referencing without allocating).            *
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
 *  2026-07-28 No structural changes. Source moved to src/.          *
 *                                                                   *
 *                                                                   *
 *********************************************************************/

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define ID_FIRST_DIRWINDOW   1         // Id of first Directory frame window

#define ID_RESOURCES         1         // Id of frame wnd resources in .RC file

#define CNR_DIRECTORY        30        // Window id for the container control

#define ID_CONTEXT_MENU      1000      // Id of the context menu resource
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

#define UM_CONTAINER_FILLED  WM_USER   // Posted by fill thread to primary thread

#define DEBUG_FILENAME       "cnrmenu.dbg"

#define DIRECTORY_WINCLASS   "CnrMenuSample"

#define PROGRAM_TITLE        "CNRMENU"

// Convenience macros for PM error/instance-data access

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
  MINIRECORDCORE rc;                  // Base information (icon, text pointer)
  CDATE          date;                // Date of last write
  CTIME          time;                // Time of last write
  ULONG          cbFile;              // File size in bytes
  ULONG          attrFile;            // DOS file attributes (FILE_DIRECTORY etc.)
  INT            iDirPosition;        // Relative position within directory
  BOOL           fSelected;           // TRUE while this record has source emphasis
  CHAR           szFileName[CCHMAXPATH + 1]; // File name (rc.pszIcon points here)

} CNRITEM, *PCNRITEM;

// Number of extra bytes per record beyond the MINIRECORDCORE base structure

#define EXTRA_RECORD_BYTES (sizeof( CNRITEM ) - sizeof( MINIRECORDCORE ))


typedef struct _INSTANCE              // PER-WINDOW INSTANCE DATA
{
    BOOL fShutdown;                     // Main thread wants to shut down
    BOOL fContainerFilled;              // Fill thread has completed
    PCNRITEM pciSelected;               // Record under the mouse when ctx menu opened
    BOOL fDirSelected;                  // At least one selected record is a directory

    // Frame handles for each "Other Window" submenu entry (one per possible item)
    HWND hwndFrame[ IDM_OTHERWIN_LASTITEM - IDM_OTHERWIN_ITEM1 + 1 ];

    CHAR szCnrTitle[ CCHMAXPATH + 20 ]; // Container title text
    CHAR szDirectory[ CCHMAXPATH + 1 ]; // Directory currently displayed
    CHAR achWorkBuf[ CCHMAXPATH + 1 ];  // Scratch buffer for path operations

} INSTANCE, *PINSTANCE;


typedef struct _THREADPARMS          // PARAMETERS PASSED TO THE FILL THREAD
{
    HWND hwndClient;                 // Client window handle
    HWND hwndCnrShare;               // Container whose records will be shared
    PCNRITEM pciParent;              // Starting parent record when sharing records

} THREADPARMS, *PTHREADPARMS;


typedef struct _WINCREATE            // PARAMETERS PASSED TO CLIENT ON WM_CREATE
{
    PSZ szDirectory;                 // Pointer to directory name to display
    HWND hwndCnrShare;               // Container whose records will be shared
    PCNRITEM pciParent;              // Starting parent record when sharing records

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

// cnrmenu.c defines GLOBALS_DEFINED, making DATADEF empty (allocates storage).
// All other modules see DATADEF as 'extern' (references existing storage).

#ifdef GLOBALS_DEFINED
#  define DATADEF
#else
#  define DATADEF extern
#endif

DATADEF INT iWinCount;           // Number of directory windows currently open
DATADEF BOOL fTrue;              // Sentinel for while(fTrue) loops (always TRUE)

/***********************************************************************
 *                   E N D     O F     S O U R C E                     *
 **********************************************************************/
