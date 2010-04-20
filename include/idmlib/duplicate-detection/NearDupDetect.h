/*
 * NearDupDetect.h
 *
 *  Created on: 2008-12-19
 *      Author: jinglei
 */

#ifndef NEARDUPDETECT_H_
#define NEARDUPDETECT_H_

/**
 * @file
 * @brief clustering
 */

#include "MultiHashSet.h"
#include "NearDuplicatePair.h"
#include "TypeKey.h"
#include "Trie.h"
#include "MEfficientLHT.h"
#include "NearDuplicateAlgorithm.h"


#include <ext/hash_map>

#include <map>
#include <utility>
#include <set>

namespace sf1v5{
/**
 * @brief near dulicate clustering class
 */
class NearDupDetect{
protected:

//	static SDB sdb;

  /**
   * @brief store already compared document id pair, the pair is a ub8 number
   * high 32bits is bigger than lower 32bits;
   */
    wiselib::MEfficientLHT<wiselib::TypeKey<wiselib::ub8> > pairTable;
  /**
   * @brief near duplicate algorithm
   */
  NearDuplicateAlgorithm& ndAlgo;



  /**
   * @brief test if two documents have been already compared, if not compared,
   * store them as compared
   *
   * @param docID1 first document id
   * @param docID2 second document id
   *
   * @return true, compared; false else.
   */
  bool has_already_compared(int docID1, int docID2);

  /**
   * private static constants.
   */

public:
  inline NearDupDetect(int numEstimatedObjects,
				 NearDuplicateAlgorithm& algo)
    : ndAlgo(algo)
  {}
  inline ~NearDupDetect() {}

  /**
   * @brief add near duplicate signature by bitArray
   *
   * @param docName document name
   * @param bitArry docuemnt signature
   */
  void add_near_duplicate_signatures(const string& docName, wiselib::CBitArray& bitArry);
  // finds near-duplicate pairs in order of proximity
  void find_near_duplicate_pairs(wiselib::DynamicArray<NearDuplicatePair>& results);
  // test near duplicate algorithms
  void test_near_duplicate_algorithm(const wiselib::DynamicArray<wiselib::YString>& docNames,
				     const wiselib::DynamicArray<wiselib::YString>& documents);
};
}
#endif /* NEARDUPDETECT_H_ */
