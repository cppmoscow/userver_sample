#pragma once

#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace pastebin {

using namespace userver;

class PastesController final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-pastes";

  PastesController(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string CreatePaste(const server::http::HttpRequest&) const;
  std::string GetPaste(std::string_view code,
                       const server::http::HttpRequest&) const;
  std::string UpdatePaste(std::string_view token,
                          const server::http::HttpRequest&) const;
  std::string DeletePaste(std::string_view token,
                          const server::http::HttpRequest&) const;

  storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace pastebin