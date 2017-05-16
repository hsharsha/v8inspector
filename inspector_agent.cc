#include "inspector_agent.h"

#include "inspector_socket_server.h"
#include "v8-inspector.h"
#include "v8-platform.h"
#include "zlib.h"

#include "libplatform/libplatform.h"

#include <cassert>
#include <condition_variable>
#include <mutex>
#include <map>
#include <sstream>
#include <tuple>
#include <unicode/unistr.h>

#include <string.h>
#include <utility>
#include <vector>
#include <openssl/rand.h>


namespace {
inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const char* data,
                                           int length = -1) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::NewStringType::kNormal,
                                    length).ToLocalChecked();
}

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const signed char* data,
                                           int length = -1) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::NewStringType::kNormal,
                                    length).ToLocalChecked();
}

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate,
                                           const unsigned char* data,
                                           int length = -1) {
  return v8::String::NewFromOneByte(isolate,
                                    reinterpret_cast<const uint8_t*>(data),
                                    v8::NewStringType::kNormal,
                                    length).ToLocalChecked();
}
using v8_inspector::StringBuffer;
using v8_inspector::StringView;

std::string GetProcessTitle() {
  // uv_get_process_title will trim the title if it is too long.
  char title[2048];
  int err = uv_get_process_title(title, sizeof(title));
  if (err == 0) {
    return title;
  } else {
    return "Node.js";
  }
}

// UUID RFC: https://www.ietf.org/rfc/rfc4122.txt
// Used ver 4 - with numbers
std::string GenerateID() {
  uint8_t buffer[16];
  RAND_bytes(buffer, sizeof(buffer));
  char uuid[256];
  snprintf(uuid, sizeof(uuid), "%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
           buffer[0],  // time_low
           buffer[1],  // time_mid
           buffer[2],  // time_low
           (buffer[3] & 0x0fff) | 0x4000,  // time_hi_and_version
           (buffer[4] & 0x3fff) | 0x8000,  // clk_seq_hi clk_seq_low
           buffer[5],  // node
           buffer[6],
           buffer[7]);
  return uuid;
}

std::string StringViewToUtf8(const StringView& view) {
  if (view.is8Bit()) {
    return std::string(reinterpret_cast<const char*>(view.characters8()),
                       view.length());
  }
  const uint16_t* source = view.characters16();
  const UChar* unicodeSource = reinterpret_cast<const UChar*>(source);
  static_assert(sizeof(*source) == sizeof(*unicodeSource),
                "sizeof(*source) == sizeof(*unicodeSource)");

  size_t result_length = view.length() * sizeof(*source);
  std::string result(result_length, '\0');
  UnicodeString utf16(unicodeSource, view.length());
  // ICU components for std::string compatibility are not enabled in build...
  bool done = false;
  while (!done) {
    CheckedArrayByteSink sink(&result[0], result_length);
    utf16.toUTF8(sink);
    result_length = sink.NumberOfBytesAppended();
    result.resize(result_length);
    done = !sink.Overflowed();
  }
  return result;
}

std::unique_ptr<StringBuffer> Utf8ToStringView(const std::string& message) {
  UnicodeString utf16 =
      UnicodeString::fromUTF8(StringPiece(message.data(), message.length()));
  StringView view(reinterpret_cast<const uint16_t*>(utf16.getBuffer()),
                  utf16.length());
  return StringBuffer::create(view);
}

}  // namespace

class V8NodeInspector;

enum class InspectorAction {
  kStartSession, kEndSession, kSendMessage
};

enum class TransportAction {
  kSendMessage, kStop
};

class AgentImpl;

class InspectorAgentDelegate: public SocketServerDelegate {
 public:
  InspectorAgentDelegate(AgentImpl* agent, const std::string& script_path,
                         const std::string& script_name, bool wait);
  bool StartSession(int session_id, const std::string& target_id) override;
  void MessageReceived(int session_id, const std::string& message) override;
  void EndSession(int session_id) override;
  std::vector<std::string> GetTargetIds() override;
  std::string GetTargetTitle(const std::string& id) override;
  std::string GetTargetUrl(const std::string& id) override;
  bool IsConnected() { return connected_; }
 private:
  AgentImpl* agent_;
  bool connected_;
  int session_id_;
  const std::string script_name_;
  const std::string script_path_;
  const std::string target_id_;
  bool waiting_;
};

