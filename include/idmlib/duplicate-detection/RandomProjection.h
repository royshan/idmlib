/**
   @file RandomProjection.h
   @author Jinglei
   @date  2009.11.24
 */
#ifndef RandomProjection_h
#define RandomProjection_h 1

#include "TypeKey.h"


#include <wiselib/DArray.h>
#include <wiselib/CBitArray.h>



namespace sf1v5{
/********************************************************************************
  Description: RandomProjection is a mapping from a token to random n dimension
                 vector whose elements ranges in [-1,1].
               For more info, see RandomProjectionEngine.cc
  Comments   :
  History    : Yeogirl Yun                                        1.12.07
                 Initial Revision
********************************************************************************/

/***
 * @brief Random projection used for Charikar algorithm.
 */
class RandomProjection {
private:
    wiselib::DynamicArray<float> projection;
    /**
     * 64bit Rabin's fingerprint.
     */
    wiselib::TypeKey<wiselib::ub8> ub8key;
public:
    inline RandomProjection(int nDimensions)
        : projection(0, nDimensions, 0.),
        ub8key(0) {}
    inline RandomProjection(const wiselib::TypeKey<wiselib::ub8>& key, int nDimensions)
        : projection(0, nDimensions, 0.), ub8key(key) {}
    inline ~RandomProjection() {}
public:
    /**
     * @brief read access to projection
     *
     * @return const reference of projection
     */
    inline const wiselib::DynamicArray<float>& get_random_projection() const { return projection; }
    /**
     * @brief read/write access to projection in d
     *
     * @param d subindex
     *
     * @return reference at d in projection
     */
    inline float& operator[](int d) { return projection[d]; }
    inline const float operator[](int d) const { return projection[d]; }
    void operator+=(const RandomProjection& rp);

    /**
     * @brief return ub8key
     *
     * interface for HashTable.
     *
     * @return ub8key
     */
    const wiselib::TypeKey<wiselib::ub8>& get_key() const { return ub8key; }
    void set_key(wiselib::ub8 k) { ub8key = k; }

    void display(std::ostream& stream) const;

    /**
     * @brief get number of projection
     *
     * static members
     * @return sizeof projection
     */
    inline int num_dimensions() const  { return projection.size(); }
    /**
     * @brief Generates a CBitArray object from a random projection.
     *
     * @param projection source
     * @param bitArray target
     */
    static void generate_bitarray(const wiselib::DynamicArray<float>& projection, wiselib::CBitArray& bitArray);
};
//define ostream<<RandomProjection, reference <basics.h>
DEF_DISPLAY(RandomProjection);
}

#endif
