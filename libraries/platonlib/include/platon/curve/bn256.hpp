
#pragma once

#include <span>
#include "platon/bigint.hpp"
#include "platon/chain.hpp"
#include "platon/print.hpp"
namespace platon {
namespace bn256 {
class G2;
class G1 {
 public:
  /**
   * @brief maps base field element into the G1 point
   *
   * @param k base field element
   */
  G1(const std::uint256_t& k) {
    ::bn256_map_g1(k.Values(), 32, x_.Values(), y_.Values());
  }

  G1(const std::uint256_t& x, const std::uint256_t& y) : x_(x), y_(y) {}

  G1(const G1& g) : x_(g.x_), y_(g.y_) {}

  G1(const G1&& g) : x_(std::move(g.x_)), y_(std::move(g.y_)) {}

  G1& operator=(const G1& other) {
    this->x_ = other.x_;
    this->y_ = other.y_;
    return *this;
  }

  G1& operator=(const G1&& other) {
    this->x_ = std::move(other.x_);
    this->y_ = std::move(other.y_);
    return *this;
  }

  bool operator==(const G1& g1) {
    return this->x_ == g1.x_ && this->y_ == g1.y_;
  }

  /**
    * @brief to perform point addition on a curve defined over prime field
    *
    * @param other point on a bn256 curve
    * @return -1 argument error, 0 success
    *
    * Example:
    *
    * @code
    * G1
    g1("11169198337205317385038692134282557493133418158128574038999810944352461077961"_uint256,
        "2820885980102468247213289930888494165190958823101043243711917453290081841766"_uint256);
    * G1
    g2("3510227910005969626168871163796842095937160976810256674232777209574668193517"_uint256,
        "2885800476071299445182650755020278501280179672256593791439003311512581969879"_uint256);
    * g1.Add(g2);
    * @endcode
    *
    */
  int Add(const G1& other) {
    return ::bn256_g1_add(this->x_.Values(), this->y_.Values(),
                          other.x_.Values(), other.y_.Values(),
                          this->x_.Values(), this->y_.Values());
  }

  /**
  * @brief  to perform point multiplication on a curve defined over prime field
  *
  * @param scalar scalar value
  * @return -1 argument error, 0 success
  *
  * Example:
  *
  * @code
  * G1
  g1("11169198337205317385038692134282557493133418158128574038999810944352461077961"_uint256,
      "2820885980102468247213289930888494165190958823101043243711917453290081841766"_uint256);
  * G1
  g2("3510227910005969626168871163796842095937160976810256674232777209574668193517"_uint256,
      "2885800476071299445182650755020278501280179672256593791439003311512581969879"_uint256);
  * g1.ScalarMul(g2);
  * @endcode
  *
  */
  int ScalarMul(const std::uint256_t& scalar) {
    return ::bn256_g1_mul(this->x_.Values(), this->y_.Values(), scalar.Values(),
                          this->x_.Values(), this->y_.Values());
  }

  std::uint256_t& X() { return x_; }

  std::uint256_t& Y() { return y_; }
  friend int pairing(const std::span<G1> g1, const std::span<G2> g2);

 private:
  std::uint256_t x_;
  std::uint256_t y_;
};

class G2 {
 public:
  /**
   * @brief maps base field element into the G1 point
   *
   * @param k base field element
   */
  G2(const std::uint256_t& k) {
    ::bn256_map_g2(k.Values(), 32, x_[0].Values(), y_[0].Values(),
                   x_[1].Values(), y_[1].Values());
  }

  G2(const std::uint256_t& x1, const std::uint256_t& y1,
     const std::uint256_t& x2, const std::uint256_t& y2) {
    x_[0] = x1;
    y_[0] = y1;
    x_[1] = x2;
    y_[1] = y2;
  }

  G2(const G2& other) {
    x_[0] = other.x_[0];
    y_[0] = other.y_[0];
    x_[1] = other.x_[1];
    y_[1] = other.y_[1];
  }

  G2(const G2&& other) {
    x_[0] = std::move(other.x_[0]);
    y_[0] = std::move(other.y_[0]);
    x_[1] = std::move(other.x_[1]);
    y_[1] = std::move(other.y_[1]);
  }

  G2& operator=(const G2& other) {
    x_[0] = other.x_[0];
    y_[0] = other.y_[0];
    x_[1] = other.x_[1];
    y_[1] = other.y_[1];
    return *this;
  }

  G2& operator=(const G2&& other) {
    x_[0] = std::move(other.x_[0]);
    y_[0] = std::move(other.y_[0]);
    x_[1] = std::move(other.x_[1]);
    y_[1] = std::move(other.y_[1]);
    return *this;
  }

