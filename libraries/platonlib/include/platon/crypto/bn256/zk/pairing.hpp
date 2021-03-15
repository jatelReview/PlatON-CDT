#pragma once
#include "platon/crypto/bn256/bn256.hpp"
namespace platon {
namespace crypto {
namespace bn256 {
namespace zk {
namespace pairing {

/// @return the negation of p, i.e. p.addition(p.negate()) should be zero.
G1 Negate(const G1 &p) {
  G1 p1 = p;
  return p1.Neg();
}  /// @return r the sum of two points of G1
G1 Addition(G1 p1, G1 p2) {
  G1 res = p1;
  return res.Add(p2);
}

/// @return r the sum of two points of G2
G2 Addition(const G2 &p1, const G2 &p2) {
  G2 r = p1;
  return r.Add(p2);
}

/// @return r the product of a point on G1 and a scalar, i.e.
/// p == p.scalar_mul(1) and p.addition(p) == p.scalar_mul(2) for all points p.
G1 ScalarMul(const G1 &p, const std::uint256_t &s) {
  G1 r = p;
  return r.ScalarMul(s);
}

/// Convenience method for a pairing check for two pairs.
bool PairingProd2(const G1 &a1, const G2 &a2, const G1 &b1, const G2 &b2) {
  std::array<G1, 2> g1{a1, b1};
  std::array<G2,2> g2{a2, b2};
  return bn256::pairing(g1,g2);
}
/// Convenience method for a pairing check for three pairs.
bool PairingProd3(const G1 &a1, const G2 &a2, const G1 &b1, const G2 &b2,
                  const G1 &c1, const G2 &c2) {
  std::array<G1, 3> g1 {a1, b1, c1};
  std::array<G2, 3> g2 {a2, b2, c2};
  return bn256::pairing(g1, g2);
}
/// Convenience method for a pairing check for four pairs.
bool PairingProd4(const G1 &a1, const G2 &a2, const G1 &b1, const G2 &b2,
                  const G1 &c1, const G2 &c2, const G1 &d1, const G2 &d2) {
  std::array<G1, 4> g1 {a1, b1, c1, d1};
  std::array<G2,4> g2 {a2, b2, c2, d2};
  return bn256::pairing(g1, g2);
}
};  // namespace pairing
}  // namespace zk
}  // namespace bn256
}  // namespace crypto
}  // namespace platon
