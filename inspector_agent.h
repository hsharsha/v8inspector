#ifndef SRC_INSPECTOR_AGENT_H_
#define SRC_INSPECTOR_AGENT_H_

#include <memory>
#include <string>
#include "v8.h"
#include "v8-inspector.h"

#include <stddef.h>

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
   __attribute__((visibility("default"))) Agent(std::string host_name, std::string file_path);
  __attribute__((visibility("default"))) ~Agent();

  // Create client_, may create io_ if option enabled
  __attribute__((visibility("default"))) bool Start(Isolate* isolate, Platform* platform, const char* path);
  // Stop and destroy io_
  __attribute__((visibility("default"))) void Stop();

  bool IsStarted() { return !!client_; }

  // IO thread started, and client connected
  bool IsConnected();


  void WaitForDisconnect();
  void FatalException(Local<Value> error,
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
  __attribute__((visibility("default"))) void PauseOnNextJavascriptStatement(const std::string& reason);

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
};

}  // namespace inspector

#endif  // SRC_INSPECTOR_AGENT_H_