class AgentImpl {
 public:
  explicit AgentImpl(v8::Isolate *isolate);

  // Start the inspector agent thread
  bool Start(v8::Platform* platform, const char* path);
  // Stop the inspector agent
  void Stop();

  bool IsStarted();
  bool IsConnected();
  void WaitForDisconnect();

  void FatalException(v8::Local<v8::Value> error,
                      v8::Local<v8::Message> message);

  void PostIncomingMessage(InspectorAction action, int session_id,
                           const std::string& message);
  void ResumeStartup() {
    uv_sem_post(&start_sem_);
  }

 private:
  template <typename Action>
  using MessageQueue =
      std::vector<std::tuple<Action, int, std::unique_ptr<StringBuffer>>>;
  enum class State { kNew, kAccepting, kConnected, kDone, kError };

  static void ThreadCbIO(void* agent);
  static void WriteCbIO(uv_async_t* async);
  static void MainThreadAsyncCb(uv_async_t* req);

  void InstallInspectorOnProcess();

  void WorkerRunIO();
  void SetConnected(bool connected);
  void DispatchMessages();
  void Write(TransportAction action, int session_id, const StringView& message);
  template <typename ActionType>
  bool AppendMessage(MessageQueue<ActionType>* vector, ActionType action,
                     int session_id, std::unique_ptr<StringBuffer> buffer);
  template <typename ActionType>
  void SwapBehindLock(MessageQueue<ActionType>* vector1,
                      MessageQueue<ActionType>* vector2);
  void WaitForFrontendMessage();
  void NotifyMessageReceived();
  State ToState(State state);

  v8::Isolate* isolate_;
  uv_sem_t start_sem_;
  uv_loop_t* inspector_event_loop;
  std::condition_variable incoming_message_cond_;
  std::mutex state_lock_;
  uv_thread_t thread_;
  uv_loop_t child_loop_;

  InspectorAgentDelegate* delegate_;
  bool wait_;
  bool shutting_down_;
  State state_;

  uv_async_t io_thread_req_;
  uv_async_t main_thread_req_;
  V8NodeInspector* inspector_;
  v8::Platform* platform_;
  MessageQueue<InspectorAction> incoming_message_queue_;
  MessageQueue<TransportAction> outgoing_message_queue_;
  bool dispatching_messages_;
  int session_id_;
  InspectorSocketServer* server_;

  std::string script_name_;
  std::string script_path_;
  const std::string id_;

  friend class ChannelImpl;
  friend class DispatchOnInspectorBackendTask;
  friend class SetConnectedTask;
  friend class V8NodeInspector;
  friend void InterruptCallback(v8::Isolate*, void* agent);
  friend void DataCallback(uv_stream_t* stream, ssize_t read,
                           const uv_buf_t* buf);
};

void InterruptCallback(v8::Isolate*, void* agent) {
  static_cast<AgentImpl*>(agent)->DispatchMessages();
}

class DispatchOnInspectorBackendTask : public v8::Task {
 public:
  explicit DispatchOnInspectorBackendTask(AgentImpl* agent) : agent_(agent) {}

  void Run() override {
    agent_->DispatchMessages();
  }

 private:
  AgentImpl* agent_;
};

class ChannelImpl final : public v8_inspector::V8Inspector::Channel {
 public:
  explicit ChannelImpl(AgentImpl* agent): agent_(agent) {}
  virtual ~ChannelImpl() {}
 private:
void sendResponse(
      int callId,
      std::unique_ptr<v8_inspector::StringBuffer> message) override {
    sendMessageToFrontend(message->string());
  }

  void sendNotification(
      std::unique_ptr<v8_inspector::StringBuffer> message) override {
    sendMessageToFrontend(message->string());
  }


#if 0
  void sendProtocolResponse(int callId, const StringView& message) override {
    sendMessageToFrontend(message);
  }

  void sendProtocolNotification(const StringView& message) override {
    sendMessageToFrontend(message);
  }
#endif

