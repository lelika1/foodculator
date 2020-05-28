#include "statusor.h"

namespace foodculator {

std::string_view ToString(StatusCode code) {
    switch (code) {
        case StatusCode::OK:
            return "OK";
        case StatusCode::INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case StatusCode::NOT_FOUND:
            return "NOT_FOUND";
        default:
            return "INTERNAL_ERROR";
    }
}

}  // namespace foodculator
