// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

#include "libplatform/libplatform.h"
#include "v8.h"
#include "inspector_agent.h"
#include "env.h"

using namespace v8;
using namespace inspector;

v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name);
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions);
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
static Local<String> createUtf8String(Isolate *isolate, const char *str)
{
    return String::NewFromUtf8(isolate, str,
        NewStringType::kNormal).ToLocalChecked();
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
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
  printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
  if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
  printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
    return false;
  } else {
  printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
  printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
      assert(try_catch.HasCaught());
      return false;
    } else {
  printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
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
#if 0
            Handle<Value> val = context->Global()->Get(
                createUtf8String(isolate, "exponent"));
            Handle<Function> unusedFun = Handle<Function>::Cast(val);
            Handle<Value> arg[2];
            arg[0] = Integer::New(isolate, 10);
            arg[1] = Integer::New(isolate, 2);
            Handle<Value> js_result = unusedFun->Call(Null(isolate), 2, arg);
#endif
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
Environment *env;
void CallAndPauseOnStart(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  assert(args.Length() >= 1);
  assert(args[0]->IsFunction());
  std::vector<v8::Local<v8::Value>> call_args;
  for (int i = 2; i < args.Length(); i++) {
    call_args.push_back(args[i]);
  }

  env->inspector_agent()->PauseOnNextJavascriptStatement("Break on start");
  v8::MaybeLocal<v8::Value> retval =
      args[0].As<v8::Function>()->Call(env->context(), args[1],
                                       call_args.size(), call_args.data());
  if (!retval.IsEmpty()) {
    args.GetReturnValue().Set(retval.ToLocalChecked());
  }
}


int main(int argc, char* argv[]) {
  // Initialize V8.
  V8::InitializeICUDefaultLocation(argv[0]);
  V8::InitializeExternalStartupData(argv[0]);
  Platform* platform = platform::CreateDefaultPlatform(4);
  V8::InitializePlatform(platform);
  V8::Initialize();

  // Create a new Isolate and make it the current one.
  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(create_params);
  {
    Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);


    // Enter the context for compiling and running the hello world script.
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    global->Set(
      v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, Print));
    global->Set(
      v8::String::NewFromUtf8(isolate, "callAndPauseOnStart", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, CallAndPauseOnStart));
    Local<Context> context = Context::New(isolate, NULL, global);
    Context::Scope context_scope(context);

    Agent *agent = new Agent();
    env = new Environment(isolate, agent);
    agent->Start(env, platform, nullptr);

    v8::Local<v8::String> file_name =
          v8::String::NewFromUtf8(isolate, argv[1], v8::NewStringType::kNormal)
              .ToLocalChecked();
      v8::Local<v8::String> source;
      if (!ReadFile(isolate, argv[1]).ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", argv[1]);
        return 1;
      }
      bool success = ExecuteString(isolate, source, file_name, false, true);
      while (v8::platform::PumpMessageLoop(platform, isolate)) {};
      if (!success) return 1;
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
  delete platform;
  delete create_params.array_buffer_allocator;
  return 0;
}
