/**
   @file NearDuplicateAlgorithm.h
   @author Jinglei
   @date 2009.11.24
 */
#ifndef NearDuplicateAlgorithm_h
#define NearDuplicateAlgorithm_h 1

#include <vector>
#include <wiselib/YString.h>
#include <wiselib/CBitArray.h>

namespace sf1v5{
class NearDuplicateSignature;
/**
 *@brief Abstract Base Class for near duplicate algortihms.
 */
class NearDuplicateAlgorithm {
public:
    /**
     * constructor
     */
    inline NearDuplicateAlgorithm() {}
    /**
     * @brief disconstructor
     */
    inline virtual ~NearDuplicateAlgorithm() {}
public:
    /**
     * @return an integer value for nearduplicate score.
     */
    virtual int neardup_score(NearDuplicateSignature& sig1, NearDuplicateSignature& sig2) = 0;
    /**
     * @brief test duplicate score
     *
     * @param sig1 first signature
     * @param sig2 second signature
     *
     * @return score of these two documents
     */
    virtual int neardup_score(const wiselib::CBitArray& sig1, const wiselib::CBitArray& sig2)=0;
    /**
     * @return true iff if the given neardup score is enough to be a near-duplicate
     */
    virtual bool is_neardup(float neardupScore) = 0;
    /**
     * @return the dimensions of the bit vectors used in this near duplicate algorithm
     */
    virtual int num_dimensions()  = 0;
    /**
     * generate document signature given a sequent of tokens in a document
     */
    virtual void generate_document_signature(const wiselib::DynamicArray<wiselib::ub8>& docTokens, wiselib::CBitArray& bitArray) = 0;

    /**
     * @brief generate document signature from document string, not store the document tokens
     *
     * @param[in] document document content
     * @param[out] bitArray document signature
     */
    virtual void generate_document_signature(const wiselib::YString& document, wiselib::CBitArray& bitArray) = 0;

    /**
     * @brief generate document signature
     *
     * @param[in] docTokens term id sequence of a document
     * @param[out] bitArray sequent of bits as a document signature
     */
    virtual void generate_document_signature(const std::vector<unsigned int>& docTokens, wiselib::CBitArray& bitArray)=0;

     /**
     * @brief generate document signature
     *
     * @param[in] docTokens term id sequence of a document
     * @param[out] bitArray sequent of bits as a document signature
     */
    virtual void generate_document_signature(const std::vector<std::string>& docTokens, wiselib::CBitArray& bitArray)=0;
};
}
#endif
