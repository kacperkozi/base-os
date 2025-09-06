#pragma once
#include <string>
#include <vector>
#include "app/state.hpp"

namespace app {

std::vector<std::string> Validate(const UnsignedTx& tx, bool strict_chain = true);
bool IsHex(const std::string& s);
bool IsAddress(const std::string& s);

} // namespace app