  bool operator==(const G2& other) {
    return x_[0] == other.x_[0] && y_[0] == other.y_[0] &&
           x_[1] == other.x_[1] && y_[1] == other.y_[1];
  }

  /**
  * @brief to perform point addition on a curve twist defined over quadratic
  extension of the base field
  *
  * @param other twist point on a bn256 curve
  * @return -1 argument error, 0 success
  *
  * Example:
  *
  * @code
  * G2
  g1("15108480512742213315752883275348732788054235236417790974273843355007533920341"_uint256,
  *
  "7087423052184812752123031502782215229502029213326381531963693605124277246488"_uint256,
  *
  "179853301121042839587000058223201906078907197410809512660027461703300819271"_uint256,
  *
  "11875742517771222702411659323252914306253772862490535628642116117134675636023"_uint256);
  * G2
  g2(19659919047260284629346094692123228544406542558959640466277696006830353021100"_uint256,
        "6025506379932237465385214201206527812848555421743710598697345947300890198457"_uint256,
        "20618922630881857155986522435001570406346639915859901858546051417452761970477"_uint256,
        "12957125681690452415152069341785043351437829970534786922248294088023103741201"_uint256);
  * g1.Add(g2);
  * @endcode
  *
  */
  int Add(const G2& other) {
    return ::bn256_g2_add(
        this->x_[0].Values(), this->y_[0].Values(), this->x_[1].Values(),
        this->y_[1].Values(), other.x_[0].Values(), other.y_[0].Values(),
        other.x_[1].Values(), other.y_[1].Values(), this->x_[0].Values(),
        this->y_[0].Values(), this->x_[1].Values(), this->y_[1].Values());
  }

  /**
  * @brief to perform point multiplication on a curve twist defined over
  quadratic extension of the base field
  *
  * @param scalar scalar value
  * @return -1 argument error, 0 success
  *
  * Example:
  *
  * @code
  * G2
  g1("15108480512742213315752883275348732788054235236417790974273843355007533920341"_uint256,
  *
  "7087423052184812752123031502782215229502029213326381531963693605124277246488"_uint256,
  *
  "179853301121042839587000058223201906078907197410809512660027461703300819271"_uint256,
  *
  "11875742517771222702411659323252914306253772862490535628642116117134675636023"_uint256);
  * G2
  g2(19659919047260284629346094692123228544406542558959640466277696006830353021100"_uint256,
        "6025506379932237465385214201206527812848555421743710598697345947300890198457"_uint256,
        "20618922630881857155986522435001570406346639915859901858546051417452761970477"_uint256,
        "12957125681690452415152069341785043351437829970534786922248294088023103741201"_uint256);
  * g1.ScalarMul(g2);
  * @endcode
  *
  */
  int ScalarMul(const std::uint256_t& scalar) {
    return ::bn256_g2_mul(
        this->x_[0].Values(), this->y_[0].Values(), this->x_[1].Values(),
        this->y_[1].Values(), scalar.Values(), this->x_[0].Values(),
        this->y_[0].Values(), this->x_[1].Values(), this->y_[1].Values());
  }

  std::uint256_t& X1() { return x_[0]; }

  std::uint256_t& Y1() { return y_[0]; }

  std::uint256_t& X2() { return x_[1]; }

  std::uint256_t& Y2() { return y_[1]; }
  friend int pairing(const std::span<G1> g1, const std::span<G2> g2);

 private:
  std::uint256_t x_[2];
  std::uint256_t y_[2];
};

/**
 * @brief  to perform a pairing operations between a set of pairs of (G1, G2)
 *points
 * @param g1 list of G1
 * @param g2 list of G2
 * @return -2 argument error -1 failed 0 success
 **/
int pairing(const std::span<G1> g1, const std::span<G2> g2) {
  size_t len = g1.size();
  size_t size = sizeof(uint8_t*) * len;
  uint8_t** x1 = (uint8_t**)malloc(size);
  uint8_t** y1 = (uint8_t**)malloc(size);
  uint8_t** x11 = (uint8_t**)malloc(size);
  uint8_t** y11 = (uint8_t**)malloc(size);
  uint8_t** x12 = (uint8_t**)malloc(size);
  uint8_t** y12 = (uint8_t**)malloc(size);
  for (size_t i = 0; i < len; i++) {
    x1[i] = g1[i].x_.Values();
    y1[i] = g1[i].y_.Values();
    x11[i] = g2[i].x_[0].Values();
    y11[i] = g2[i].y_[0].Values();
    x12[i] = g2[i].x_[1].Values();
    y12[i] = g2[i].y_[1].Values();
  }

  return ::bn256_pairing(x1, y1, x11, y11, x12, y12, len);
}
}  // namespace bn256
}  // namespace platon