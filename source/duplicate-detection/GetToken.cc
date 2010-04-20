#include <idmlib/duplicate-detection/GetToken.h>
#include <idmlib/duplicate-detection/TokenGen.h>

#include <wiselib/YString.h>
namespace sf1v5
{
  
/**
 * @file
 * @brief define get a token function from a document
 */
wiselib::YString GetToken::get_token_string() {
    int k = 0;
    for (; i < document_.size(); ++i) {
      if (delimiter_.contains(wiselib::SM_SENSITIVE, document_[i]) || ispunct((document_)[i])) {
            if (k == 0) {
                continue;
            }
            wiselib::YString result = document_.cut(i-k, k);
            k = 0;
            return result;
        } else {
            k++;
        }
    }

    if(k != 0) {
        return document_.cut(i-k, k);
    }

    return wiselib::YString();
}

bool GetToken::get_token(uint64_t& token_number) {
    if(i >= document_.size()) {
        return false;
    }
    wiselib::YString ds = get_token_string();
    if(ds.empty()) {
        return false;
    }
    token_number = TokenGen::mfgenerate_token(ds.c_str(), ds.size());
    return true;
}
}
