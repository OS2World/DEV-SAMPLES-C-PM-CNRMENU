/*********************************************************************
 *                                                                   *
 * MODULE NAME :  create.c               			     *
 * 			                                             *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  This module is part of CNRMENU.EXE. It performs the function of  *
 *  creating the container window and starting a thread that will    *
 *  populate the window with file icons.                             *
 *                                                                   *
 *  The primary purpose of this module is to create the container    *
 *  and set it up to be able to switch views easily. This is         *
 *  accomplished by creating the column definitions for the Details  *
 *  view. The Details view is really the only view that requires     *
 *  any substantial setup.                                           *
 *                                                                   *
 *  We also set the base directory here. This directory will be      *
 *  traversed in the PopulateContainer thread as will all its        *
 *  subdirectories. If the user entered a directory name on the      *
 *  command line we don't need to do anything. If not, we get the    *
 *  current drive and directory and use it as the base.              *
 *                                                                   *
 * CALLABLE FUNCTIONS:                                               *
 *                                                                   *
 *   HWND CreateDirectoryWin( PSZ szDirectory, PCNRITEM pciParent ); *
 *   HWND CreateContainer( HWND hwndClient, PSZ szDirectory );       *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-24-92 - Source copied from CNRBASE.EXE sample.                *
 *             Removed FCF_MENU from FRAME_FLAGS. Will use context   *
 *               menu.                                               *
 *             Changed WinShowWindow to WinSetWindowPos in the       *
 *               CreateDirWindow function so we could SHOW and       *
 *               ACTIVATE and ZORDER the secondary windows.          *
 *             Added 2nd and 3rd parms to CreateDirectoryWin.        *
 *             Added 3rd and 4th parms to CreateContainer.           *
 *             Pass WINCREATE stuct in WinCreateWindow of client     *
 *               rather than just the directory name.                *
 *             Add CCS_EXTENDSEL to container styles.                *
 *                                                                   *
 *                                                                   *
 *********************************************************************/

// #pragma strings(readonly)   // used for debug version of memory mgmt routines

/*********************************************************************/
/*------- Include relevant sections of the OS/2 header files --------*/
/*********************************************************************/

#define  INCL_WINERRORS
#define  INCL_WINFRAMEMGR
#define  INCL_WINSTDCNR
#define  INCL_WINWINDOWMGR

/**********************************************************************/
/*----------------------------- INCLUDES -----------------------------*/
/**********************************************************************/

#include <os2.h>
#include <process.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnrmenu.h"

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define FRAME_FLAGS         (FCF_TASKLIST | FCF_TITLEBAR   | FCF_SYSMENU | \
                             FCF_MINMAX   | FCF_SIZEBORDER | FCF_SHELLPOSITION)

#define CONTAINER_COLUMNS   5           // Number of columns in details view

#define SPLITBAR_OFFSET     150         // Pixel offset of details splitbar

#define THREAD_STACKSIZE    65536       // Stacksize for secondary thread

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

