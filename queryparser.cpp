#include "sync.hpp"
#include "async.hpp"

using v8::FunctionTemplate;

NAN_MODULE_INIT(PgQuery) {
  Nan::Set(target, Nan::New<v8::String>("parseQuery").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(Parse)).ToLocalChecked());

  Nan::Set(target, Nan::New<v8::String>("parseQueryAsync").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(ParseAsync)).ToLocalChecked());
}

NODE_MODULE(queryparser, PgQuery)