#ifndef SRC_INSPECTOR_AGENT_H_
#define SRC_INSPECTOR_AGENT_H_

#include <memory>
#include "v8.h"
#include "v8-inspector.h"
#include "env.h"

#include <stddef.h>

namespace inspector {

class InspectorSessionDelegate {
 public:
  virtual ~InspectorSessionDelegate() = default;
  virtual bool WaitForFrontendMessageWhilePaused() = 0;
  virtual void SendMessageToFrontend(const v8_inspector::StringView& message)
                                     = 0;
};

class InspectorIo;
class NodeInspectorClient;

class Agent {
 public:
   Agent();
  ~Agent();

  // Create client_, may create io_ if option enabled
  bool Start(Environment *env, v8::Platform* platform, const char* path);
  // Stop and destroy io_
  void Stop();

  bool IsStarted() { return !!client_; }

  // IO thread started, and client connected
  bool IsConnected();


  void WaitForDisconnect();
  void FatalException(v8::Local<v8::Value> error,
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
  void PauseOnNextJavascriptStatement(const std::string& reason);

  // Initialize 'inspector' module bindings
  static void InitInspector(v8::Local<v8::Object> target,
                            v8::Local<v8::Value> unused,
                            v8::Local<v8::Context> context,
                            void* priv);

  InspectorIo* io() {
    return io_.get();
  }

  // Can only be called from the the main thread.
  bool StartIoThread(bool wait_for_connect);

  // Calls StartIoThread() from off the main thread.
  void RequestIoThreadStart();

 private:
  std::unique_ptr<NodeInspectorClient> client_;
  std::unique_ptr<InspectorIo> io_;
  v8::Platform* platform_;
  bool enabled_;
  std::string path_;
  Environment *parent_env_;
};

}  // namespace inspector

#endif  // SRC_INSPECTOR_AGENT_H_
