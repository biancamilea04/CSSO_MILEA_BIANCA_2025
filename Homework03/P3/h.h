//
// Created by Bianca Milea on 11/3/2025.
//

void verifyPidStillExist(int pid) {

    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if(hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printf("CreateToolhelp32Snapshot failed.err = %d \n",GetLastError());
        ErrorExit();
    }

    pe32.dwSize = sizeof( PROCESSENTRY32 );

    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        printf("ErrorProcess32First : %d \n", GetLastError() );
        CloseHandle( hProcessSnap );
        ErrorExit();
    }
    do
    {
       if (pe32.th32ProcessID == pid) {
            printf("Process with PID %d still exists.\n", pid);
            CloseHandle(hProcessSnap);
            return;
        }

    }
    while(Process32Next(hProcessSnap, &pe32 ));

    printf("Process with PID %d does not exists.\n", pid);

    CloseHandle(hProcessSnap);
}
