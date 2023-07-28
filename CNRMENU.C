/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrmenu.c             			     *
 * 			                                             *
 *                                                                   *
 * HOW TO RUN THIS PROGRAM:                                          *
 *                                                                   *
 *  By just entering CNRMENU on the command line, a container will   *
 *  be created that will contain the files in the current directory. *
 *  Any subdirectories will be included and expandable in Tree view. *
 *  The container starts in Tree view. You can switch to the other   *
 *  supported views.                                                 *
 *                                                                   *
 *  Optionally enter 'CNRMENU directory' and that directory will be  *
 *  displayed.                                                       *
 *                                                                   *
 * MODULE DESCRIPTION:                                               *
 *                                                                   *
 *  Root module for CNRMENU.EXE, a program that demonstrates the     *
 *  base functionality of a container control. This module contains  *
 *  the client window procedure and some supporting functions for    *
 *  the container messages. It builds on the CNRBASE.EXE sample      *
 *  program by adding a context menu, container record sharing,      *
 *  direct editing, source emphasis, and container sorting. It lets  *
 *  you create additional directory windows by double-clicking on    *
 *  a subdirectory in the current window.                            *
 *                                                                   *
 *  The container is populated with records from a secondary thread. *
 *  This is done not to complicate things but to let the user        *
 *  interact with the container before it is completely filled if we *
 *  are traversing a directory with many subdirectories.             *
 *                                                                   *
 *  Drag/Drop, Deltas, Ownerdraw, MiniIcons are not demonstrated     *
 *  in this sample program.                                          *
 *                                                                   *
 * OTHER MODULES:                                                    *
 *                                                                   *
 *  create.c -   contains the code used to create and tailor a       *
 *               container. These functions are called for each      *
 *               container that is created.                          *
 *                                                                   *
 *  populate.c - contains the code for the thread used to fill the   *
 *               container with records.                             *
 *                                                                   *
 *  common.c -   contains common support routines for CNRMENU.EXE.   *
 *                                                                   *
 *  ctxtmenu.c - contains context menu routines.                     *
 *                                                                   *
 *  edit.c -     contains direct editing routines.                   *
 *                                                                   *
 *  sort.c -     contains container sorting routines.                *
 *                                                                   *
 * NOTES:                                                            *
 *                                                                   *
 *  This program has gotten pretty complex as new aspects of the     *
 *  container were added. This makes it a pretty difficult program   *
 *  to follow. I believe it is most useful for exploring the         *
 *  different areas from a working program perspective. I don't      *
 *  think it would be a good idea to try and follow this program     *
 *  through its whole operation as you will quickly get lost in the  *
 *  details.                                                         *
 *                                                                   *
 *  I hope this code proves useful for other PM programmers. The     *
 *  more of us the better!                                           *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-24-92 - Source copied from CNRBASE.EXE sample.                *
 *             Moved SetContainerView to ctxtmenu.c (CtxtmenuSetView)*
 *             Moved WM_COMMAND code to ctxtmenu.c (CtxtmenuCommand) *
 *             Moved WM_CONTROL msg processing to wmControl function *
 *             Added RecordSelected function on CN_ENTER msg.        *
 *             Added directory name to the titlebar text.            *
 *             Added new parameters on CreateDirectoryWin call.      *
 *             Added new parameters on CreateContainer call.         *
 *             Get WINCREATE struct from mp1 on WM_CREATE instead of *
 *               just the directory name.                            *
 *             Moved the CnrBeginEdit and CnrEndEdit functions to    *
 *               edit.c as EditBegin and EditEnd.                    *
 *             Added call to CtxtmenuEnd on a WM_MENUEND msg.        *
 *  12-26-92   Added a WinDestroyWindow of hwndMenu in WM_MENUEND.   *
 *  01-01-93   Initialize fTrue for all while( fTrue ) loops.        *
 *  03-27-93   Changed PSZ szArg to char *szArg  - compiler bug.     *
 *                                                                   *
 *                                                                   *
 *********************************************************************/

// #pragma strings(readonly)   // used for debug version of memory mgmt routines

/*********************************************************************/
/*------- Include relevant sections of the OS/2 header files --------*/
/*********************************************************************/

#define INCL_DOSERRORS
#define INCL_WINDIALOGS
#define INCL_WINERRORS
#define INCL_WINFRAMEMGR
#define INCL_WINSTDCNR
#define INCL_WINSTDDLGS
#define INCL_WINWINDOWMGR

#define GLOBALS_DEFINED        // This module defines the globals in cnrmenu.h

/**********************************************************************/
/*----------------------------- INCLUDES -----------------------------*/
/**********************************************************************/

