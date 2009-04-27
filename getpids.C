/* ######################################################################### */
/* ##                                                                     ## */
/* ##  G E T P I D S . C P P                                              ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Job .......: Starting with the current Process ID, the tool        ## */
/* ##               climbs up the process hierarchy and displays the      ## */
/* ##               Parent Process ID, parent's Parent Process ID,...     ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Copyright (C) 2003  Daniel Scheibli                                ## */
/* ##                                                                     ## */
/* ##  This program is free software; you can redistribute it and/or      ## */
/* ##  modify it under the terms of the GNU General Public License        ## */
/* ##  as published by the Free Software Foundation; either version 2     ## */
/* ##  of the License, or (at your option) any later version.             ## */
/* ##                                                                     ## */
/* ##  This program is distributed in the hope that it will be useful,    ## */
/* ##  but WITHOUT ANY WARRANTY; without even the implied warranty of     ## */
/* ##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      ## */
/* ##  GNU General Public License for more details.                       ## */
/* ##                                                                     ## */
/* ##  You should have received a copy of the GNU General Public License  ## */
/* ##  along with this program; if not, write to the Free Software        ## */
/* ##  Foundation, Inc., 59 Temple Place - Suite 330, Boston,             ## */
/* ##  MA  02111-1307, USA.                                               ## */
/* ##                                                                     ## */
/* ## ------------------------------------------------------------------- ## */
/* ##                                                                     ## */
/* ##  Author ....: Daniel Scheibli <daniel@scheibli.com>                 ## */
/* ##  Date ......: 2003-01-15                                            ## */
/* ##  Changes ...: ----- getpids.cpp 1.00 ------------------------------ ## */
/* ##               2009-03-13 o FEATURE: Modified code to take an        ## */
/* ##                            arbitrary pid.
/* ##               2008-02-09 o FEATURE: Added the option to display     ## */
/* ##                            the PID of a no longer existing parent   ## */
/* ##                            process.                                 ## */
/* ##                          o CLEANUP: Removed the trailing blank in   ## */
/* ##                            the program output.                      ## */
/* ##               ----- getpids.cpp 0.99.002 -------------------------- ## */
/* ##               2006-09-05 o BUGFIX: Parent PID's are now only        ## */
/* ##                            printed, if they are still occupied      ## */
/* ##                            by the original parent process. This     ## */
/* ##                            has to be checked, as the Windows OS     ## */
/* ##                            is extensively reusing PID's. That       ## */
/* ##                            way we also prevent "look like loops"    ## */
/* ##                            in the process hierarchy.                ## */
/* ##               2006-09-04 o BUGFIX: Corrected return code behaviour  ## */
/* ##                            ensuring that != 0 is only returned in   ## */
/* ##                            real error situations (opening a process ## */
/* ##                            to query might also fail due to the fact ## */
/* ##                            that the process does no longer exist).  ## */
/* ##               ----- getpids.c 0.99.001 ---------------------------- ## */
/* ##               <none/>                                               ## */
/* ##                                                                     ## */
/* ######################################################################### */ 


#ifdef Q_WS_WIN

#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <windows.h>


using namespace std;

