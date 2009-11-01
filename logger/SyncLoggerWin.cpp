
#include "Unicode.h"
#include "AbstractCallbackFuncModifier.h"
#include "SyncLogger.h"

#if (defined(_WIN32)) || defined(WIN32)

#include <windows.h>
#include <windowsx.h>



typedef struct
{
    DWORD Tick;
} TIMEBEGIN;

UNITEX_FUNC hTimeElasped UNITEX_CALL SyncBuidTimeMarkerObject()
{
    TIMEBEGIN* pBegin = (TIMEBEGIN*)malloc(sizeof(TIMEBEGIN));
    pBegin->Tick = GetTickCount();
    return pBegin;
}


UNITEX_FUNC unsigned int UNITEX_CALL SyncGetMSecElapsed(hTimeElasped ptr)
{
    TIMEBEGIN tBegin;
    TIMEBEGIN tEnd;
    TIMEBEGIN* pBegin = (TIMEBEGIN*)ptr;
    unsigned int iRet;
    tBegin = *pBegin;
    tEnd.Tick = GetTickCount();
    free(pBegin);


    iRet = (unsigned int)(tEnd.Tick - tBegin.Tick) ;
    return iRet;
}



typedef struct
{
    t_thread_func thread_func;
    void* privateDataPtr;
    unsigned int iNbThread;

    DWORD dwThreadId;
} SYNC_THEAD_INFO;

static DWORD WINAPI SyncThreadWrkFunc(LPVOID lpv)
{
    SYNC_THEAD_INFO* ptwi=(SYNC_THEAD_INFO*)lpv;
    (*(ptwi->thread_func))(ptwi->privateDataPtr,ptwi->iNbThread);
    return 0;
}

UNITEX_FUNC int UNITEX_CALL IsSeveralThreadsPossible()
{
    return 1;
}

UNITEX_FUNC void UNITEX_CALL SyncDoRunThreads(unsigned int iNbThread,t_thread_func thread_func,void** privateDataPtrArray)
{
    unsigned int i;
    HANDLE* pHandle=(HANDLE*)malloc(sizeof(HANDLE)*iNbThread);
    SYNC_THEAD_INFO* pThreadInfoArray=(SYNC_THEAD_INFO*)malloc(sizeof(SYNC_THEAD_INFO)*iNbThread);

    for (i=0;i<iNbThread;i++)
    {
        void* privateDataPtr=*(privateDataPtrArray+i);
        (pThreadInfoArray+i)->privateDataPtr = privateDataPtr;
        (pThreadInfoArray+i)->thread_func = thread_func;
        (pThreadInfoArray+i)->iNbThread = i;
        *(pHandle+i) = CreateThread(NULL,0,SyncThreadWrkFunc,(pThreadInfoArray+i),0,&((pThreadInfoArray+i)->dwThreadId));
    }
    WaitForMultipleObjects(iNbThread,pHandle,TRUE,INFINITE);
    free(pHandle);
    free(pThreadInfoArray);
}

/**************************/



/* Mutex implementation for Win32 API */
/* see http://msdn.microsoft.com/en-us/library/ms686927(VS.85).aspx */

UNITEX_FUNC SYNC_Mutex_OBJECT UNITEX_CALL SyncBuildMutex()
{
    HANDLE hMutex;
    hMutex = CreateMutex(NULL,FALSE,NULL);
    if (hMutex == INVALID_HANDLE_VALUE)
        hMutex = NULL;

    return (SYNC_Mutex_OBJECT)hMutex;
}

UNITEX_FUNC void UNITEX_CALL SyncGetMutex(SYNC_Mutex_OBJECT pMut)
{
    HANDLE hMutex = (HANDLE)pMut;
    if (hMutex != NULL)
        WaitForSingleObject(hMutex,INFINITE);
}

UNITEX_FUNC void UNITEX_CALL SyncReleaseMutex(SYNC_Mutex_OBJECT pMut)
{
    HANDLE hMutex = (HANDLE)pMut;
    if (hMutex != NULL)
        ReleaseMutex(hMutex);
}

UNITEX_FUNC void UNITEX_CALL SyncDeleteMutex(SYNC_Mutex_OBJECT pMut)
{
    HANDLE hMutex = (HANDLE)pMut;
    if (hMutex != NULL)
        CloseHandle(hMutex);
}



typedef struct
{
    DWORD dwTls;
} SYNC_TLS_OBJECT_INTERNAL_WIN32;