  void flushProtocolNotifications() override { }

  void sendMessageToFrontend(const StringView& message) {
    agent_->Write(TransportAction::kSendMessage, agent_->session_id_, message);
    //agent_->server_->Send(agent_->session_id_, StringViewToUtf8(message));
  }

  AgentImpl* const agent_;
};

// Used in V8NodeInspector::currentTimeMS() below.
#define NANOS_PER_MSEC 1000000

using V8Inspector = v8_inspector::V8Inspector;

class V8NodeInspector : public v8_inspector::V8InspectorClient {
 public:
  V8NodeInspector(AgentImpl* agent,
                  v8::Platform* platform)
                  : agent_(agent),
                    platform_(platform),
                    terminated_(false),
                    running_nested_loop_(false),
                    inspector_(V8Inspector::create(agent_->isolate_, this)) {
    const uint8_t CONTEXT_NAME[] = "Node.js Main Context";
    StringView context_name(CONTEXT_NAME, sizeof(CONTEXT_NAME) - 1);
    v8_inspector::V8ContextInfo info(agent_->isolate_->GetCurrentContext(), 1, context_name);
    inspector_->contextCreated(info);
  }

  void runMessageLoopOnPause(int context_group_id) override {
    if (running_nested_loop_)
      return;
    terminated_ = false;
    running_nested_loop_ = true;
    while (!terminated_) {
      agent_->WaitForFrontendMessage();
      while (v8::platform::PumpMessageLoop(platform_, agent_->isolate_))
        {}
    }
    terminated_ = false;
    running_nested_loop_ = false;
  }

  double currentTimeMS() override {
    return uv_hrtime() * 1.0 / NANOS_PER_MSEC;
  }

  void quitMessageLoopOnPause() override {
    terminated_ = true;
  }

  void connectFrontend() {
    session_ = inspector_->connect(1, new ChannelImpl(agent_), StringView());
  }

  void disconnectFrontend() {
    session_.reset();
  }

  void dispatchMessageFromFrontend(const StringView& message) {
    assert(session_);
    session_->dispatchProtocolMessage(message);
  }

  v8::Local<v8::Context> ensureDefaultContextInGroup(int contextGroupId)
      override {
    return agent_->isolate_->GetCurrentContext();
  }

  V8Inspector* inspector() {
    return inspector_.get();
  }

 private:
  AgentImpl* agent_;
  v8::Platform* platform_;
  bool terminated_;
  bool running_nested_loop_;
  std::unique_ptr<V8Inspector> inspector_;
  std::unique_ptr<v8_inspector::V8InspectorSession> session_;
};

AgentImpl::AgentImpl(v8::Isolate* isolate) : delegate_(nullptr),
                                         wait_(true),
                                         isolate_(isolate),
                                         shutting_down_(false),
                                         state_(State::kNew),
                                         inspector_(nullptr),
                                         platform_(nullptr),
                                         dispatching_messages_(false),
                                         session_id_(0),
                                         server_(nullptr) {
  inspector_event_loop = uv_default_loop();
  assert(0 == uv_async_init(inspector_event_loop, &main_thread_req_,
                            AgentImpl::MainThreadAsyncCb));
  uv_unref(reinterpret_cast<uv_handle_t*>(&main_thread_req_));
  assert(0 == uv_sem_init(&start_sem_, 0));
  memset(&io_thread_req_, 0, sizeof(io_thread_req_));
}

