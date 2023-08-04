#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

namespace userver_sample {

std::string SayHelloTo(std::string_view name);

void AppendHello(userver::components::ComponentList &component_list);

} // namespace userver_sample
