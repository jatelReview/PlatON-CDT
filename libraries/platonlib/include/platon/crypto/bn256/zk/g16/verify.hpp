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
G1 Addition(const G1 &p1, const G1 &p2) {
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
  return bn256::pairing(g1,g2) == 0;
}
/// Convenience method for a pairing check for three pairs.
bool PairingProd3(const G1 &a1, const G2 &a2, const G1 &b1, const G2 &b2,
                  const G1 &c1, const G2 &c2) {
  std::array<G1, 3> g1 {a1, b1, c1};
  std::array<G2, 3> g2 {a2, b2, c2};
  return bn256::pairing(g1, g2) == 0;
}
/// Convenience method for a pairing check for four pairs.
bool PairingProd4(const G1 &a1, const G2 &a2, const G1 &b1, const G2 &b2,
                  const G1 &c1, const G2 &c2, const G1 &d1, const G2 &d2) {
  std::array<G1, 4> g1 {a1, b1, c1, d1};
  std::array<G2,4> g2 {a2, b2, c2, d2};
  return bn256::pairing(g1, g2) == 0;
}
};  // namespace pairing

class Verifier {
 public:
  struct VerifyingKey {
    G1 alpha;
    G2 beta;
    G2 gamma;
    G2 delta;
    G1 gamma_abc[3];
  };
  struct Proof {
    G1 a;
    G2 b;
    G1 c;
  };
  VerifyingKey GetVerifyingKey() {
    return VerifyingKey{
        G1{std::uint256_t("0x1936c240636390dc823e3a728e94b208eb53c6756d81da57ec3425e05d43ac10"),
           std::uint256_t("0x2d70ff78e8216bf29d58923a686d9738278b8ce2fd822e197c85b09286d15566")},
        G2(
            std::uint256_t("0x2b4daf047abe2e7f0b311118c1b963b63695dc0d769cea78849604434de055bf"),
            std::uint256_t("0x29c13ecb6f33dbc4b3b8a02e2e255511ce4c26a8a2f299efcc94caf2de4fce00"),
            std::uint256_t("0x1da9020008df7f549751f8a251af3b2dc4a2ad3e0870de54acaedd9fc1b47e17"),
           std::uint256_t("0x25ea0d7e2b29de431b86a943db30dbf4d98f68df9ca8a9628d14d1591e817d90")
           ),
        G2(
            std::uint256_t("0x011016e22ae045444f50fb80f246ec486c7e02af09132cd38c4fcf484983e4f2"),
            std::uint256_t("0x00e83c788c2878d1d5eba3ed49b0d81e4c0487dedc3e4d1c2baab5833785b62f"),
            std::uint256_t("0x05eb89e741ed5b5d611cebf92d1ed02cd6f3311089f0d400df7d9ced5a48fd41"),
           std::uint256_t("0x132a90a3b0d369ccd66e2a5ba04a935e44d8ad5dca93a76bba592a578130a911")),
        G2(
            std::uint256_t("0x065f6a3323a2abffd621fc263f348eb914904b68d5897729ae34a6b9d33f0852"),
            std::uint256_t("0x0c3b60f59d3bd50328a04c0ff6d979199685d0526f89f6ac29d6174ce24707a2"),
            std::uint256_t("0x26e7ebce2b44efef6b6315938e33f0a8ecc82dbad635c9efa681ed85bbb59982"),
           std::uint256_t("0x12e0f3721230a0f38f6c9913048d5230fd2615ef3ff7f6ee4b20dfe0bdea1a86")),
        //        vk.gamma_abc = new G1[3];
        {G1{"14009039398155720505150269708790661333297837075728094920382828224942856766144"_uint256,
            "20930955662491267233397107828124586351105651506146609020125271365893061173293"_uint256},
         G1{"5851231919488061643454688318825712062697499020691897643701372875390621893321"_uint256,
            "16233473441361394560431684535997736703822957284610781662173469397325430267528"_uint256},
         G1{"1385197555366316638547884976923676458145592326878252587889427394007889814716"_uint256,
            "2352687799100231607798767035998150582166744960824007121429034459320835851722"_uint256}}};
  }

  std::uint256_t Verify(const std::uint256_t input[], size_t len,
                        const Proof &proof) {
    std::uint256_t snark_scalar_field =
        "21888242871839275222246405745257275088548364400416034343698204186575808495617"_uint256;
    VerifyingKey vk = GetVerifyingKey();
    // Compute the linear combination vk_x
    G1 vk_x = G1{0, 0};
    for (int i = 0; i < len; i++) {
      G1 p2 = pairing::ScalarMul(vk.gamma_abc[i + 1], input[i]);
      vk_x = pairing::Addition(
          vk_x, pairing::ScalarMul(vk.gamma_abc[i + 1], input[i]));
    }
    vk_x = pairing::Addition(vk_x, vk.gamma_abc[0]);

    if (!pairing::PairingProd4(proof.a, proof.b, pairing::Negate(vk_x),
                               vk.gamma, pairing::Negate(proof.c), vk.delta,
                               pairing::Negate(vk.alpha), vk.beta))
      return 1;
    return 0;
  }

  bool VerifyTx(const std::uint256_t a[2], const std::uint256_t b[2][2],
                const std::uint256_t c[2], const std::uint256_t input[2]) {
    Proof proof{G1{a[0], a[1]}, G2(b[0][0], b[0][1], b[1][0], b[1][1]),
                G1{c[0], c[1]}};

    return Verify(input, 2, proof) == 0;
  }
};
}  // namespace zk
}  // namespace bn256
}  // namespace crypto
}  // namespace platon