#include <os2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnrmenu.h"

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

       INT   main               ( INT iArgc, char *szArg[] );
static BOOL  InitClient         ( HWND hwndClient, PWINCREATE pwc );
static BOOL  wmControl          ( HWND hwndClient, USHORT idCtrl, MPARAM mp2 );
static VOID  RecordSelected     ( HWND hwndClient, PNOTIFYRECORDENTER pnre );
static VOID  ContainerFilled    ( HWND hwndClient );
static VOID  UserWantsToClose   ( HWND hwndClient );
static VOID  FreeResources      ( HWND hwndClient );

FNWP wpClient;

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*------------------------------ MAIN --------------------------------*/
/*                                                                    */
/*  PROGRAM ENTRYPOINT                                                */
/*                                                                    */
/*  INPUT: command line                                               */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: return code                                               */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
INT main( INT iArgc, char *szArg[] )
{
    BOOL  fSuccess = FALSE;
    HAB   hab;
    HMQ   hmq = NULLHANDLE;
    QMSG  qmsg;
    PSZ   szStartingDir = NULL;
    HWND  hwndFrame = NULLHANDLE;

    // This macro is defined for the debug version of the C Set/2 Memory
    // Management routines. Since the debug version writes to stderr, we
    // send all stderr output to a debuginfo file. Look in MAKEFILE to see how
    // to enable the debug version of those routines.

#ifdef __DEBUG_ALLOC__
    freopen( DEBUG_FILENAME, "w", stderr );
#endif

    // fTrue will be used for all while( fTrue ) loops. The C Set/2++ compiler
    // took away the ability to use while( TRUE ) and for( ; ; ) constructs.

    fTrue = TRUE;

    // Get the directory to display from the command line if specified.

    if( iArgc > 1 )
        szStartingDir =  (PSZ) szArg[ 1 ];

    hab = WinInitialize( 0 );

    if( hab )
        hmq = WinCreateMsgQueue( hab, 0 );
    else
    {
        DosBeep( 1000, 100 );

        (void) fprintf( stderr, "WinInitialize failed!" );
    }

    if( hmq )

        // CS_SIZEREDRAW needed for initial display of the container in the
        // client window. Allocate enough extra bytes for 1 window word.

        fSuccess = WinRegisterClass( hab, (PCSZ) DIRECTORY_WINCLASS, wpClient,
                                     CS_SIZEREDRAW, sizeof( PVOID ) );
    else
    {
        DosBeep( 1000, 100 );

        (void) fprintf( stderr, "WinCreateMsgQueue RC(%X)", HABERR( hab ) );
    }

    if( fSuccess )

        // CreateDirectoryWin is in CREATE.C

        hwndFrame = CreateDirectoryWin( szStartingDir, NULLHANDLE, NULL );
    else
        Msg( (PSZ) "WinRegisterClass RC(%X)", HABERR( hab ) );

    if( hwndFrame )
        while( WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ) )
            (void) WinDispatchMsg( hab, &qmsg );

    if( hmq )
        (void) WinDestroyMsgQueue( hmq );

    if( hab )
        (void) WinTerminate( hab );

#ifdef __DEBUG_ALLOC__
    _dump_allocated( -1 );
#endif

    return 0;
}

