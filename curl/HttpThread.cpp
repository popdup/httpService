#include "HttpThread.h"
#include "pthread.h"
#include <memory>
const int MaxRecvDataLen =64 * 1024; 
const long Max_SimuTrans_In_CurlMulti = 500;
#define CURL_MAX_WAIT_MSECS 500000
 
typedef std::shared_ptr<FILE> FilePtr;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{ 
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

HttpThread::HttpThread() 
{ 
    std::cout << "HttpThread::HttpThread" << std::endl; 
    m_bStart = false; 
    m_dwThreadID = 0; 
} 

HttpThread::~HttpThread() 
{ 
    std::lock_guard<mutex> m_guard(m_mutex); 
    while(!m_eventQueue.empty()) 
    { 
        EventInfo *event = m_eventQueue.front(); 
        m_eventQueue.pop();   
        delete event; 
    }  
} 

bool HttpThread::Init() 
{ 
    int ret; 
    m_bStart = true; 
    ret = pthread_create(&m_dwThreadID, NULL, OnThreadRun, this); 
    if(ret != 0) 
    { 
        std::cout <<"Can not create httpthread"<< std::endl; 
        m_bStart = false; 
        return false; 
    } 
     
    return true; 
} 
void HttpThread::Stop() 
{ 
    std::cout <<"Http Thread is stopping"<< std::endl; 
    m_bStart = false; 
    if(m_dwThreadID > 0) 
    { 
        pthread_cancel(m_dwThreadID); 
        pthread_join(m_dwThreadID, NULL); 
        std::cout <<"Http Thread is stopped,threadid="<<m_dwThreadID<< std::endl; 
    } 
    else 
        std::cout <<"HttpThread Stop, invalid thread ID"<< std::endl; 
}

void* HttpThread::OnThreadRun(void* arg) 
{ 
    HttpThread *pThread = static_cast<HttpThread *>(arg); 
    pThread->m_lHandleCountInStack = 0; 
    pThread->m_multi_handle = curl_multi_init(); 
    curl_multi_setopt(pThread->m_multi_handle,  CURLMOPT_MAXCONNECTS,            Max_SimuTrans_In_CurlMulti); 
    curl_multi_setopt (pThread->m_multi_handle, CURLMOPT_MAX_CONCURRENT_STREAMS, Max_SimuTrans_In_CurlMulti); 
    while(pThread->m_bStart) 
    { 
        pThread->AddEasyHandler(); 
        if(0 == pThread->m_lHandleCountInStack) 
        { 
            usleep(1000);
            continue; 
        } 
        pThread->PerformTransfer(); 
        pThread->RemoveEasyHandler();
    } 
    std::cout<< "HttpThread::OnThreadRun: Http Thread is stopping"<< std::endl; 
    curl_multi_cleanup(pThread->m_multi_handle); 
    return (void*) NULL; 

} 

void HttpThread::AddEasyHandler() 
{ 
    int nMaxToRetrieve = Max_SimuTrans_In_CurlMulti - m_lHandleCountInStack; 
    if(nMaxToRetrieve <= 0) 
        return; 
     
    list<EventInfo *> events; 
    this->GetEventsFromQueue(events, nMaxToRetrieve); 
    list<EventInfo *>::iterator it = events.begin(); 
    for (; it != events.end(); ++ it) 
    { 
        EventInfo * eventInfo = static_cast<EventInfo *>(*it);     
        if(eventInfo) 
        { 
            AddOneEasyHandler(eventInfo); 
            delete eventInfo; 
            eventInfo = NULL; 
        } 
    } 
    events.clear(); 
}

void HttpThread::AddOneEasyHandler(EventInfo* eventInfo) 
{ 
    int res = 0; 
    if(!eventInfo) 
        return; 

    CURL *curl = curl_easy_init(); 
    PrepareCommonOPT(curl); 
    PrepareCurl(curl,eventInfo); 
    curl_multi_add_handle(m_multi_handle, curl); 
    m_lHandleCountInStack ++;  
}

void HttpThread::PrepareCommonOPT(CURL * pCurl) 
{ 
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(pCurl, CURLOPT_SSL_VERIFYHOST, 0L); 
    curl_easy_setopt(pCurl, CURLOPT_FORBID_REUSE, 0); 
} 

void HttpThread::PrepareCurl(CURL* pCurl,EventInfo* eventInfo) 
{
    if(!pCurl || !eventInfo) 
        return;
    
    switch(eventInfo->GetRequestType()) 
    { 
        case REQUEST_HTTP_GET: 
            break; 
        case REQUEST_HTTP_POST: 
            break; 
        case REQUEST_HTTP_DOWNLOAD:
            std::cout <<"case REQUEST_HTTP_DOWNLOAD:" << std::endl;
            PrepareRequestHttpDownload(pCurl,eventInfo); 
            break; 
        case REQUEST_FTP:
            break; 
        default: 
            std::cout <<"Invalid request type. Request type is :" << eventInfo->GetRequestType()<< std::endl; 
            break; 
    } 
}

void HttpThread::PrepareRequestHttpDownload(CURL* pCurl,EventInfo* eventInfo) 
{ 
    if(!pCurl || !eventInfo) 
        return;
    
    string url = eventInfo->GetUrl();
    std::cout << "url="  << url << std::endl;
    /* set URL to get here */
    curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str()); 
  
    /* Switch on full protocol/debug output while testing */
    curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);
    
    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(pCurl, CURLOPT_NOPROGRESS, 1L);
    
    /* send all data to this function  */
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, write_data);

    /* open the file */
    FILE *pagefile = fopen("./1.pdf", "wb");

    if(pagefile) {
        //FilePtr(pagefile,::fclose);
        /* write the page body to this file handle */
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, pagefile);
    }

} 