void InspectorConsoleCall(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  assert(info.Data()->IsArray());
  v8::Local<v8::Array> args = info.Data().As<v8::Array>();
  assert(args->Length() == 3);

  v8::Local<v8::Value> inspector_method =
      args->Get(context, 0).ToLocalChecked();
  assert(inspector_method->IsFunction());
  v8::Local<v8::Value> node_method =
      args->Get(context, 1).ToLocalChecked();
  assert(node_method->IsFunction());
  v8::Local<v8::Value> config_value =
      args->Get(context, 2).ToLocalChecked();
  assert(config_value->IsObject());
  v8::Local<v8::Object> config_object = config_value.As<v8::Object>();

  std::vector<v8::Local<v8::Value>> call_args(info.Length());
  for (int i = 0; i < info.Length(); ++i) {
    call_args[i] = info[i];
  }

  v8::Local<v8::String> in_call_key = OneByteString(isolate, "in_call");
  bool in_call = config_object->Has(context, in_call_key).FromMaybe(false);
  if (!in_call) {
    assert(config_object->Set(context,
                             in_call_key,
                             v8::True(isolate)).FromJust());
    assert(!inspector_method.As<v8::Function>()->Call(
        context,
        info.Holder(),
        call_args.size(),
        call_args.data()).IsEmpty());
  }

  v8::TryCatch try_catch(info.GetIsolate());
  static_cast<void>(node_method.As<v8::Function>()->Call(context,
                                                         info.Holder(),
                                                         call_args.size(),
                                                         call_args.data()));
  assert(config_object->Delete(context, in_call_key).FromJust());
  if (try_catch.HasCaught())
    try_catch.ReThrow();
}

void InspectorWrapConsoleCall(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate *isolate = args.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (args.Length() != 3 || !args[0]->IsFunction() ||
      !args[1]->IsFunction() || !args[2]->IsObject()) {
    printf("inspector.wrapConsoleCall takes exactly 3 arguments: two functions and an object.\n");
    exit(0);
  }

  v8::Local<v8::Array> array = v8::Array::New(isolate, args.Length());
  assert(array->Set(context, 0, args[0]).FromJust());
  assert(array->Set(context, 1, args[1]).FromJust());
  assert(array->Set(context, 2, args[2]).FromJust());
  args.GetReturnValue().Set(v8::Function::New(context,
                                              InspectorConsoleCall,
                                              array).ToLocalChecked());
}
bool AgentImpl::Start(v8::Platform* platform, const char* path) {
  inspector_ = new V8NodeInspector(this, platform);
  platform_ = platform;
  if (path != nullptr)
    script_name_ = path;

  InstallInspectorOnProcess();

  int err = uv_thread_create(&thread_, AgentImpl::ThreadCbIO, this);
  assert(err == 0);
  uv_sem_wait(&start_sem_);

  if (state_ == State::kError) {
    Stop();
    return false;
  }
  state_ = State::kAccepting;
  if (wait_) {
    DispatchMessages();
  }
  return true;
}

void AgentImpl::Stop() {
  int err = uv_thread_join(&thread_);
  assert(err == 0);
  delete inspector_;
}

bool AgentImpl::IsConnected() {
  return delegate_ != nullptr && delegate_->IsConnected();
}

bool AgentImpl::IsStarted() {
  return !!platform_;
}

void AgentImpl::WaitForDisconnect() {
  if (state_ == State::kConnected) {
    shutting_down_ = true;
    Write(TransportAction::kStop, 0, StringView());
    fprintf(stderr, "Waiting for the debugger to disconnect...\n");
    fflush(stderr);
    inspector_->runMessageLoopOnPause(0);
  }
}

void AgentImpl::InstallInspectorOnProcess() {
  v8::Local<v8::ObjectTemplate> inspector = v8::ObjectTemplate::New(isolate_);

    inspector->Set(v8::String::NewFromUtf8(isolate_, "wrapConsoleCall"),
                          v8::FunctionTemplate::New(isolate_, InspectorWrapConsoleCall));
}

std::unique_ptr<StringBuffer> ToProtocolString(v8::Local<v8::Value> value) {
  if (value.IsEmpty() || value->IsNull() || value->IsUndefined() ||
      !value->IsString()) {
    return StringBuffer::create(StringView());
  }
  v8::Local<v8::String> string_value = v8::Local<v8::String>::Cast(value);
  size_t len = string_value->Length();
  std::basic_string<uint16_t> buffer(len, '\0');
  string_value->Write(&buffer[0], 0, len);
  return StringBuffer::create(StringView(buffer.data(), len));
}

