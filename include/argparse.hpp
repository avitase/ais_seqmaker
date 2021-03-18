#pragma once

#include "utility.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace argparse {
class Argparse {
  private:
    std::unordered_map<std::string, std::string> args_;

  public:
    Argparse(int argc, const char** argv) noexcept {
        std::string last_arg;
        for (auto i = 1; i < argc; i++) {
            std::string arg = argv[i];   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (not arg.empty()) {
                if (arg.starts_with('-')) {
                    args_.emplace(arg, "");
                    last_arg = arg;
                } else if (not last_arg.empty()) {
                    args_.insert_or_assign(last_arg, arg);
                    last_arg.clear();
                }
            }
        }
    }

    [[nodiscard]] auto n_args() const noexcept {
        return args_.size();
    }

    [[nodiscard]] auto is_set(std::string_view arg) const noexcept {
        return args_.contains(std::string{arg});
    }

    [[nodiscard]] std::optional<std::string> get(std::string_view arg) const noexcept {
        if (auto it = args_.find(std::string{arg}); it != args_.end() and not it->second.empty()) {
            return it->second;
        }
        return std::nullopt;
    }

    template <typename T>
    [[nodiscard]] std::optional<std::string> check_args(const T& valid_args) const noexcept {
        for (auto [k, v] : args_) {
            if (not valid_args.contains(k)) {
                return k;
            }
        }
        return std::nullopt;
    }
};
}   // namespace argparse