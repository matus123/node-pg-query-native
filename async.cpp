/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2016 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>
#include <memory> 
#include "async.hpp"  // NOLINT(build/include)

using v8::Function;
using v8::Local;
using v8::Number;
using v8::Value;
using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::HandleScope;
using Nan::New;
using Nan::Null;
using Nan::To;

class ParserWorker : public AsyncWorker {
 public:
  ParserWorker(Callback *callback, std::shared_ptr<Nan::Utf8String> query)
    : AsyncWorker(callback), query_(query) {}
  ~ParserWorker() {
    pg_query_free_parse_result(result_);
  }

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    result_ = pg_query_parse(*(*query_));
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    HandleScope scope;

    Local<Value> argv[2];

    if(result_.error) {
      v8::Local<v8::Object> error = Nan::New<v8::Object>();

      v8::Local<v8::String> message = Nan::New(result_.error->message).ToLocalChecked();
      v8::Local<v8::String> fileName = Nan::New(result_.error->filename).ToLocalChecked();
      v8::Local<v8::String> functionName = Nan::New(result_.error->funcname).ToLocalChecked();
      v8::Local<v8::Number> lineNumber = Nan::New(result_.error->lineno);
      v8::Local<v8::Number> cursorPosition = Nan::New(result_.error->cursorpos);

      Nan::Set(error, Nan::New("message").ToLocalChecked(), message);
      Nan::Set(error, Nan::New("fileName").ToLocalChecked(), fileName);
      Nan::Set(error, Nan::New("functionName").ToLocalChecked(), functionName);
      Nan::Set(error, Nan::New("lineNumber").ToLocalChecked(), lineNumber);
      Nan::Set(error, Nan::New("cursorPosition").ToLocalChecked(), cursorPosition);

      if (result_.error->context) {
        Nan::Set(error, Nan::New("context").ToLocalChecked(), Nan::New(result_.error->context).ToLocalChecked());
      }
      else {
        Nan::Set(error, Nan::New("context").ToLocalChecked(), Nan::Null());
      }

      argv[0] = error;
      argv[1] = Null();
    }

    if(result_.parse_tree) {
      v8::Local<v8::Object> hash = Nan::New<v8::Object>();

      Nan::Set(hash, Nan::New("query").ToLocalChecked(),
                       Nan::New(result_.parse_tree).ToLocalChecked());

      argv[0] = Null();
      argv[1] = hash;
    }

    callback->Call(2, argv);
    
  }

 private:
  PgQueryParseResult result_;
  std::shared_ptr<Nan::Utf8String> query_;
};

NAN_METHOD(ParseAsync) {
  std::shared_ptr<Nan::Utf8String> query = std::make_shared<Nan::Utf8String>(info[0]);
  // int points = To<int>(info[0]).FromJust();
  Callback *callback = new Callback(info[1].As<Function>());

  AsyncQueueWorker(new ParserWorker(callback, query));
}