/*********************************************************************
 *                                                                   *
 * MODULE NAME :  edit.c                 AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-24-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  This module is part of CNRMENU.EXE. It contains the functions    *
 *  necessary to implement container direct editing.                 *
 *                                                                   *
 * CALLABLE FUNCTIONS:                                               *
 *                                                                   *
 *  VOID EditBegin( HWND hwndClient, PCNREDITDATA pced );            *
 *  VOID EditEnd( HWND hwndClient, PCNREDITDATA pced );              *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-24-92 - Program coded.                                        *
 *  11-08-92 - Take off MLS_DISABLEUNDO flag for MLM_SETTEXTLIMIT.   *
 *  11-21-92 - Use WinSetWindowBits for taking off the               *
 *             MLS_DISABLEUNDO flag instead of WinQueryWindowULong/  *
 *             WinSetWindowULong per John Webb's idea.               *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
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
#define INCL_WINMLE
#define INCL_WINSTDCNR
#define INCL_WINSTDDLGS
#define INCL_WINWINDOWMGR

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

static ULONG GetMaxNameSize     ( CHAR chDrive );
static BOOL  RenameFile         ( HWND hwndCnr, PCNRITEM pci, PSZ szNewName );
static VOID RefreshAllContainers( HWND hwndCnr, PCNRITEM pciChanged );

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------------- EditBegin ----------------------------*/
/*                                                                    */
/*  PROCESS CN_BEGINEDIT NOTIFY MESSAGE.                              */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID EditBegin( HWND hwndClient, PCNREDITDATA pced )
{
    PFIELDINFO  pfi = pced->pFieldInfo;
    PINSTANCE   pi = INSTDATA( hwndClient );
    HWND        hwndMLE = WinWindowFromID(
                    WinWindowFromID( hwndClient, CNR_DIRECTORY ), CID_MLE );

    if( !pi )
    {
        Msg( (PSZ) "EditBegin cant get Inst data. RC(%X)", HWNDERR( hwndClient ) );

        return;
    }

    // pfi only available if details view. If we are in details view and
    // the column the user is direct-editing is the file name field, set the
    // text limit of the MLE to the Maximum that the filename can be.
    // If MLM_SETTEXTLIMIT returns a non-zero value, it means that the text
    // length in the MLE is greater than what we are trying to set it to.

    if( !pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, rc.pszIcon ) )
    {
        // The Service Pack (November, 1992) included a change where the
        // MLE was defined with the MLS_DISABLEUNDO flag. This causes problems
        // with MLM_SETTEXTLIMIT. With this bit enabled, when the user starts
        // typing after the text limit has been reached, the MLE beeps but
        // also displays the characters. I took off this bit flag with the
        // following code but the MLE doesn't re-check this style bit
        // unfortunately so it is still an outstanding bug.

        WinSetWindowBits( hwndMLE, QWL_STYLE, 0, MLS_DISABLEUNDO );

        WinUpdateWindow( hwndMLE );

        if( WinSendMsg( hwndMLE, MLM_SETTEXTLIMIT,
                        MPFROMLONG( GetMaxNameSize( pi->szDirectory[ 0 ] ) ),
                        NULL) )
            Msg( (PSZ) "MLM_SETTEXTLIMIT failed. RC(%X)", HWNDERR( hwndClient ) );
    }

    return;
}

/**********************************************************************/
/*----------------------------- EditEnd ------------------------------*/
/*                                                                    */
/*  PROCESS CN_ENDEDIT NOTIFY MESSAGE.                                */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID EditEnd( HWND hwndClient, PCNREDITDATA pced )
{
    PINSTANCE   pi = INSTDATA( hwndClient );
    PCNRITEM    pci = (PCNRITEM) pced->pRecord;
    PFIELDINFO  pfi = pced->pFieldInfo;
    HWND        hwndCnr, hwndMLE;

    if( !pi )
    {
        Msg( (PSZ) "EditEnd cant get Inst data. RC(%X)", HWNDERR( hwndClient ) );

        return;
    }

    hwndCnr = WinWindowFromID( hwndClient, CNR_DIRECTORY );

    // Get the handle to the MLE that the container uses for direct editing

    hwndMLE = WinWindowFromID( hwndCnr, CID_MLE );

    // pfi only available if details view

    if( pci && (!pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, rc.pszIcon )) )
    {
        CHAR szNewName[ CCHMAXPATH + 1 ];

        WinQueryWindowText( hwndMLE, sizeof( szNewName ), (PCH) szNewName );

        if( RenameFile( hwndCnr, pci, (PSZ) szNewName ) )
        {
            (void) strcpy( pci->szFileName, szNewName );

            // Since more than one container can share this record, let all
            // of them know that this record has changed.

            RefreshAllContainers( hwndCnr, pci );
        }
    }

    return;
}

/**********************************************************************/
/*-------------------------- GetMaxNameSize --------------------------*/
/*                                                                    */
/*  GET THE MAXIMUM SIZE OF A FILE NAME FOR A DRIVE.                  */
/*                                                                    */
/*  INPUT: drive letter                                               */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: max filename size                                         */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/

#define QFSBUFFSIZE 100

