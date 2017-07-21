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

using namespace v8;
using namespace inspector;

MaybeLocal<String> ReadFile(Isolate* isolate, const char* name);
bool ExecuteString(Isolate* isolate, Local<String> source,
                   Local<Value> name, bool print_result,
                   bool report_exceptions, Agent* agent);
// Reads a file into a v8 string.
MaybeLocal<String> ReadFile(Isolate* isolate, const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return MaybeLocal<String>();

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (size_t i = 0; i < size;) {
    i += fread(&chars[i], 1, size - i, file);
    if (ferror(file)) {
      fclose(file);
      return MaybeLocal<String>();
    }
  }
  fclose(file);
  MaybeLocal<String> result = String::NewFromUtf8(
      isolate, chars, NewStringType::kNormal, static_cast<int>(size));
  delete[] chars;
  return result;
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}
// Executes a string within the current v8 context.
bool ExecuteString(Isolate* isolate, Local<String> source,
                   Local<Value> name, bool print_result,
                   bool report_exceptions, Agent* agent) {
  HandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);
  ScriptOrigin origin(name);
  Local<Context> context(isolate->GetCurrentContext());
  Local<Script> script;
  if (!Script::Compile(context, source, &origin).ToLocal(&script)) {
    return false;
  } else {
    Local<Value> result;
    if (!script->Run(context).ToLocal(&result)) {
      assert(try_catch.HasCaught());
      return false;
    } else {
      assert(!try_catch.HasCaught());
      if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        String::Utf8Value str(result);
        const char* cstr = ToCString(str);
        printf("%s\n", cstr);
      }
     Handle<Value> exponent = context->Global()->Get(
        String::NewFromUtf8(isolate, "exponent", NewStringType::kNormal)
        .ToLocalChecked());
      Handle<Function> expFun = Handle<Function>::Cast(exponent);
      Handle<Value> arg[2];
      arg[0] = Integer::New(isolate, 10);
      arg[1] = Integer::New(isolate, 2);
      Handle<Value> js_result = expFun->Call(Null(isolate), 2, arg);
      if (try_catch.HasCaught()) {
        agent->FatalException(try_catch.Exception(), try_catch.Message());
      }

      return true;
    }
  }
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const FunctionCallbackInfo<Value>& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
}

int main(int argc, char* argv[]) {
  // Initialize V8.
  V8::InitializeICUDefaultLocation(argv[0]);
  V8::InitializeExternalStartupData(argv[0]);
  Platform* platform = platform::CreateDefaultPlatform();
  V8::InitializePlatform(platform);
  V8::Initialize();

  // Create a new Isolate and make it the current one.
  Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      ArrayBuffer::Allocator::NewDefaultAllocator();
  Isolate* isolate = Isolate::New(create_params);
  {
    Isolate::Scope isolate_scope(isolate);

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);


    // Enter the context for compiling and running the hello world script.
  Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
    global->Set(
      String::NewFromUtf8(isolate, "print", NewStringType::kNormal)
          .ToLocalChecked(),
      FunctionTemplate::New(isolate, Print));
    Local<Context> context = Context::New(isolate, NULL, global);
    Context::Scope context_scope(context);

    Agent *agent = new Agent("172.31.0.219", "/tmp/frontend.url");
    agent->Start(isolate, platform, argv[1]);
    agent->PauseOnNextJavascriptStatement("Break on start");

    Local<String> file_name =
          String::NewFromUtf8(isolate, argv[1], NewStringType::kNormal)
              .ToLocalChecked();
      Local<String> source;
      if (!ReadFile(isolate, argv[1]).ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", argv[1]);
        return 1;
      }
      bool success = ExecuteString(isolate, source, file_name, false, true, agent);
      while (platform::PumpMessageLoop(platform, isolate)) {};
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