void AgentImpl::FatalException(v8::Local<v8::Value> error,
                               v8::Local<v8::Message> message) {
  if (!IsStarted())
    return;
  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  int script_id = message->GetScriptOrigin().ScriptID()->Value();

  v8::Local<v8::StackTrace> stack_trace = message->GetStackTrace();

  if (!stack_trace.IsEmpty() &&
      stack_trace->GetFrameCount() > 0 &&
      script_id == stack_trace->GetFrame(0)->GetScriptId()) {
    script_id = 0;
  }

  const uint8_t DETAILS[] = "Uncaught";

  inspector_->inspector()->exceptionThrown(
      context,
      StringView(DETAILS, sizeof(DETAILS) - 1),
      error,
      ToProtocolString(message->Get())->string(),
      ToProtocolString(message->GetScriptResourceName())->string(),
      message->GetLineNumber(context).FromMaybe(0),
      message->GetStartColumn(context).FromMaybe(0),
      inspector_->inspector()->createStackTrace(stack_trace),
      script_id);
  WaitForDisconnect();
}

// static
void AgentImpl::ThreadCbIO(void* agent) {
  static_cast<AgentImpl*>(agent)->WorkerRunIO();
}

// static
void AgentImpl::WriteCbIO(uv_async_t* async) {
  AgentImpl* agent = static_cast<AgentImpl*>(async->data);
  MessageQueue<TransportAction> outgoing_messages;
  agent->SwapBehindLock(&agent->outgoing_message_queue_, &outgoing_messages);
  for (const auto& outgoing : outgoing_messages) {
    switch (std::get<0>(outgoing)) {
    case TransportAction::kStop:
      agent->server_->Stop(nullptr);
      break;
    case TransportAction::kSendMessage:
      std::string message = StringViewToUtf8(std::get<2>(outgoing)->string());
      printf("%d %s sending message %s \n", __LINE__, __FILE__, message.c_str());
      agent->server_->Send(std::get<1>(outgoing), message);
      break;
    }
  }
}

void AgentImpl::WorkerRunIO() {
  int err = uv_loop_init(&child_loop_);
  assert(err == 0);
  err = uv_async_init(&child_loop_, &io_thread_req_, AgentImpl::WriteCbIO);
  assert(err == 0);
  io_thread_req_.data = this;
  std::string script_path;
  if (!script_name_.empty()) {
    uv_fs_t req;
    if (0 == uv_fs_realpath(&child_loop_, &req, script_name_.c_str(), nullptr))
      script_path = std::string(reinterpret_cast<char*>(req.ptr));
    uv_fs_req_cleanup(&req);
  }
  InspectorAgentDelegate delegate(this, script_path, script_name_, wait_);
  delegate_ = &delegate;
  InspectorSocketServer server(&delegate,
                               "172.31.56.175",
                               9900);
  if (!server.Start(&child_loop_)) {
    state_ = State::kError;  // Safe, main thread is waiting on semaphore
    uv_close(reinterpret_cast<uv_handle_t*>(&io_thread_req_), nullptr);
    uv_loop_close(&child_loop_);
    uv_sem_post(&start_sem_);
    return;
  }
  server_ = &server;
  if (!wait_) {
    uv_sem_post(&start_sem_);
  }
  uv_run(&child_loop_, UV_RUN_DEFAULT);
  uv_close(reinterpret_cast<uv_handle_t*>(&io_thread_req_), nullptr);
  server.Stop(nullptr);
  server.TerminateConnections(nullptr);
  uv_run(&child_loop_, UV_RUN_NOWAIT);
  err = uv_loop_close(&child_loop_);
  assert(err == 0);
  delegate_ = nullptr;
  server_ = nullptr;
}

template <typename ActionType>
bool AgentImpl::AppendMessage(MessageQueue<ActionType>* queue,
                              ActionType action, int session_id,
                              std::unique_ptr<StringBuffer> buffer) {
  state_lock_.lock();
  bool trigger_pumping = queue->empty();
  queue->push_back(std::make_tuple(action, session_id, std::move(buffer)));
  state_lock_.unlock();
  return trigger_pumping;
}

template <typename ActionType>
void AgentImpl::SwapBehindLock(MessageQueue<ActionType>* vector1,
                               MessageQueue<ActionType>* vector2) {
  state_lock_.lock();
  vector1->swap(*vector2);
  state_lock_.unlock();
}