static ULONG GetMaxNameSize( CHAR chDrive )
{
    APIRET      rc;
    CHAR        szDrive[ 3 ], achBuf[ QFSBUFFSIZE ];
    PFSQBUFFER2 pfsqb = (PFSQBUFFER2) achBuf;
    ULONG       cbFileName = 0, cbBuf = sizeof( achBuf );
    PSZ         szFSDName;

    szDrive[ 0 ] = chDrive;
    szDrive[ 1 ] = ':';
    szDrive[ 2 ] = 0;

    // Get the file system type for this drive (i.e. HPFS, FAT, etc.)

    rc = DosQueryFSAttach( (PCSZ) szDrive, 0, FSAIL_QUERYNAME, (PFSQBUFFER2) achBuf,
                           &cbBuf );

    // Should probably handle ERROR_BUFFER_OVERFLOW more gracefully, but not
    // in this sample program <g>

    if( rc )
        cbFileName = 12;                     // If any errors, assume FAT
    else
    {
        szFSDName = pfsqb->szName + pfsqb->cbName + 1;

        if( !stricmp( "FAT", (const char *) szFSDName ) )
            cbFileName = 12;
        else
            cbFileName = CCHMAXPATH;         // If not FAT, assume maximum path
    }

    return cbFileName;
}

/**********************************************************************/
/*--------------------------- RenameFile -----------------------------*/
/*                                                                    */
/*  RENAME A FILE.                                                    */
/*                                                                    */
/*  INPUT: container window handle,                                   */
/*         pointer to CNRITEM record of the current file,             */
/*         new file name                                              */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL RenameFile( HWND hwndCnr, PCNRITEM pci, PSZ szNewName )
{
    BOOL      fSuccess = TRUE;
    CHAR      szCurrentPath[ CCHMAXPATH + 1 ];
    PCH       pch;
    APIRET    rc;
    PINSTANCE pi = INSTDATA( PARENT( hwndCnr ) );

    if( !pi )
    {
        Msg( (PSZ) "RenameFile cant get Inst data. RC(%X)", HWNDERR( hwndCnr ) );

        return FALSE;
    }

    // Recursively go up the container tree to get the fully qualified path
    // name of the file to be renamed.

    (void) strcpy( szCurrentPath, pi->szDirectory );

    FullyQualify( (PSZ) szCurrentPath, hwndCnr, pci );

    (void) strcpy( pi->achWorkBuf, szCurrentPath );

    // Use the fully qualified path to build the filename that the current
    // file is to be renamed to. In other words, in order to rename
    // d:\path\file.ext to file.ren, we need to do a
    // DosMove( "d:\path\file.ext", "d:\path\file.ren" );
    // So we add file.ren after the last backslash of the current pathname.

     pch = (PCH) strrchr( pi->achWorkBuf, '\\' );

    if( pch )
        *(pch + 1) = 0;
    else
        pi->achWorkBuf[ 0 ] = 0;

    (void) strcat( pi->achWorkBuf, (const char * restrict) szNewName );

    // Do the rename. Alert the user if the rename was not successful. It
    // won't be successful if, for instance, the user tries to change the name
    // to a name that is already used in that directory.

    rc = DosMove( (PCSZ) szCurrentPath, (PCSZ) pi->achWorkBuf );

    if( rc )
    {
        Msg( (PSZ) "DosMove of %s to %s failed! RC(%u)", szCurrentPath,
             pi->achWorkBuf, rc );

        fSuccess = FALSE;
    }

    return fSuccess;
}

/**********************************************************************/
/*----------------------- RefreshAllContainers -----------------------*/
/*                                                                    */
/*  REFRESH ALL CONTAINERS BECAUSE A FILE WAS RENAMED THAT COULD ALSO */
/*  BE IN OTHER CONTAINERS.                                           */
/*                                                                    */
/*  INPUT: container window handle that is being direct-edited,       */
/*         pointer to CNRITEM record of the renamed file              */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID RefreshAllContainers( HWND hwndCnr, PCNRITEM pci )
{
    HWND hwndEnum;

    hwndEnum = WinBeginEnumWindows( HWND_DESKTOP );

    if( hwndEnum )
    {
        HWND hwndFrame, hwndClient;
        CHAR szClass[ 50 ];

        while( fTrue )
        {
            hwndFrame = WinGetNextWindow( hwndEnum );

            if( !hwndFrame )
                break;

            // If we found a frame window (child of the desktop), check to see
            // if it has a client window because that's the one that would have
            // the class that we're looking for

            hwndClient = WinWindowFromID( hwndFrame, FID_CLIENT );

            if( hwndClient )
            {
                // Make sure we are checking NULL-terminated strings

                (void) memset( szClass, 0, sizeof( szClass ) );

                // Get the class of this client window. If it is one of ours,
                // send it a CM_INVALIDATERECORD message so it repaints the
                // renamed record. Since we don't know if this container has
                // the renamed record, ignore the return code.

                if( WinQueryClassName( hwndClient, sizeof(szClass), (PCH) szClass ) )
                    if( !strcmp( szClass, DIRECTORY_WINCLASS ) )
                        WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY,
                                    CM_INVALIDATERECORD, MPFROMP( &pci ),
                                    MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );
            }
        }

        WinEndEnumWindows( hwndEnum );
    }
    else
        Msg( (PSZ) "RefreshAllContainers EnumWindows RC(%X)", HWNDERR( hwndCnr ) );

    return;
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