namespace GetPIDs {

struct ProcessBasicInformation {
   DWORD ExitStatus;
   PVOID PebBaseAddress;
   DWORD AffinityMask;
   DWORD BasePriority;
   ULONG UniqueProcessId;
   ULONG InheritedFromUniqueProcessId;
};

struct KernelUserTimes {
   LONGLONG CreateTime;
   LONGLONG ExitTime;
   LONGLONG KernelTime;
   LONGLONG UserTime;
};


int iArgDisplayAncestor = FALSE;
ULONG ulQueryPid = 0;

int parseargs( int argc, char *argv[] );
int queryprocess( ULONG pid, ULONG *ppid, LONGLONG *createtime );
void usage( void );





/* ######################################################################## */
/* ##                                                                    ## */
/* ##   << M A I N >>                                                    ## */
/* ##                                                                    ## */
/* ######################################################################## */

int main( int argc, char *argv[] ) {

    // Presettings
    ULONG         ulProcessPpid;
    ULONG         ulChildProcessPpid;
    LONGLONG      llProcessCreateTime;
    LONGLONG      llChildProcessCreateTime;	
    int           I, iRc;
    std::vector<int> familyTree;

    if (parseargs( argc, argv ) != 0 ) {
       usage( );
       return( 1 );
    }
	
    if (ulQueryPid == 0) {
       ulQueryPid = GetCurrentProcessId( );
    }


    // Collect initial data based on the current process

   if ( queryprocess( ulQueryPid, &ulChildProcessPpid, 
      &llChildProcessCreateTime ) == 1 ) {
      //cout << ulQueryPid;
      familyTree.push_back(ulQueryPid);
      ulQueryPid = ulChildProcessPpid;
   }else {
      return( 1 );
   }



	// Collect and print the process hierarchy data by climbing up the tree

	while( 1 ) {
	
		// Collect data for the given process
		
		iRc = queryprocess( ulQueryPid, &ulProcessPpid, &llProcessCreateTime );
		
		if( iRc == 0 ) {
			if( iArgDisplayAncestor ) {
				cout << " -" << ulQueryPid;
			}
			break;
		}
		else if( iRc == 1 ) {

			// Ensure that the queries process wasn't created after its 
			// child process (was queried in the loop iteration before).

			// NOTE: The Windows operating system is extensively reusing the 
			//       process id's (without sequentially increasing the PID).
			//       This leads to a situation where the process hierarchy
			//       can not be trusted without additional tests. 
			//       Say we have the hierarchy C <- B <- A. In this case 
			//       process C was created by process B which had process 
			//       A as parent. Process A also has a parent which we 
			//       will call N for now. It might well be that process N 
			//       is no longer arround, but instead another process has
			//       been created and is using the same PID as process N 
			//       did. It might even be that this new process is process
 			//       C which would look a loop in the process hierarchy:
			//       C <- B <- A <- C <- A ...	
			
			if( llChildProcessCreateTime < llProcessCreateTime ) {
				if( iArgDisplayAncestor ) {
					cout << " -" << ulQueryPid;
				}
				break;
			}
			else {
				//cout << " " << ulQueryPid;
                                familyTree.push_back(ulQueryPid);
				
				ulChildProcessPpid       = ulProcessPpid;
  				llChildProcessCreateTime = llProcessCreateTime;
		
				ulQueryPid = ulProcessPpid;
			}
		}
		else {
			cout << endl;
			return( 1 );
		}
	}

	cout << endl;



	// End program

       for (int i = 0; i < familyTree.size(); ++i) {
           cout << i << "  " << familyTree[i] << endl;
       }
	return( 0 );
}





/* ######################################################################## */
/* ##                                                                    ## */
/* ##   p a r s e a r g s ( )                                            ## */
/* ##                                                                    ## */
/* ######################################################################## */

int parseargs( int argc, char *argv[] ) {
    while( 1 ) {
       switch( getopt( argc, argv, "xp:" ) ) {
          // Covers the number of threads
          case 'x':  iArgDisplayAncestor = TRUE; break;
          case 'p':  ulQueryPid = atoi(optarg); break;
          case -1:   return( 0 );  // End of argument list
          case '?':
          case ':':
          default:   return( 1 );
       }
    }
}





/* ######################################################################## */
/* ##                                                                    ## */
/* ##   q u e r y p r o c e s s ( )                                      ## */
/* ##                                                                    ## */
/* ######################################################################## */

int queryprocess( ULONG pid, ULONG *ppid, LONGLONG *createtime ) {

	// Presettings
	
	HANDLE hProcess = NULL;

	struct ProcessBasicInformation sProcessInfo;
	struct KernelUserTimes	       sProcessTime;



	// TODO: Do function lookup only once.
	
	typedef LONG ( __stdcall *FPTR_NtQueryInformationProcess ) ( HANDLE, INT, PVOID, ULONG, PULONG );
	FPTR_NtQueryInformationProcess NtQueryInformationProcess = ( FPTR_NtQueryInformationProcess ) GetProcAddress( GetModuleHandle( "ntdll" ), "NtQueryInformationProcess" );



	// Query the process and collect 
	// (a) the Parent Process ID
	// (b) the the process creation time
		
	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, pid );
	if( hProcess == NULL ) {
		return( 0 );
	}

	if( NtQueryInformationProcess( hProcess, 0, (void *) &sProcessInfo, sizeof( sProcessInfo ), NULL ) != 0 ) {
		CloseHandle( hProcess );
		return( -1 );
	}

	if( NtQueryInformationProcess( hProcess, 4, (void *) &sProcessTime, sizeof( sProcessTime ), NULL ) != 0 ) {
		CloseHandle( hProcess );
		return( -1 );
	}

	*ppid       = sProcessInfo.InheritedFromUniqueProcessId;
	*createtime = sProcessTime.CreateTime;
	
	return( 1 );
}





/* ######################################################################## */
/* ##                                                                    ## */
/* ##   u s a g e ( )                                                    ## */
/* ##                                                                    ## */
/* ######################################################################## */

void usage( void ) {

	cerr << endl;
	cerr << "USAGE: getpids [OPTIONS]" << endl;
	cerr << endl;
	cerr << "       -x   Display PID of a no longer existing parent."       << endl;
	cerr << "            Given the default output D C B, the altered ouput" << endl;
	cerr << "            becomes D C B -A (please note the leading dash in" << endl;
	cerr << "            front the no longer existing parent's PID)."       << endl;
	cerr << endl;
}

} // end namespace GetPIDs

#endif

