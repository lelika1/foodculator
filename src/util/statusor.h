#ifndef __SRC_UTIL_STATUSOR_H__
#define __SRC_UTIL_STATUSOR_H__

#include <optional>
#include <string>
#include <string_view>

namespace foodculator {

enum class StatusCode { OK = 0, INVALID_ARGUMENT, NOT_FOUND, INTERNAL_ERROR };
std::string_view ToString(StatusCode code);

template <class T>
class StatusOr {
   public:
    explicit StatusOr(T t) : value_(std::move(t)), code_(StatusCode::OK){};
    StatusOr(StatusCode code, std::string error) : code_(code), error_(std::move(error)){};

    bool Ok() const { return code_ == StatusCode::OK && value_.has_value(); };
    StatusCode Code() const { return code_; };

    std::string& Error() { return error_; }
    std::string_view Error() const { return error_; }

    T& Value() { return value_.value(); }
    const T& Value() const { return value_.value(); };

   private:
    std::optional<T> value_;
    StatusCode code_;
    std::string error_;
};

}  // namespace foodculator

#endif
