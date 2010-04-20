/**
 @file GetToken.h
 @author Jinglei
 @date 2009.11.24
 @brief define a wrapper to get token from a document one by one.
 */

#ifndef __GETTOKEN_H__
#define __GETTOKEN_H__

#include <types.h>
#include <wiselib/YString.h>
//#include <wiselib/IntTypes.h>

namespace sf1v5{

/**
 * @class GetToken
 * @brief this class use to get token from document content
 */
class GetToken {
public:
    /**
     * @brief contructor
     *
     * @param document document content
     */
    GetToken(const wiselib::YString& document):
        document_(document), i(0)
        {
            delimiter_ = "\n\r\t ";
        }
private:
    /**
     * @brief document content string
     */
    const wiselib::YString& document_;
    /**
     * @brief current document position
     */
    size_t i;
    /**
     * @brief delimiter of the string to identify a token string
     */
    wiselib::YString delimiter_;
private:
    /**
     * @brief get a token string
     *
     * if can not get, return an empty string
     *
     * @return a token string
     */
    wiselib::YString get_token_string();
public:
    /**
     * @brief reset current position for a new get
     */
    void reset() {
        i = 0;
    }

    /**
     * @brief get a token string from the document and convert to a number
     *
     * @param[out] token_number target token number
     *
     * @return if success, return true, else return false
     */
    bool get_token(uint64_t& token_number);
};
}
#endif


