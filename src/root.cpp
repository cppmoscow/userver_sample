#include "root.hpp"

#include <fmt/format.h>

#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace userver_sample {

namespace {

class Root final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-root";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    userver::formats::json::ValueBuilder builder;
    builder["message"] = "Hello, World!";
    auto response = builder.ExtractValue();

    request.GetHttpResponse().SetContentType("application/json");
    return ToString(response);
  }
};

}  // namespace

void AppendRoot(userver::components::ComponentList& component_list) {
  component_list.Append<Root>();
}

}  // namespace userver_sample