static BOOL SetContainerColumns  ( HWND hwndCnr );
static VOID GetCurrentDirectory  ( PSZ pszDirectory );
static VOID UseCmdLineDirectory  ( PSZ pszDirectoryOut, PSZ szDirectoryIn );

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------- CreateDirectoryWin -------------------------*/
/*                                                                    */
/*  CREATE A DIRECTORY WINDOW MADE UP OF FRAME/CLIENT/CONTAINER.      */
/*                                                                    */
/*  INPUT: directory to represent in the window,                      */
/*         window handle with which to share records, or NULLHANDLE,  */
/*         pointer to CNRITEM if using shared records, or NULL        */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: window handle of created frame or NULLHANDLE if error     */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
HWND CreateDirectoryWin( PSZ szDirectory, HWND hwndCnrShare, PCNRITEM pciParent)
{
    FRAMECDATA  fcdata;
    HWND        hwndFrame, hwndClient = NULLHANDLE;

    (void) memset( &fcdata, 0, sizeof( FRAMECDATA ) );

    fcdata.cb               = sizeof( FRAMECDATA );
    fcdata.flCreateFlags    = FRAME_FLAGS;
    fcdata.idResources      = ID_RESOURCES;

    // Create the frame window. If we were creating multiple instances of
    // this window, iWinCount would become useful.

    hwndFrame = WinCreateWindow( HWND_DESKTOP, WC_FRAME, NULL, 0, 0, 0, 0, 0,
                                 NULLHANDLE, HWND_TOP,
                                 ID_FIRST_DIRWINDOW + iWinCount, &fcdata,
                                 NULL );

    // Create the client window which will create the container in its
    // WM_CREATE processing. If that fails, hwndClient will be NULLHANDLE.
    // We pass parameters in the CTLDATA parameter which will be received by the
    // client in mp1 on WM_CREATE.

    if( hwndFrame )
    {
        // This memory block will be freed after WM_CREATE processing is
        // complete by InitClient in cnrmenu.c

        PWINCREATE pwc = (PWINCREATE) malloc( sizeof( WINCREATE ) );

        if( pwc )
        {
            pwc->szDirectory  = szDirectory;
            pwc->hwndCnrShare = hwndCnrShare;
            pwc->pciParent    = pciParent;

            hwndClient = WinCreateWindow( hwndFrame, (PCSZ) DIRECTORY_WINCLASS, NULL,
                                          0, 0, 0, 0, 0, NULLHANDLE, HWND_TOP,
                                          FID_CLIENT, pwc, NULL );
        }
        else
            Msg( (PSZ) "Out of memory in CreateContainer!" );
    }

    if( hwndClient )
    {
        // Show the frame window (it was created invisible)

        if( !WinSetWindowPos( hwndFrame, HWND_TOP, 0, 0, 0, 0,
                              SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER ) )
        {
            hwndFrame = NULLHANDLE;

            Msg( (PSZ) "hwndFrame WinSetWindowPos RC(%X)", HWNDERR( hwndClient ) );
        }
    }
    else
        hwndFrame = NULLHANDLE;

    return hwndFrame;
}

/**********************************************************************/
/*------------------------- CreateContainer --------------------------*/
/*                                                                    */
/*  CREATE THE CONTAINER AND CUSTOMIZE IT.                            */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         directory to display in the container,                     */
/*         window handle with which to share records, or NULLHANDLE,  */
/*         pointer to CNRITEM if using shared records, or NULL        */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: Container window handle                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
HWND CreateContainer( HWND hwndClient, PSZ szDirectory, HWND hwndCnrShare,
                      PCNRITEM pciParent )
{
    PINSTANCE pi = INSTDATA( hwndClient );
    HWND      hwndCnr = NULLHANDLE;

    if( !pi )
    {
        Msg( (PSZ) "CreateContainer cant get Inst data. RC(%X)", HWNDERR(hwndClient));

        return NULLHANDLE;
    }

    // Create the container control. Indicate that we want to use
    // MINIRECORDCORE structures rather than RECORDCORE structures. This
    // saves memory. Also allow extended selection which essentially lets the
    // user select multiple items.

    hwndCnr = WinCreateWindow( hwndClient, WC_CONTAINER, NULL,
                               CCS_EXTENDSEL | CCS_MINIRECORDCORE | WS_VISIBLE,
                               0, 0, 0, 0, hwndClient, HWND_TOP, CNR_DIRECTORY,
                               NULL, NULL);

    if( hwndCnr )
    {
        if( SetContainerColumns( hwndCnr ) )
        {
            // Start the thread that will populate the container. Allocate
            // memory to pass the thread a structure of data.

            PTHREADPARMS ptp = malloc( sizeof( THREADPARMS ) );

            if( ptp )
            {
                TID tid = 0;

                ptp->hwndClient     = hwndClient;
                ptp->hwndCnrShare   = hwndCnrShare;
                ptp->pciParent      = pciParent;

                // If no directory was passed to us, assume that the current
                // directory is to be displayed.

                if( !szDirectory )
                    GetCurrentDirectory( (PSZ) pi->szDirectory );
                else
                    UseCmdLineDirectory( (PSZ) pi->szDirectory, szDirectory );

                SetWindowTitle( hwndClient, (PSZ) "%s: Processing %s...",
                                PROGRAM_TITLE, pi->szDirectory );

                // Start the thread that will populate the container with
                // file icons. This is a separate thread because it could
                // take a while if there are many subdirectories.

                tid = _beginthread( PopulateContainer, NULL,
                                    THREAD_STACKSIZE, (PVOID) ptp );

                if( (INT) tid == -1 )
                    Msg( (PSZ) "Cant start PopulateContainer thread!" );
            }
            else
            {
                hwndCnr = NULLHANDLE;

                Msg( (PSZ) "Out of Memory in CreateContainer!" );
            }
        }
        else
            hwndCnr = NULLHANDLE;
    }
    else
        Msg( (PSZ) "Couldnt create container. RC(%X)", HWNDERR( hwndClient ) );

    return hwndCnr;
}