void AgentImpl::PostIncomingMessage(InspectorAction action, int session_id,
                                    const std::string& message) {
    printf("%s %d appending action %d session %d and  message %s\n", __FILE__, __LINE__, action, session_id, message.c_str());
  if (AppendMessage(&incoming_message_queue_, action, session_id,
                    Utf8ToStringView(message))) {
    platform_->CallOnForegroundThread(isolate_,
                                      new DispatchOnInspectorBackendTask(this));
    isolate_->RequestInterrupt(InterruptCallback, this);
    assert(0 == uv_async_send(&main_thread_req_));
  }
  NotifyMessageReceived();
}

void AgentImpl::WaitForFrontendMessage() {
  std::unique_lock<std::mutex> lck(state_lock_);
  if (incoming_message_queue_.empty())
    incoming_message_cond_.wait(lck);
}

void AgentImpl::NotifyMessageReceived() {
  std::unique_lock<std::mutex> lck(state_lock_);
  incoming_message_cond_.notify_all();
}

void AgentImpl::DispatchMessages() {
  // This function can be reentered if there was an incoming message while
  // V8 was processing another inspector request (e.g. if the user is
  // evaluating a long-running JS code snippet). This can happen only at
  // specific points (e.g. the lines that call inspector_ methods)
  if (dispatching_messages_)
    return;
  dispatching_messages_ = true;
  MessageQueue<InspectorAction> tasks;
  do {
    tasks.clear();
    SwapBehindLock(&incoming_message_queue_, &tasks);
    for (const auto& task : tasks) {
      StringView message = std::get<2>(task)->string();
      switch (std::get<0>(task)) {
      case InspectorAction::kStartSession:
        assert(State::kAccepting == state_);
        session_id_ = std::get<1>(task);
        state_ = State::kConnected;
        fprintf(stderr, "Debugger attached.\n");
        inspector_->connectFrontend();
        break;
      case InspectorAction::kEndSession:
        assert(State::kConnected == state_);
        if (shutting_down_) {
          state_ = State::kDone;
        } else {
          state_ = State::kAccepting;
        }
        inspector_->quitMessageLoopOnPause();
        inspector_->disconnectFrontend();
        break;
      case InspectorAction::kSendMessage:
        inspector_->dispatchMessageFromFrontend(message);
        break;
      }
    }
  } while (!tasks.empty());
  dispatching_messages_ = false;
}

// static
void AgentImpl::MainThreadAsyncCb(uv_async_t* req) {
  AgentImpl* agent = ContainerOf(&AgentImpl::main_thread_req_, req);
  agent->DispatchMessages();
}

#if 1
void AgentImpl::Write(TransportAction action, int session_id,
                      const StringView& inspector_message) {
  AppendMessage(&outgoing_message_queue_, action, session_id,
                StringBuffer::create(inspector_message));
  int err = uv_async_send(&io_thread_req_);
  assert(0 == err);
}
#endif

// Exported class Agent
InspectorAgentDelegate::InspectorAgentDelegate(AgentImpl* agent,
                                               const std::string& script_path,
                                               const std::string& script_name,
                                               bool wait)
                                               : agent_(agent),
                                                 connected_(false),
                                                 session_id_(0),
                                                 script_name_(script_name),
                                                 script_path_(script_path),
                                                 target_id_(GenerateID()),
                                                 waiting_(wait) { }


bool InspectorAgentDelegate::StartSession(int session_id,
                                          const std::string& target_id) {
  if (connected_)
    return false;
  connected_ = true;
  session_id_++;
  agent_->PostIncomingMessage(InspectorAction::kStartSession, session_id, "");
  return true;
}

void InspectorAgentDelegate::MessageReceived(int session_id,
                                             const std::string& message) {
  // TODO(pfeldman): Instead of blocking execution while debugger
  // engages, node should wait for the run callback from the remote client
  // and initiate its startup. This is a change to node.cc that should be
  // upstreamed separately.
  if (waiting_) {
    if (message.find("\"Runtime.runIfWaitingForDebugger\"") !=
        std::string::npos) {
      waiting_ = false;
      agent_->ResumeStartup();
    }
  }
  agent_->PostIncomingMessage(InspectorAction::kSendMessage, session_id,
                              message);
}

