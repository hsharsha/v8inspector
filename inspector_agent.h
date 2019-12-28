/*
*    Copyright Node.js contributors. All rights reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining a copy
*    of this software and associated documentation files (the "Software"), to
*    deal in the Software without restriction, including without limitation the
*    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*    sell copies of the Software, and to permit persons to whom the Software is
*    furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*    IN THE SOFTWARE.
*/

#ifndef SRC_INSPECTOR_AGENT_H_
#define SRC_INSPECTOR_AGENT_H_

#include <memory>
#include <string>
#include <functional>
#include "v8.h"
#include "v8-inspector.h"

#include <stddef.h>

#ifdef WIN32
#define EXPORT_ATTRIBUTE __declspec(dllexport) 
#else
#define EXPORT_ATTRIBUTE __attribute__((visibility("default"))) 
#endif


namespace inspector {



using namespace v8;

class InspectorSessionDelegate {
 public:
  virtual ~InspectorSessionDelegate() = default;
  virtual bool WaitForFrontendMessageWhilePaused() = 0;
  virtual void SendMessageToFrontend(const v8_inspector::StringView& message)
                                     = 0;
};

class InspectorIo;
class CBInspectorClient;

class Agent {
 public:

  EXPORT_ATTRIBUTE  static void SetLogFileStream(FILE *file);
  EXPORT_ATTRIBUTE  Agent(const std::string &host_name, const std::string &file_path, const std::string &target_id = std::string()); 
  EXPORT_ATTRIBUTE  ~Agent();

  // Create client_, may create io_ if option enabled
  EXPORT_ATTRIBUTE  bool Start(Isolate* isolate, Platform* platform, const char* file_path = nullptr)
  {
    if(! Prepare(isolate, platform, file_path))
        return false;
    return Run();
  }

  EXPORT_ATTRIBUTE  bool Prepare(Isolate* isolate, Platform* platform, const char* file_path = nullptr);
  EXPORT_ATTRIBUTE  bool Run();
  EXPORT_ATTRIBUTE  const std::string &GetFrontendURL();
  // Stop and destroy io_
  EXPORT_ATTRIBUTE  void Stop();

  bool IsStarted() { return !!client_; }
  bool IsValid();

  // IO thread started, and client connected
  bool IsConnected();


  void WaitForDisconnect();
   EXPORT_ATTRIBUTE  void FatalException(Local<Value> error,
                      v8::Local<v8::Message> message);

  // These methods are called by the WS protocol and JS binding to create
  // inspector sessions.  The inspector responds by using the delegate to send
  // messages back.
  void Connect(InspectorSessionDelegate* delegate);
  void Disconnect();
  void Dispatch(const v8_inspector::StringView& message);
  InspectorSessionDelegate* delegate();

  void RunMessageLoop();
  bool enabled() { return enabled_; }
  EXPORT_ATTRIBUTE  void PauseOnNextJavascriptStatement(const std::string& reason);

  // Initialize 'inspector' module bindings
  static void InitInspector(Local<Object> target,
                            Local<Value> unused,
                            Local<Context> context,
                            void* priv);

  InspectorIo* io() {
    return io_.get();
  }

  // Can only be called from the the main thread.
  bool StartIoThread(bool wait_for_connect);

  // Calls StartIoThread() from off the main thread.
  void RequestIoThreadStart();

 private:
  std::unique_ptr<CBInspectorClient> client_;
  std::unique_ptr<InspectorIo> io_;
  Platform* platform_;
  Isolate* isolate_;
  bool enabled_;
  std::string path_;
  std::string host_name_;
  std::string file_path_;
  std::string target_id_;

  std::string frontend_url_buff_;
  static const int VALID_MAGIC = 0xF0F0F0F0;
  static const int   BAD_MAGIC = 0xDE11C0DE;
  int magic_ = VALID_MAGIC;
};

}  // namespace inspector

#endif  // SRC_INSPECTOR_AGENT_H_
