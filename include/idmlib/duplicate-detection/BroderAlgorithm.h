/**
 @file BroderAlgorithm.h
 @author Jinglei
 @date   2009.11.24
 */
#ifndef BroderAlgorithm_h
#define BroderAlgorithm_h 1


#include "FingerPrinter.h"
#include "NearDuplicateAlgorithm.h"

//#include <wiselib/basics.h>
//#include <wiselib/IntTypes.h>
#include <wiselib/CBitArray.h>
#include <wiselib/Array.h>
#include <wiselib/DArray.h>

namespace sf1v5{
/**
 * @class BroderAlgorithm
 * @brief generate a vector of n number of supershingles from a document.
 */
 class BroderAlgorithm
 : public NearDuplicateAlgorithm
  {
    

 private:
 /**
  * number of different rabin fingerprint functions
  */
  const int m;
  /**
   * supershingle factor
   */
  const int l;
  /**
   * m/l = mPrime, m%l = 0
   */
  const int mPrime;
  /**
   * min number of num super shingles, if score >= this value,
   * are duplicate or near duplicate documents
   */
  const int minNumSuperShingles;

  /**
   * the size of sequence of tokens for shingle, fixed for 8
   */
  static const int k = 8;

  /**
   * array of m number of different figerprint functions
   */
  wiselib::Array<FingerPrinter> fpFuncs;

  /**
   * fingerprint function to generate a super shingle from a vector of min shingles.
   */
  FingerPrinter fpSuperShingle;

  /**
   * max value of l
   */
  static const int MAX_L_VALUE = 20;

  /**
   * temp place holders when hold
   */
  wiselib::ub8 ktokens[k];

  /**
   * temp place when do super shingle
   */
  wiselib::ub8 ltokens[MAX_L_VALUE];
private:
  /**
   * @brief set fingerprinters' seeds, instead of random generated seed
   */
  void set_fingerprinters();
public:
  /**
   * default m value
   */
  static const int DEFAULT_M_VALUE = 84;
  /**
   * default l value
   */
  static const int DEFAULT_L_VALUE = 14;
  /**
   * default minNumSuperShingles value
   */
  static const int DEFAULT_MIN_NUM_SUPERSHINGLES = 2;
public:
  /**
   * @brief constructor
   *
   * @param am value to init m
   * @param al value to init l
   * @param mscore value to init minNumSuperShingles
   */
  inline BroderAlgorithm(int am=DEFAULT_M_VALUE, int al=DEFAULT_L_VALUE, int mscore=DEFAULT_MIN_NUM_SUPERSHINGLES)
    : m(am), l(al), mPrime(m/l), minNumSuperShingles(mscore), fpFuncs(m) {
    ASSERT(m%l == 0);
    ASSERT(l < MAX_L_VALUE);
    set_fingerprinters();
  }
  /**
   * @brief constructor
   *
   * @param threshold use this value to init minNumSuperShingles
   */
  inline BroderAlgorithm(double threshold)
    : m(DEFAULT_M_VALUE), l(DEFAULT_L_VALUE), mPrime(m/l),minNumSuperShingles((int)(m / l * threshold) ), fpFuncs(m) {
    ASSERT(m%l == 0);
    ASSERT(l < MAX_L_VALUE);
    set_fingerprinters();
  }
  /**
   * destructor
   */
  inline ~BroderAlgorithm() {}
  /**
   * get mPrime
   */
  inline int num_supershingles() const { return mPrime; }
  /**
   * get minNumSuperShingles
   */
  inline int min_num_supershingles() const { return minNumSuperShingles; }
  /**
   * get dimensions
   */
  inline int num_dimensions() { return mPrime * 64; }
  /**
   * get near duplicate score
   *
   * @param sig1 signature to compare
   * @param sig2 signature to compare
   *
   * @return similarity score
   */
  int neardup_score(NearDuplicateSignature& sig1,
		    NearDuplicateSignature& sig2);

  /**
   * get near duplicate score
   *
   * @param sig1 first hash value to compare
   * @param sig2 second hash value to compare
   *
   * @return similarity score
   */
  int neardup_score(const wiselib::CBitArray& sig1, const wiselib::CBitArray& sig2);
  /**
   * generate document's signature
   *
   * @param[in] docTokens document tokens' id array
   * @param[out] bitArray document's signature
   */
  void generate_document_signature(const wiselib::DynamicArray<wiselib::ub8>& docTokens, wiselib::CBitArray& bitArray);
  /**
   * generate document's signature
   *
   * @param[in] document document's string value
   * @param[out] bitArray document's signature
   */
  void generate_document_signature(const wiselib::YString& document, wiselib::CBitArray& bitArray);
  /**
   * generate document's signature
   *
   * @param docTokens a vector id of a document
   * @param bitArray signature of a document
   */
  void generate_document_signature(const std::vector<unsigned int>& docTokens, wiselib::CBitArray& bitArray);

  /**
   * generate document's signature
   *
   * @param docTokens a vector id of a document
   * @param bitArray signature of a document
   */
  void generate_document_signature(const std::vector<std::string>& docTokens, wiselib::CBitArray& bitArray);

  /**
   * check if 2 documents are duplicate or near duplicate
   *
   * @param neardupScore the similarity score of the 2 documents
   *
   * @return if the 2 documents are similarity, return true, else return false
   */
  inline bool is_neardup(float neardupScore) { return neardupScore >= minNumSuperShingles; }
};
}
#endif