/**********************************************************************/
/*----------------------- SetContainerColumns ------------------------*/
/*                                                                    */
/*  SET THE COLUMNS OF THE CONTAINER FOR DETAIL VIEW                  */
/*                                                                    */
/*  INPUT: container window handle                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL SetContainerColumns( HWND hwndCnr )
{
    BOOL        fSuccess = TRUE;
    PFIELDINFO  pfi, pfiLastLeftCol;

    // Allocate storage for container column data

    pfi = WinSendMsg( hwndCnr, CM_ALLOCDETAILFIELDINFO,
                      MPFROMLONG( CONTAINER_COLUMNS ), NULL );

    if( pfi )
    {
        PFIELDINFO      pfiFirst;
        FIELDINFOINSERT fii;

        // Store original value of pfi so we won't lose it when it changes.
        // This will be needed on the CM_INSERTDETAILFIELDINFO message.

        pfiFirst = pfi;

        // Fill in column information for the icon column

        pfi->flData     = CFA_BITMAPORICON | CFA_HORZSEPARATOR | CFA_CENTER |
                          CFA_SEPARATOR;
        pfi->flTitle    = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = "Icon";
        pfi->offStruct  = FIELDOFFSET( CNRITEM, rc.hptrIcon );

        // Fill in column information for the file name. Note that we are
        // using the rc.pszIcon variable rather than szFileName. We do this
        // because the container needs a pointer to the file name. If we used
        // szFileName (a character array, not a pointer), the container would
        // take the first 4 bytes of szFileName and think it was a pointer,
        // which of course it is not. Later in the FillInRecord function we set
        // rc.pszIcon to point to szFileName.

        pfi             = pfi->pNextFieldInfo;
        pfi->flData     = CFA_STRING | CFA_LEFT | CFA_HORZSEPARATOR;
        pfi->flTitle    = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = "File Name";
        pfi->offStruct  = FIELDOFFSET( CNRITEM, rc.pszIcon );

        // Store the current pfi value as that will be used to indicate the
        // last column in the lefthand container window (we have a splitbar)

        pfiLastLeftCol = pfi;

        // Fill in column information for the file size

        pfi             = pfi->pNextFieldInfo;
        pfi->flData     = CFA_ULONG | CFA_RIGHT | CFA_HORZSEPARATOR |
                          CFA_SEPARATOR;
        pfi->flTitle    = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = "File Size";
        pfi->offStruct  = FIELDOFFSET( CNRITEM, cbFile );

        // Fill in column information for file date

        pfi             = pfi->pNextFieldInfo;
        pfi->flData     = CFA_DATE | CFA_RIGHT | CFA_HORZSEPARATOR |
                          CFA_SEPARATOR;
        pfi->flTitle    = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData =  "Date";
        pfi->offStruct  = FIELDOFFSET( CNRITEM, date );

        // Fill in column information for the file time

        pfi             = pfi->pNextFieldInfo;
        pfi->flData     = CFA_TIME | CFA_RIGHT | CFA_HORZSEPARATOR;
        pfi->flTitle    = CFA_CENTER | CFA_FITITLEREADONLY;
        pfi->pTitleData = "Time";
        pfi->offStruct  = FIELDOFFSET( CNRITEM, time );

        // Use the CM_INSERTDETAILFIELDINFO message to tell the container
        // all the column information it needs to function properly. Place
        // this column info first in the column list and update the display
        // after they are inserted (fInvalidateFieldInfo = TRUE)

        (void) memset( &fii, 0, sizeof( FIELDINFOINSERT ) );

        fii.cb                   = sizeof( FIELDINFOINSERT );
        fii.pFieldInfoOrder      = (PFIELDINFO) CMA_FIRST;
        fii.cFieldInfoInsert     = (SHORT) CONTAINER_COLUMNS;
        fii.fInvalidateFieldInfo = TRUE;

        if( !WinSendMsg( hwndCnr, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiFirst ),
                         MPFROMP( &fii ) ) )
        {
            fSuccess = FALSE;

            Msg( (PSZ) "CM_INSERTDETAILFIELDINFO RC(%X)", HWNDERR( hwndCnr ) );
        }
    }
    else
    {
        fSuccess = FALSE;

        Msg( (PSZ) "CM_ALLOCDETAILFIELDINFO failed. RC(%X)", HWNDERR( hwndCnr ) );
    }

    if( fSuccess )
    {
        CNRINFO cnri;

        // Tell the container about the splitbar and where it goes

        cnri.cb             = sizeof( CNRINFO );
        cnri.pFieldInfoLast = pfiLastLeftCol;
        cnri.xVertSplitbar  = SPLITBAR_OFFSET;

        if( !WinSendMsg( hwndCnr, CM_SETCNRINFO, MPFROMP( &cnri ),
                        MPFROMLONG( CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR ) ) )
        {
            fSuccess = FALSE;

            Msg( (PSZ) "SetContainerColumns CM_SETCNRINFO RC(%X)", HWNDERR(hwndCnr) );
        }
    }

    return fSuccess;
}

/**********************************************************************/
/*----------------------- GetCurrentDirectory ------------------------*/
/*                                                                    */
/*  GET THE CURRENT DIRECTORY AND STORE IT.                           */
/*                                                                    */
/*  INPUT: pointer to buffer in which to place path info              */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID GetCurrentDirectory( PSZ pszDirectory )
{
    ULONG  cbDirPath = CCHMAXPATH;
    APIRET rc;
    ULONG  ulCurrDrive, ulDrvMap;

    // Set up the "d:\" string to which we will add the current directory.
    // If we can get the current drive number, use it. Otherwise default to
    // C:

    rc = DosQueryCurrentDisk( &ulCurrDrive, &ulDrvMap );

    if( !rc )
    {
        pszDirectory[ 0 ] = ulCurrDrive + ('A' - 1);

        (void) strcpy( (char * restrict) pszDirectory + 1, ":\\" );
    }
    else
        (void) strcpy( (char * restrict) pszDirectory, "C:\\" );

    rc = DosQueryCurrentDir( 0, pszDirectory + 3, &cbDirPath );

    // If good retcode, DosQueryCurrentDir just returns the current
    // directory without a trailing backslash. Of course there are always
    // complications. The complication here is that if the current directory is
    // the root, the string *will* end with a backslash. So if the trailing
    // character is a backslash, remove it so we have a pure directory name.

    if( pszDirectory[ strlen( (const char *) pszDirectory ) - 1 ] == '\\' )
        pszDirectory[ strlen( (const char *) pszDirectory ) - 1 ] = 0;

    if( rc )
        Msg( (PSZ) "DosQueryCurrentDir RC(%X). Using Root Directory.", rc );

    return;
}

/**********************************************************************/
/*----------------------- UseCmdLineDirectory ------------------------*/
/*                                                                    */
/*  TAKE THE DIRECTORY FROM THE COMMANDLINE AND VALIDATE IT.          */
/*                                                                    */
/*  INPUT: pointer to buffer in which to place correct path,          */
/*         directory input at the commandline                         */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID UseCmdLineDirectory( PSZ pszDirectoryOut, PSZ szDirectoryIn )
{
    (void) strcpy( (char * restrict) pszDirectoryOut, (const char * restrict) szDirectoryIn );

    // If the user ended their directory name with a backslash, take it off
    // because this program needs to have just the base directory name here.

    if( pszDirectoryOut[ strlen( (const char * restrict) pszDirectoryOut ) - 1 ] == '\\' )
        pszDirectoryOut[ strlen( (const char *) pszDirectoryOut ) - 1 ] = 0;

    return;
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
