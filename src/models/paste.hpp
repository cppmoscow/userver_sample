#pragma once

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace pastebin {

struct Paste {
  int id;
  std::string code;
  std::string token;
};

userver::formats::json::Value Serialize(
    const Paste& paste,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["code"] = paste.code;
  builder["token"] = paste.token;

  return builder.ExtractValue();
}

}  // namespace pastebin