/**
   @file FingerPrinter.h
   @author Jinglei
   @date  2009.11.24
 */
#ifndef FingerPrinter_h
#define FingerPrinter_h 1
//#include <wiselib/basics.h>
#include <types.h>
#include <wiselib/MRandom.h>

namespace sf1v5{
/**
   @class FingerPrinter
 * @brief generate hash value of a string, use Finger printer
 */
class FingerPrinter {
private:
    /**
     * @brief seed when generate the hash
     */
    uint64_t seed;
  /**
   * @brief to generate a default seed
   */
  static wiselib::MRandom rand;
  enum {RAND_INIT = 47u};
public:
  /**
   * @brief default constructor
   */
  FingerPrinter();
  FingerPrinter(uint64_t s):seed(s){}
  /**
   * @brief deconstructor
   */
  inline ~FingerPrinter() {}
  /**
   * @brief set a new seed
   *
   * @param s seed to set
   */
  inline void set_seed(uint64_t s) {
      seed = s;
  }

  inline static void reset_rand(unsigned int init_seed = RAND_INIT) {
      rand.init(init_seed);
  }

  void set_seed();
  /**
   * @brief generate the string's hash value
   *
   * @param data source string
   * @param len source string length
   *
   * @return hash value
   */
  uint64_t fp(const char* data, int len) const;
};
}
#endif
