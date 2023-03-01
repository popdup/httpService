#include "HttpThread.h"
#include <stdio.h>
#include <sstream>
#include <memory>

int main()
{
  std::shared_ptr<HttpThread> pHttpThread = std::make_shared<HttpThread>();
  pHttpThread->Init();
  // here will construct new event when receive new message
  EventInfo *evt = new EventInfo();
  evt->SetRequestType(REQUEST_HTTP_DOWNLOAD);
  evt->SetUrl("https://chenshuo-public.s3.amazonaws.com/pdf/allinone.pdf");
  //
  // the event object will release in HttpThread
  pHttpThread->PutEventInQueue(evt);
  sleep(100);
  pHttpThread->Stop();
}