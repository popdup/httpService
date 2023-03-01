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
  //evt->SetUrl("https://chenshuo-public.s3.amazonaws.com/pdf/allinone.pdf");
  evt->SetUrl("https://img0.baidu.com/it/u=242767209,2541342896&fm=253&fmt=auto&app=138&f=JPEG?w=889&h=500");
  // the event object will release in HttpThread
  pHttpThread->PutEventInQueue(evt);

  //should max life cycle if download large file
  sleep(10);
  pHttpThread->Stop();
}