UNITEX_FUNC SYNC_TLS_OBJECT UNITEX_CALL SyncBuildTls()
{
    SYNC_TLS_OBJECT_INTERNAL_WIN32* pstoi = (SYNC_TLS_OBJECT_INTERNAL_WIN32*)malloc(sizeof(SYNC_TLS_OBJECT_INTERNAL_WIN32));

    if (pstoi != NULL)
    {
        pstoi -> dwTls = TlsAlloc();
        if ((pstoi -> dwTls) == TLS_OUT_OF_INDEXES)
        {
            free(pstoi);
            return NULL;
        }
    }

    return (SYNC_TLS_OBJECT)pstoi;
}

UNITEX_FUNC int UNITEX_CALL SyncTlsSetValue(SYNC_TLS_OBJECT pTls,void* pUsrPtr)
{
    SYNC_TLS_OBJECT_INTERNAL_WIN32* pstoi = (SYNC_TLS_OBJECT_INTERNAL_WIN32*)pTls;
    if (pstoi == NULL)
        return 0;

    if (TlsSetValue(pstoi->dwTls,pUsrPtr))
        return 1;
    else
        return 0;
}

UNITEX_FUNC void* UNITEX_CALL SyncTlsGetValue(SYNC_TLS_OBJECT pTls)
{
    SYNC_TLS_OBJECT_INTERNAL_WIN32* pstoi = (SYNC_TLS_OBJECT_INTERNAL_WIN32*)pTls;
    if (pstoi == NULL)
        return NULL;

    return TlsGetValue(pstoi->dwTls);
}

UNITEX_FUNC void UNITEX_CALL SyncDeleteTls(SYNC_TLS_OBJECT pTls)
{
    SYNC_TLS_OBJECT_INTERNAL_WIN32* pstoi = (SYNC_TLS_OBJECT_INTERNAL_WIN32*)pTls;
    if (pstoi != NULL)
    {
        TlsFree(pstoi->dwTls);
        free(pstoi);
    }
}

#else

#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>


typedef struct
{
    t_thread_func thread_func;
    void* privateDataPtr;
    unsigned int iNbThread;

    DWORD dwThreadId;
} SYNC_THEAD_INFO;

typedef struct
{
    pthread_mutex_t pmt;
} SYNC_Mutex_OBJECT_INTERNAL;








typedef struct
{
    t_thread_func thread_func;
    void* privateDataPtr;
    unsigned int iNbThread;
} SYNC_THEAD_INFO;


static void* SyncThreadWrkFuncPosix(void*pv)
{
    SYNC_THEAD_INFO* ptwi=(SYNC_THEAD_INFO*)pv;
    (*(ptwi->thread_func))(ptwi->privateDataPtr,ptwi->iNbThread);
    pthread_exit(NULL);
    return NULL;
}



UNITEX_FUNC int UNITEX_CALL IsSeveralThreadsPossible()
{
    return 1;
}

UNITEX_FUNC void UNITEX_CALL SyncDoRunThreads(unsigned int iNbThread,t_thread_func thread_func,void** privateDataPtrArray)
{
    unsigned int i;
    pthread_t * pTid=(pthread_t*)malloc(sizeof(pthread_t)*iNbThread);
    SYNC_THEAD_INFO* pThreadInfoArray=(SYNC_THEAD_INFO*)malloc(sizeof(SYNC_THEAD_INFO)*iNbThread);

    for (i=0;i<iNbThread;i++)
    {
        void* privateDataPtr=*(privateDataPtrArray+i);
        (pThreadInfoArray+i)->privateDataPtr = privateDataPtr;
        (pThreadInfoArray+i)->thread_func = thread_func;
        (pThreadInfoArray+i)->iNbThread = i;
        pthread_create(pTid+i,NULL,SyncThreadWrkFuncPosix,(pThreadInfoArray+i));
    }

    

    for (i=0;i<iNbThread;i++)
        pthread_join(*(pTid+i),NULL);

    free(pThreadInfoArray);
    free(pTid);
}


typedef struct
{
    time_t tm_t;
    struct timeval tp;
} TIMEBEGIN;

UNITEX_FUNC hTimeElasped UNITEX_CALL SyncBuidTimeMarkerObject()
{
    TIMEBEGIN* pBegin = (TIMEBEGIN*)malloc(sizeof(TIMEBEGIN));
    pBegin->tm_t = time(NULL);
    gettimeofday(&(pBegin->tp),NULL);
    return (hTimeElasped)pBegin;
}


UNITEX_FUNC unsigned int UNITEX_CALL SyncGetMSecElapsed(hTimeElasped ptr)
{
    TIMEBEGIN tBegin;
    TIMEBEGIN tEnd;
    TIMEBEGIN* pBegin = (TIMEBEGIN*)ptr;
    unsigned int iRet;
    tBegin = *pBegin;
    tEnd.tm_t = time(NULL);
    gettimeofday(&(tEnd.tp),NULL);
    free(pBegin);


    iRet = (unsigned int)(((tEnd.tp.tv_sec*1000) + (tEnd.tp.tv_usec/1000)) -
                 ((tBegin.tp.tv_sec*1000) + (tBegin.tp.tv_usec/1000))) ;
    return iRet;
}