/**********************************************************************/
/*---------------------------- wpClient ------------------------------*/
/*                                                                    */
/*  CLIENT WINDOW PROCEDURE FOR THE DIRECTORY WINDOW.                 */
/*                                                                    */
/*  NOTE: This window is created by CreateDirectoryWin in CREATE.C    */
/*                                                                    */
/*  INPUT: standard window procedure parameters                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: result of message processing                              */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
MRESULT EXPENTRY wpClient( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_CREATE:

            // If window initialization fails, don't create window

            if( InitClient( hwnd, (PWINCREATE) mp1 ) )
            {
                iWinCount++;

                break;
            }
            else
                return (MRESULT) TRUE;


        case UM_CONTAINER_FILLED:

            // This message is posted to us by the thread that fills the
            // container with records. This indicates that the container is
            // now filled.

            ContainerFilled( hwnd );

            return 0;


        case WM_ERASEBACKGROUND:

            // Paint the client window in the default color

            return (MRESULT) TRUE;


        case WM_SIZE:

            // Size the container with the client window

            if( !WinSetWindowPos( WinWindowFromID( hwnd, CNR_DIRECTORY ),
                                  NULLHANDLE, 0, 0, SHORT1FROMMP( mp2 ),
                                  SHORT2FROMMP( mp2 ), SWP_MOVE | SWP_SIZE ) )
                Msg( (PSZ) "WM_SIZE WinSetWindowPos RC(%X)", HWNDERR( hwnd ) );

            return 0;


        case WM_COMMAND:    // Context Menu messages (in ctxtmenu.c)

            CtxtmenuCommand( hwnd, SHORT1FROMMP( mp1 ), SHORT1FROMMP( mp2 ) );

            return 0;


        case WM_CONTROL:

            // This function returns TRUE if the message was processed

            if( wmControl( hwnd, SHORT2FROMMP( mp1 ), mp2 ) )
                return 0;
            else
                break;


        case WM_MENUEND:

            // This function in ctxtmenu.c. Interestingly enough, even though
            // we give the menu the id of ID_CONTEXT_MENU, the ID that is
            // actually assigned to the popup menu as a whole is FID_MENU.
            // Since we get WM_MENUEND messages for the submenus of the popup
            // also and we only care when the whole popup is gone, just call
            // this function if it is the popup menu.

            if( SHORT1FROMMP( mp1 ) == FID_MENU )
            {
                CtxtmenuEnd( hwnd );

                // Destroy the window so its resources are freed up for the next
                // time we do a WinLoadMenu in ctxtmenu.c

                if( !WinDestroyWindow( (HWND) mp2 ) )
                    Msg( (PSZ) "WM_MENUEND WinDestroyWindow RC(%X)", HWNDERR( hwnd ));
            }

            break;


        case WM_CLOSE:

            // Don't let the WM_QUIT message get posted. *We* will decide
            // when to shut down the message queue.

            UserWantsToClose( hwnd );

            return 0;


        case WM_DESTROY:

            FreeResources( hwnd );

            // If this is the last window open, shut down the message queue
            // which will kill the process.

            if( --iWinCount == 0 )
                (void) WinPostMsg( NULLHANDLE, WM_QUIT, NULL, NULL );

            break;
    }

    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/**********************************************************************/
/*--------------------------- InitClient -----------------------------*/
/*                                                                    */
/*  PROCESS WM_CREATE FOR THE CLIENT WINDOW.                          */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to window creation parameters                      */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL InitClient( HWND hwndClient, PWINCREATE pwc )
{
    BOOL      fSuccess = TRUE;
    PINSTANCE pi = (PINSTANCE) malloc( sizeof( INSTANCE ) );

    if( pi )
    {
        (void) memset( pi, 0, sizeof( INSTANCE ) );

        if( WinSetWindowPtr( hwndClient, 0, pi ) )
        {
            // CreateContainer is located in CREATE.C

            HWND hwndCnr = CreateContainer( hwndClient, pwc->szDirectory,
                                            pwc->hwndCnrShare, pwc->pciParent );

            if( hwndCnr )

                // Set the initial container view to Tree/Icon view. This
                // function is in CTXTMENU.C

                CtxtmenuSetView( hwndClient, CV_TREE | CV_ICON );
            else
                fSuccess = FALSE;

            // This was allocated in create.c in the CreateDirectoryWin function

            free( pwc );
        }
        else
        {
            fSuccess = FALSE;

            Msg( (PSZ) "InitClient WinSetWindowPtr RC(%X)", HWNDERR( hwndClient ) );
        }
    }
    else
    {
        fSuccess = FALSE;

        Msg( (PSZ) "InitClient out of memory!" );
    }

    return fSuccess;
}

/**********************************************************************/
/*---------------------------- wmControl -----------------------------*/
/*                                                                    */
/*  PROCESS WM_CONTROL MESSAGES FOR THE CLIENT WINDOW.                */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         notify code,                                               */
/*         2nd WM_CONTROL message parameter                           */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if message processed                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL wmControl( HWND hwndClient, USHORT usNotifyCode, MPARAM mp2 )
{
    BOOL fProcessed = TRUE;

    switch( usNotifyCode )
    {
        case CN_ENTER:

            RecordSelected( hwndClient, (PNOTIFYRECORDENTER) mp2 );

            break;


        case CN_CONTEXTMENU:

            // In ctxtmenu.c

            CtxtmenuCreate( hwndClient, (PCNRITEM) mp2 );

            break;


        case CN_BEGINEDIT:

            // In edit.c

            EditBegin( hwndClient, (PCNREDITDATA) mp2 );

            break;


        case CN_ENDEDIT:

            // In edit.c

            EditEnd( hwndClient, (PCNREDITDATA) mp2 );

            break;


        default:

            fProcessed = FALSE;

            break;
    }

    return fProcessed;
}

