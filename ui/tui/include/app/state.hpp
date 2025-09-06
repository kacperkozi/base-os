#pragma once
#include <string>
#include <vector>
#include <optional>

namespace app {

struct UnsignedTx {
  std::string to;
  std::string value;
  std::string data;
  std::string nonce;
  std::string gas_limit;
  std::string max_fee_per_gas;
  std::string max_priority_fee_per_gas;
  int chain_id = 8453;
  int type = 2;
};

struct DeviceInfo {
  std::string model;
};

enum class Route {
  Home,
  CreateSimple,
  CreateAdvanced,
  Import,
  Review,
  Sign,
  Export,
  Settings,
  Help,
};

struct AppState {
  Route route = Route::Home;
  UnsignedTx unsigned_tx;
  bool has_unsigned = false;
  std::string signed_hex;
  bool has_signed = false;
  std::string status;
  std::string error;
  std::string info;
  std::string account_path = "m/44'/60'/0'/0/0";
  int selected_device = 0;
  std::vector<DeviceInfo> devices;
};

} // namespace app