/*
Mutex implementation for Posix API
 (Linux, MacOS X, BSD...)
Documentation about posix thread API
http://manpages.ubuntu.com/manpages/dapper/fr/man3/pthread_mutex_init.3.html
http://developer.apple.com/documentation/Darwin/Reference/Manpages/man3/pthread_mutex_init.3.html
http://www.linux-kheops.com/doc/man/manfr/man-html-0.9/man3/pthread_mutex_init.3.html
*/

typedef struct
{
    pthread_mutex_t pmt;
} SYNC_Mutex_OBJECT_INTERNAL;

UNITEX_FUNC SYNC_Mutex_OBJECT UNITEX_CALL SyncBuildMutex()
{
    int retCreate;
    SYNC_Mutex_OBJECT_INTERNAL* pMoi = (SYNC_Mutex_OBJECT_INTERNAL*)malloc(sizeof(SYNC_Mutex_OBJECT_INTERNAL));
    if (pMoi == NULL)
        return NULL;
    retCreate = pthread_mutex_init(&pMoi->pmt,NULL);
    if (retCreate != 0)
    {
        free(pMoi);
        return NULL;
    }

    return (SYNC_Mutex_OBJECT)pMoi;
}

UNITEX_FUNC void UNITEX_CALL SyncGetMutex(SYNC_Mutex_OBJECT pMut)
{
    SYNC_Mutex_OBJECT_INTERNAL* pMoi = (SYNC_Mutex_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
        pthread_mutex_lock(&pMoi->pmt);
}

UNITEX_FUNC void UNITEX_CALL SyncReleaseMutex(SYNC_Mutex_OBJECT pMut)
{
    SYNC_Mutex_OBJECT_INTERNAL* pMoi = (SYNC_Mutex_OBJECT_INTERNAL*)pMut;
    if (pMut != NULL)
        pthread_mutex_unlock(&pMoi->pmt);
}

UNITEX_FUNC void UNITEX_CALL SyncDeleteMutex(SYNC_Mutex_OBJECT pMut)
{
    SYNC_Mutex_OBJECT_INTERNAL* pMoi = (SYNC_Mutex_OBJECT_INTERNAL*)pMut;
    if (pMoi != NULL)
    {
        pthread_mutex_destroy(&pMoi->pmt);
        free(pMoi);
    }
}




typedef struct
{
    pthread_key_t pthread_key;
} SYNC_TLS_OBJECT_INTERNAL_POSIX;





UNITEX_FUNC SYNC_TLS_OBJECT UNITEX_CALL SyncBuildTls()
{
    SYNC_TLS_OBJECT_INTERNAL_POSIX* pstoi = (SYNC_TLS_OBJECT_INTERNAL_POSIX*)malloc(sizeof(SYNC_TLS_OBJECT_INTERNAL_POSIX));

    if (pstoi != NULL)
    {
        if (pthread_key_create(&pstoi -> pthread_key,NULL) != 0)
        {
            free(pstoi);
            return NULL;
        }
    }

    return (SYNC_TLS_OBJECT)pstoi;
}

UNITEX_FUNC int UNITEX_CALL SyncTlsSetValue(SYNC_TLS_OBJECT pTls,void* pUsrPtr)
{
    SYNC_TLS_OBJECT_INTERNAL_POSIX* pstoi = (SYNC_TLS_OBJECT_INTERNAL_POSIX*)pTls;
    if (pstoi == NULL)
        return 0;

    if (pthread_setspecific(pstoi->pthread_key,pUsrPtr) == 0)
        return 1;
    else
        return 0;
}

UNITEX_FUNC void* UNITEX_CALL SyncTlsGetValue(SYNC_TLS_OBJECT pTls)
{
    SYNC_TLS_OBJECT_INTERNAL_POSIX* pstoi = (SYNC_TLS_OBJECT_INTERNAL_POSIX*)pTls;
    if (pstoi == NULL)
        return NULL;

    return pthread_getspecific(pstoi->pthread_key);
}

UNITEX_FUNC void UNITEX_CALL SyncDeleteTls(SYNC_TLS_OBJECT pTls)
{
    SYNC_TLS_OBJECT_INTERNAL_POSIX* pstoi = (SYNC_TLS_OBJECT_INTERNAL_POSIX*)pTls;
    if (pstoi != NULL)
    {
        pthread_key_delete(pstoi->dwTls);
        free(pstoi);
    }
}
#endif