/**********************************************************************/
/*-------------------------- RecordSelected --------------------------*/
/*                                                                    */
/*  PROCESS CN_ENTER NOTIFY MESSAGE.                                  */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to the NOTIFYRECORDENTER structure                 */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID RecordSelected( HWND hwndClient, PNOTIFYRECORDENTER pnre )
{
    PCNRITEM  pci = (PCNRITEM) pnre->pRecord;
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( (PSZ) "RecordSelected cant get Inst data. RC(%X)", HWNDERR(hwndClient) );

        return;
    }

    // If the user selected a directory, create another directory window for it.
    // Don't do anything if the directory is '..' (parent directory) or '.'
    // (current directory) as this program is not equipped to handle them.

    if( pci && (pci->attrFile & FILE_DIRECTORY) && pci->szFileName[0] != '.' )
    {
        // Set the selected CNRITEM container record so other functions can get
        // at it.

        pi->pciSelected = pci;

        // Simulate a IDM_CREATE_NEWWIN menuitem being selected. This allows
        // us to reuse the code that is processed when that menu item is pressed
        // Specify CMDSRC_OTHER so the code that processes the IDM_CREATE_NEWWIN
        // message can differentiate this message from one legitimately sent
        // from the context menu.

        WinSendMsg( hwndClient, WM_COMMAND,
                    MPFROM2SHORT( IDM_CREATE_NEWWIN, 0 ),
                    MPFROM2SHORT( CMDSRC_OTHER, 0 ) );
    }

    return;
}

/**********************************************************************/
/*------------------------- ContainerFilled --------------------------*/
/*                                                                    */
/*  THE FILL THREAD HAS COMPLETED.                                    */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID ContainerFilled( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( (PSZ) "ContainerFilled cant get Inst data. RC(%X)", HWNDERR(hwndClient));

        return;
    }

    // If the user closed the window while the Fill thread was executing, we
    // want to shut down this window now.

    if( pi->fShutdown )
        WinDestroyWindow( PARENT( hwndClient ) );
    else
    {
        // Set a flag so the window will know the Fill thread has finished

        pi->fContainerFilled = TRUE;

        // Set the titlebar to the program title. We do this because while
        // the container was being filled, the titlebar text was changed
        // to indicate progress.

        SetWindowTitle( hwndClient, (PSZ) "%s [%s]", PROGRAM_TITLE, pi->szDirectory );
    }

    return;
}

/**********************************************************************/
/*------------------------- UserWantsToClose -------------------------*/
/*                                                                    */
/*  PROCESS THE WM_CLOSE MESSAGE.                                     */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID UserWantsToClose( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( (PSZ) "UserWantsToClose cant get Inst data. RC(%X)",HWNDERR(hwndClient));

        return;
    }

    // If the Fill thread has completed, destroy the frame window.
    // If the Fill thread has not completed, set a flag that will cause it to
    // terminate, then the destroy will occur under the UM_CONTAINER_FILLED
    // message.

    if( pi->fContainerFilled )
        WinDestroyWindow( PARENT( hwndClient ) );
    else
    {
        // Indicate in the titlebar that the window is in the process of
        // closing.

        SetWindowTitle( hwndClient, (PSZ) "%s: CLOSING...", PROGRAM_TITLE );

        // Set a flag that will tell the Fill thread to shut down

        pi->fShutdown = TRUE;
    }

    return;
}

/**********************************************************************/
/*-------------------------- FreeResources ---------------------------*/
/*                                                                    */
/*  FREE PROGRAM RESOURCES.                                           */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID FreeResources( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( pi )
        free( pi );
    else
        Msg( (PSZ) "FreeResources cant get Inst data. RC(%X)", HWNDERR( hwndClient ));

    // Free the memory that was allocated with CM_ALLOCDETAILFIELDINFO. The
    // zero in the first SHORT of mp2 says to free memory for all columns

    if( -1 == (INT) WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY,
                                       CM_REMOVEDETAILFIELDINFO, NULL,
                                       MPFROM2SHORT( 0, CMA_FREE ) ) )
        Msg( (PSZ) "CM_REMOVEDETAILFIELDINFO failed! RC(%X)", HWNDERR( hwndClient ) );

    // Free the memory allocated by the CM_INSERTRECORD messages. The zero
    // in the first SHORT of mp2 says to free memory for all records

    if( -1 == (INT) WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY,
                                       CM_REMOVERECORD, NULL,
                                       MPFROM2SHORT( 0, CMA_FREE ) ) )
        Msg( (PSZ) "CM_REMOVERECORD failed! RC(%X)", HWNDERR( hwndClient ) );

    return;
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 ************************************************* ************************/