void InspectorAgentDelegate::EndSession(int session_id) {
  connected_ = false;
  agent_->PostIncomingMessage(InspectorAction::kEndSession, session_id, "");
}

std::vector<std::string> InspectorAgentDelegate::GetTargetIds() {
  return { target_id_ };
}

std::string InspectorAgentDelegate::GetTargetTitle(const std::string& id) {
  return script_name_.empty() ? GetProcessTitle() : script_name_;
}

std::string InspectorAgentDelegate::GetTargetUrl(const std::string& id) {
  return "file://" + script_path_;
}

void start_debug_agent(v8::Isolate *isolate, v8::Platform* platform, const char *path) {
    AgentImpl agent(isolate);
    agent.Start(platform, path);
}
// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <include/v8.h>

#include <include/libplatform/libplatform.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <inspector_agent.h>

/**
 * This sample program shows how to implement a simple javascript shell
 * based on V8.  This includes initializing V8 with command line options,
 * creating global functions, compiling and executing strings.
 *
 * For a more sophisticated shell, consider using the debug shell D8.
 */


v8::Local<v8::Context> CreateShellContext(v8::Isolate* isolate);
void RunShell(v8::Local<v8::Context> context, v8::Platform* platform);
int RunMain(v8::Isolate* isolate, v8::Platform* platform, int argc,
            char* argv[]);
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions);
void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
void Version(const v8::FunctionCallbackInfo<v8::Value>& args);
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name);
void ReportException(v8::Isolate* isolate, v8::TryCatch* handler);


static bool run_shell;

int main(int argc, char* argv[]) {
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  v8::Platform* platform = v8::platform::CreateDefaultPlatform();
  v8::V8::InitializePlatform(platform);
  v8::V8::Initialize();
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);
  run_shell = (argc == 1);
  int result;
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = CreateShellContext(isolate);
    if (context.IsEmpty()) {
      fprintf(stderr, "Error creating context\n");
      return 1;
    }
    v8::Context::Scope context_scope(context);
    result = RunMain(isolate, platform, argc, argv);
    //if (run_shell) RunShell(context, platform);
    //InspectorSocketServer debug_server;
    //debug_server.Start();
  }
  isolate->Dispose();
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  delete platform;
  delete create_params.array_buffer_allocator;
  return result;
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}


// Creates a new execution environment containing the built-in
// functions.
v8::Local<v8::Context> CreateShellContext(v8::Isolate* isolate) {
  // Create a template for the global object.
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
  // Bind the global 'print' function to the C++ Print callback.
  global->Set(
      v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, Print));
  // Bind the global 'read' function to the C++ Read callback.
  global->Set(v8::String::NewFromUtf8(
                  isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Read));
  // Bind the global 'load' function to the C++ Load callback.
  global->Set(v8::String::NewFromUtf8(
                  isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Load));
  // Bind the 'quit' function
  global->Set(v8::String::NewFromUtf8(
                  isolate, "quit", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Quit));
  // Bind the 'version' function
  global->Set(
      v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, Version));

  return v8::Context::New(isolate, NULL, global);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  v8::String::Utf8Value file(args[0]);
  if (*file == NULL) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  v8::Local<v8::String> source;
  if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  args.GetReturnValue().Set(source);
}


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value file(args[i]);
    if (*file == NULL) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    v8::Local<v8::String> source;
    if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
  }
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
  // If not arguments are given args[0] will yield undefined which
  // converts to the integer value 0.
  int exit_code =
      args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
  fflush(stdout);
  fflush(stderr);
  exit(exit_code);
}


void Version(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(
      v8::String::NewFromUtf8(args.GetIsolate(), v8::V8::GetVersion(),
                              v8::NewStringType::kNormal).ToLocalChecked());
}


// Reads a file into a v8 string.
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::MaybeLocal<v8::String>();

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (size_t i = 0; i < size;) {
    i += fread(&chars[i], 1, size - i, file);
    if (ferror(file)) {
      fclose(file);
      return v8::MaybeLocal<v8::String>();
    }
  }
  fclose(file);
  v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
      isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
  delete[] chars;
  return result;
}


