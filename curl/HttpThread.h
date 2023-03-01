
#ifndef __HTTPTHREAD_H__
#define __HTTPTHREAD_H__
#include "curl/curl.h"
#include <queue>
#include <list>
#include <mutex>
#include "stdio.h" 
#include <string> 
#include <iostream> 
#include <sstream> 
#include "curl/curl.h"
 #include <unistd.h>
 
using namespace std;

enum RequestType{
REQUEST_HTTP_GET,
REQUEST_HTTP_POST,
REQUEST_HTTP_DOWNLOAD,
REQUEST_FTP
};

class EventInfo;
class HttpThread
{
public:
    HttpThread();
    virtual ~HttpThread();
    static void *OnThreadRun(void *arg);
    void PutEventInQueue(EventInfo *event);
    EventInfo *GetEventFromQueue();
    void GetEventsFromQueue(list<EventInfo *> &events, int nMaxToRetrieve);

    bool Init();
    void AddEasyHandler();
    void AddOneEasyHandler(EventInfo *eventInfo);
    int PerformTransfer();
    int RemoveEasyHandler();
    void RemoveOneEasyHandler(CURLMsg *msg);
    void PrepareCommonOPT(CURL *pCurl);
    void PrepareRequestHttpDownload(CURL *pCurl, EventInfo *eventInfo);
    void PrepareCurl(CURL *pCurl, EventInfo *eventInfo);
    void Stop();

private:
    pthread_t m_dwThreadID;
    std::queue<EventInfo *> m_eventQueue;
    unsigned long m_dwDuringTime;
    bool m_bStart;
    int m_lHandleCountInStack;
    CURLM *m_multi_handle;
    mutex m_mutex;
};

// TODO also should inlcude more detail info to extend
class EventInfo{
public:
    EventInfo(){}
    ~EventInfo(){}
    RequestType GetRequestType(){return m_requestType;}
    void SetRequestType(RequestType reqType){m_requestType = reqType;}
    string GetUrl(){return m_url;}
    void SetUrl(string url){m_url = url;}

private:
    RequestType m_requestType;
    string m_url;
};

#endif