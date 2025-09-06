#include "app/validation.hpp"
#include <regex>

namespace app {

bool IsHex(const std::string& s) {
  static const std::regex r("0x[0-9a-fA-F]*");
  return std::regex_match(s, r);
}

bool IsAddress(const std::string& s) {
  static const std::regex r("0x[0-9a-fA-F]{40}");
  return std::regex_match(s, r);
}

std::vector<std::string> Validate(const UnsignedTx& tx, bool strict_chain) {
  std::vector<std::string> errs;
  if (!IsAddress(tx.to)) errs.push_back("to");
  if (!IsHex(tx.data)) errs.push_back("data");
  auto is_num = [](const std::string& v){ return !v.empty() && std::all_of(v.begin(), v.end(), ::isdigit); };
  if (!is_num(tx.value)) errs.push_back("value");
  if (!is_num(tx.nonce)) errs.push_back("nonce");
  if (!is_num(tx.gas_limit)) errs.push_back("gasLimit");
  if (!is_num(tx.max_fee_per_gas)) errs.push_back("maxFeePerGas");
  if (!is_num(tx.max_priority_fee_per_gas)) errs.push_back("maxPriorityFeePerGas");
  if (strict_chain && tx.chain_id != 8453) errs.push_back("chainId");
  return errs;
}

} // namespace app