int HttpThread::PerformTransfer() {
    std::cout<< "PerformTransfer" << std::endl; 

    int nStillRunning = -1, nReady = -1; 
    CURLMcode ret = curl_multi_perform(m_multi_handle, &nStillRunning); 
     
    /* wait for activity or timeout since we have remaining handlers */  
    if (nStillRunning > 0) 
    { 
        std::cout<< "nStillRunning >0" << std::endl; 

        curl_multi_poll(m_multi_handle, NULL, 0, CURL_MAX_WAIT_MSECS, &nReady); 
    } 
    else if(ret != CURLM_OK) 
    { 
        std::cout<< "curl_multi_perform, ret: " << ret << std::endl; 
        usleep(1000);
    } 
    return 0; 
}

int  HttpThread::RemoveEasyHandler() 
{
    std::cout<< "RemoveEasyHandler"  << std::endl; 

    CURLMsg          * msg; 
    int                nMsgCountInQueue; 
    int nRemovedHandlers = 0; 
    //remove easy_handler from m_multi_handler one by one 
    while (msg = curl_multi_info_read(m_multi_handle, &nMsgCountInQueue)) 
    { 
        if (msg->msg == CURLMSG_DONE)  
        { 
            RemoveOneEasyHandler(msg); 
            ++ nRemovedHandlers; 
        } 
    } 
     
    m_lHandleCountInStack -= nRemovedHandlers; 
    return 0; 
} 

void  HttpThread::RemoveOneEasyHandler(CURLMsg * msg) 
{ 
    std::cout << "RemoveOneEasyHandler" << std::endl;
    CURL *easy_handle = msg->easy_handle; 
    long rspCode; 
    int RequestType=0; 
    curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &rspCode);
    if(rspCode > 199 && rspCode <300)//response code is 2xx it is sucess 
    { 
        std::cout <<"success " << std::endl;
    } 
    else 
    { 
        std::cout << "HttpThread::response fails, http response code=" << rspCode << std::endl;
    } 
    curl_multi_remove_handle(m_multi_handle, easy_handle); 
    curl_easy_cleanup(easy_handle); 
} 

void HttpThread::PutEventInQueue(EventInfo * event) 
{ 
    std::lock_guard<mutex> m_guard(m_mutex); 
    m_eventQueue.push(event); 
}

EventInfo * HttpThread::GetEventFromQueue() 
{ 
    std::lock_guard<mutex> m_guard(m_mutex); 
    if(!m_eventQueue.empty()) 
    { 
        EventInfo *event = m_eventQueue.front(); 
        m_eventQueue.pop(); 
        return event; 
    } 
    std::cout<<"HttpThread::EventQueue is empty"<< std::endl; 
    return NULL; 
}

void HttpThread::GetEventsFromQueue(list<EventInfo *> & events, int nMaxToRetrieve) 
{ 
    lock_guard<mutex> m_guard(m_mutex); 
    while(!m_eventQueue.empty() && nMaxToRetrieve > 0) 
    { 
        EventInfo *event = m_eventQueue.front(); 
        m_eventQueue.pop(); 
        events.push_back(event); 
        -- nMaxToRetrieve; 
    } 
} 