/*********************************************************************
 *                                                                   *
 * MODULE NAME :  common.c               			     *
 * 				                                     *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  This module is part of CNRMENU.EXE. It contains common functions *
 *  used by all modules in the EXE.                                  *
 *                                                                   *
 * CALLABLE FUNCTIONS:                                               *
 *                                                                   *
 *  VOID SetWindowTitle( HWND hwndClient, PSZ szFormat, ... );       *
 *  VOID Msg           ( PSZ szFormat, ... );                        *
 *  VOID FullyQualify  ( PSZ szBuf, HWND hwndCnr, PCNRITEM pci );    *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-24-92 - Source copied from CNRBASE.EXE sample.                *
 *             Added sending Msg to stderr besides the msgbox.       *
 *             Added FullyQualify function.                          *
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

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*-------------------------- SetWindowTitle --------------------------*/
/*                                                                    */
/*  SET THE FRAME WINDOW'S TITLEBAR TEXT.                             */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         a message in printf format with its parms                  */
/*                                                                    */
/*  1. Format the message using vsprintf.                             */
/*  2. Set the text into the titlebar.                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/

#define TITLEBAR_TEXTLEN (CCHMAXPATH + 50)

VOID SetWindowTitle( HWND hwndClient, PSZ szFormat,... )
{
    PSZ     szMsg;
    va_list argptr;

    if( (szMsg = (PSZ) malloc( TITLEBAR_TEXTLEN )) == NULL )
    {
        DosBeep( 1000, 1000 );

        return;
    }

    va_start( argptr, szFormat );

    vsprintf( (char * restrict) szMsg, (const char * restrict) szFormat, argptr );

    va_end( argptr );

    szMsg[ TITLEBAR_TEXTLEN - 1 ] = 0;

    (void) WinSetWindowText( PARENT( hwndClient ), szMsg );

    free( szMsg );

    return;
}

/**********************************************************************/
/*------------------------------- Msg --------------------------------*/
/*                                                                    */
/*  DISPLAY A MESSAGE TO THE USER.                                    */
/*                                                                    */
/*  INPUT: a message in printf format with its parms                  */
/*                                                                    */
/*  1. Format the message using vsprintf.                             */
/*  2. Sound a warning sound.                                         */
/*  3. Display the message in a message box.                          */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/

#define MESSAGE_SIZE 1024

VOID Msg( PSZ szFormat,... )
{
    PSZ     szMsg;
    va_list argptr;

    if( (szMsg = (PSZ) malloc( MESSAGE_SIZE )) == NULL )
    {
        DosBeep( 1000, 1000 );

        return;
    }

    va_start( argptr, szFormat );

    vsprintf( (char * restrict) szMsg, (const char * restrict) szFormat, argptr );

    va_end( argptr );

    szMsg[ MESSAGE_SIZE - 1 ] = 0;

    fprintf( stderr, "\n%s", szMsg );

    (void) WinAlarm( HWND_DESKTOP, WA_WARNING );

    (void) WinMessageBox(  HWND_DESKTOP, HWND_DESKTOP, szMsg,
                           (PCSZ) PROGRAM_TITLE, 1, MB_OK | MB_MOVEABLE );

    free( szMsg );

    return;
}

/**********************************************************************/
/*--------------------------- FullyQualify ---------------------------*/
/*                                                                    */
/*  RECURSIVELY BUILD A FULLY QUALIFIED DIRECTORY NAME.               */
/*                                                                    */
/*  INPUT: directory name buffer,                                     */
/*         container window handle,                                   */
/*         pointer to current CNRITEM container record (subdirectory) */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID FullyQualify( PSZ szDirectory, HWND hwndCnr, PCNRITEM pci )
{
    PCNRITEM pciParent = WinSendMsg( hwndCnr, CM_QUERYRECORD, MPFROMP( pci ),
                                     MPFROM2SHORT(CMA_PARENT, CMA_ITEMORDER) );

    if( (INT) pciParent == -1 )
        Msg( (PSZ) "FullyQualify... CM_QUERYRECORD RC(%X)", HWNDERR( hwndCnr ) );
    else
    {
        if( pciParent )
            FullyQualify( szDirectory, hwndCnr, pciParent );

        (void) strcat( (char * restrict) szDirectory, "\\" );

        (void) strcat( (char * restrict) szDirectory, pci->szFileName );
    }

    return;
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
