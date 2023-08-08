#include "pastes.hpp"

#include "../models/paste.hpp"

#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace {

const storages::postgres::Query kSelectPasteContent{
    "SELECT content FROM pastes WHERE code=$1",
    storages::postgres::Query::Name{"select_paste_content"}};

const storages::postgres::Query kInsertPaste{
    "INSERT INTO pastes (content) VALUES($1)",
    storages::postgres::Query::Name{"insert_paste_content"}};

const std::string& GetArgOrThrow(const server::http::HttpRequest& request,
                                 const std::string& argument) {
  const auto& value = request.GetArg(argument);
  if (value.empty())
    throw server::handlers::ClientError(server::handlers::ExternalBody{
        fmt::format("No '{}' argument", argument)});
  return value;
}

const auto not_found_error = server::handlers::ResourceNotFound{};

const auto authentication_error = server::handlers::Unauthorized{};

}  // namespace

class Pastes final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-pastes";

  Pastes(const components::ComponentConfig& config,
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

Pastes::Pastes(const components::ComponentConfig& config,
               const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("pastes-database")
                      .GetCluster()) {}

std::string Pastes::HandleRequestThrow(const server::http::HttpRequest& request,
                                       server::request::RequestContext&) const {
  auto method = request.GetMethod();
  if (method == server::http::HttpMethod::kGet) {
    auto code = GetArgOrThrow(request, "code");
    return GetPaste(code, request);
  } else if (method == server::http::HttpMethod::kPut) {
    auto token = GetArgOrThrow(request, "token");
    return UpdatePaste(token, request);
  } else if (method == server::http::HttpMethod::kDelete) {
    auto token = GetArgOrThrow(request, "token");
    return DeletePaste(token, request);
  } else if (method == server::http::HttpMethod::kPost) {
    return CreatePaste(request);
  } else {
    throw server::handlers::ClientError(server::handlers::ExternalBody{
        fmt::format("Unsupported method {}", request.GetMethod())});
  }
}

std::string Pastes::CreatePaste(
    const server::http::HttpRequest& request) const {
  const auto& content = request.RequestBody();
  auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                                  kInsertPaste, content);
  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  auto [_, code, token] = res.AsSingleRow<Paste>(storages::postgres::kRowTag);

  userver::formats::json::ValueBuilder builder;
  builder["code"] = code;
  builder["token"] = token;
  auto response = builder.ExtractValue();

  request.GetHttpResponse().SetContentType("application/json");
  return ToString(response);
}

std::string Pastes::GetPaste(std::string_view code,
                             const server::http::HttpRequest&) const {
  storages::postgres::ResultSet res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kSlave, kSelectPasteContent, code);
  if (res.IsEmpty()) {
    throw not_found_error;
  }
  return res.AsSingleRow<std::string>();
}

std::string Pastes::UpdatePaste(std::string_view token,
                                const server::http::HttpRequest&) const {}

std::string Pastes::DeletePaste(std::string_view token,
                                const server::http::HttpRequest&) const {}

namespace pastebin {

void AppendPastes(userver::components::ComponentList& component_list) {
  component_list.Append<Pastes>();
}

}  // namespace pastebin