// Process remaining command line arguments and execute files
int RunMain(v8::Isolate* isolate, v8::Platform* platform, int argc,
            char* argv[]) {
  for (int i = 1; i < argc; i++) {
    const char* str = argv[i];
    if (strcmp(str, "--shell") == 0) {
      run_shell = true;
    } else if (strcmp(str, "-f") == 0) {
      // Ignore any -f flags for compatibility with the other stand-
      // alone JavaScript engines.
      continue;
    } else if (strncmp(str, "--", 2) == 0) {
      fprintf(stderr,
              "Warning: unknown flag %s.\nTry --help for options\n", str);
    } else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
      // Execute argument given to -e option directly.
      v8::Local<v8::String> file_name =
          v8::String::NewFromUtf8(isolate, "unnamed",
                                  v8::NewStringType::kNormal).ToLocalChecked();
      v8::Local<v8::String> source;
      if (!v8::String::NewFromUtf8(isolate, argv[++i],
                                   v8::NewStringType::kNormal)
               .ToLocal(&source)) {
        return 1;
      }
      bool success = ExecuteString(isolate, source, file_name, false, true);
      while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
      if (!success) return 1;
    } else {
      // Use all other arguments as names of files to load and run.
      v8::Local<v8::String> file_name =
          v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal)
              .ToLocalChecked();
      v8::Local<v8::String> source;
      if (!ReadFile(isolate, str).ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", str);
        continue;
      }
      AgentImpl agent(isolate);
      agent.Start(platform, str);
      //while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
      bool success = ExecuteString(isolate, source, file_name, false, true);
      while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
      if (!success) return 1;
    }
  }
  return 0;
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Local<v8::Context> context, v8::Platform* platform) {
  fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());
  static const int kBufferSize = 256;
  // Enter the execution environment before evaluating any code.
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> name(
      v8::String::NewFromUtf8(context->GetIsolate(), "(shell)",
                              v8::NewStringType::kNormal).ToLocalChecked());
  while (true) {
    char buffer[kBufferSize];
    fprintf(stderr, "> ");
    char* str = fgets(buffer, kBufferSize, stdin);
    if (str == NULL) break;
    v8::HandleScope handle_scope(context->GetIsolate());
    ExecuteString(
        context->GetIsolate(),
        v8::String::NewFromUtf8(context->GetIsolate(), str,
                                v8::NewStringType::kNormal).ToLocalChecked(),
        name, true, true);
    while (v8::platform::PumpMessageLoop(platform, context->GetIsolate()))
      continue;
  }
  fprintf(stderr, "\n");
}


// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  v8::ScriptOrigin origin(name);
  v8::Local<v8::Context> context(isolate->GetCurrentContext());
  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
    // Print errors that happened during compilation.
    if (report_exceptions)
      ReportException(isolate, &try_catch);
    return false;
  } else {
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
      assert(try_catch.HasCaught());
      // Print errors that happened during execution.
      if (report_exceptions)
        ReportException(isolate, &try_catch);
      return false;
    } else {
      assert(!try_catch.HasCaught());
      if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        const char* cstr = ToCString(str);
        printf("%s\n", cstr);
      }
      return true;
    }
  }
}


void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope(isolate);
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Local<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    fprintf(stderr, "%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
    v8::Local<v8::Context> context(isolate->GetCurrentContext());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber(context).FromJust();
    fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(
        message->GetSourceLine(context).ToLocalChecked());
    const char* sourceline_string = ToCString(sourceline);
    fprintf(stderr, "%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn(context).FromJust();
    for (int i = 0; i < start; i++) {
      fprintf(stderr, " ");
    }
    int end = message->GetEndColumn(context).FromJust();
    for (int i = start; i < end; i++) {
      fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
    v8::Local<v8::Value> stack_trace_string;
    if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
        stack_trace_string->IsString() &&
        v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
      v8::String::Utf8Value stack_trace(stack_trace_string);
      const char* stack_trace_string = ToCString(stack_trace);
      fprintf(stderr, "%s\n", stack_trace_string);
    }
  }
}
