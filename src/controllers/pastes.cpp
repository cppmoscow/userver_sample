#include "pastes.hpp"

#include "models/paste.hpp"

using namespace userver;

namespace {

const storages::postgres::Query kSelectPasteContent{
    "SELECT content FROM pastes WHERE code=$1",
    storages::postgres::Query::Name{"select_paste_content"}};

const storages::postgres::Query kInsertPaste{
    "INSERT INTO pastes (content) VALUES($1) RETURNING code, token::text "
    "AS token",
    storages::postgres::Query::Name{"insert_paste"}};

const storages::postgres::Query kUpdatePaste{
    "UPDATE pastes SET content=$1 WHERE token::text=$2",
    storages::postgres::Query::Name{"update_paste"}};

const storages::postgres::Query kDeletePaste{
    "DELETE FROM pastes WHERE token::text=$1",
    storages::postgres::Query::Name{"delete_paste"}};

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

namespace pastebin {

PastesController::PastesController(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("pastes-database")
                      .GetCluster()) {}

std::string PastesController::HandleRequestThrow(
    const server::http::HttpRequest& request,
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

std::string PastesController::CreatePaste(
    const server::http::HttpRequest& request) const {
  const auto& content = request.RequestBody();
  auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                                  kInsertPaste, content);
  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  auto paste = res.AsSingleRow<Paste>(storages::postgres::kRowTag);

  userver::formats::json::ValueBuilder builder(paste);
  auto response = builder.ExtractValue();

  request.GetHttpResponse().SetContentType("application/json");
  return ToString(response);
}

std::string PastesController::GetPaste(std::string_view code,
                                       const server::http::HttpRequest&) const {
  storages::postgres::ResultSet res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kSlave, kSelectPasteContent, code);
  if (res.IsEmpty()) {
    throw not_found_error;
  }
  return res.AsSingleRow<std::string>();
}

std::string PastesController::UpdatePaste(
    std::string_view token, const server::http::HttpRequest& request) const {
  const auto& content = request.RequestBody();
  auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                                  kUpdatePaste, content, token);
  if (res.RowsAffected()) {
    return {};
  }

  throw authentication_error;
}

std::string PastesController::DeletePaste(
    std::string_view token, const server::http::HttpRequest&) const {
  auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                                  kDeletePaste, token);
  if (res.RowsAffected()) {
    return {};
  }

  throw authentication_error;
}

}  // namespace pastebin