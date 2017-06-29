#ifndef SRC_ENV_H_
#define SRC_ENV_H_

#include "v8.h"
#include "inspector_agent.h"
#include <cassert>

namespace inspector {
    class Agent;

    class Environment {
        public:
            Environment(v8::Isolate* isolate, Agent* agent)
                :isolate_(isolate), agent_(agent)
            {}
            v8::Isolate* isolate() {return isolate_;}
            Agent* inspector_agent() {return agent_;}
            v8::Local<v8::Context> context() { return isolate_->GetCurrentContext(); }
            void SetMethod(v8::Local<v8::Object> that, const char* name,
                    v8::FunctionCallback callback) {
                v8::Local<v8::Function> function =
                    v8::FunctionTemplate::New(isolate_, callback)->GetFunction();
                // kInternalized strings are created in the old space.
                const v8::NewStringType type = v8::NewStringType::kInternalized;
                v8::Local<v8::String> name_string =
                    v8::String::NewFromUtf8(isolate_, name, type).ToLocalChecked();
                that->Set(name_string, function);
                function->SetName(name_string);  // NODE_SET_METHOD() compatibility.
            }
 static inline Environment* GetCurrent(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  assert(info.Data()->IsExternal());
  return static_cast<Environment*>(info.Data().As<v8::External>()->Value());
}



        private:
            v8::Isolate *isolate_;
            Agent* agent_;
    };
}
#endif //SRC_ENV_H_
