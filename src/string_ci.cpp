#include "string_ci.h"

#include <algorithm>

namespace string_ci {

bool LessStr::operator()(std::string const& lhs, std::string const& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), LessChar());
}

bool Contains(std::string_view str, std::string_view substr) {
    for (int superIx = 0; superIx < str.size(); ++superIx) {
        if (superIx + substr.size() > str.size()) {
            return false;
        }
        bool found = true;
        for (int subIx = 0; subIx < substr.size(); ++subIx) {
            if (tolower(str[superIx+subIx]) != tolower(substr[subIx])) {
                found = false;
                break;
            }
        }
        if (found) {
            return true;
        }
    }
    return false;
}

}
