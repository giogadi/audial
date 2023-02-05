#include "string_ci.h"

#include <algorithm>

namespace string_ci {

bool LessStr::operator()(std::string const& lhs, std::string const& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), LessChar());
}

}
