#pragma once

#include <algorithm>
#include <climits>
#include "chain.hpp"

#include <concepts>
#include <vector>
#include <string>
namespace std {

template <size_t Bits, bool Signed>
class WideInteger {
 public:
  static constexpr size_t arr_size = Bits / 8;

 public:
  WideInteger() = default;

  template <size_t Bits2, bool Signed2>
  WideInteger(const WideInteger<Bits2, Signed2> &rhs) {
    size_t copy_size = std::min(arr_size, rhs.arr_size);

    // The left and right operands have the same sign
    if (Signed == Signed2) {
      negative_ = rhs.negative_;
      memcpy(&arr_[arr_size - copy_size], &rhs.arr_[rhs.arr_size - copy_size],
             copy_size);
    }

    // Unsigned on the left, signed on the right
    if (!Signed && Signed2) {
      if (rhs.negative_) {
        WideInteger<Bits2, Signed2> complement = (~rhs) + 1;
        memcpy(&arr_[arr_size - copy_size],
               &complement.arr_[complement.arr_size - copy_size], copy_size);
      } else {
        memcpy(&arr_[arr_size - copy_size], &rhs.arr_[rhs.arr_size - copy_size],
               copy_size);
      }
    }

    // Signed on the left, unsigned on the right
    if (Signed && !Signed2) {
      memcpy(&arr_[arr_size - copy_size], &rhs.arr_[rhs.arr_size - copy_size],
             copy_size);
    }
  }

  template <typename T,
            class = typename std::enable_if<
                std::numeric_limits<std::decay_t<T>>::is_integer ||
                std::numeric_limits<std::decay_t<T>>::is_iec559>::type>
  constexpr WideInteger(const T &num) {
    uint128_t temp_value = 0;
    if constexpr (Signed) {
      if (num < 0) {
        int128_t neg_value = num;
        negative_ = true;
        temp_value = -neg_value;
      } else {
        temp_value = num;
      }
    } else {
      temp_value = num;
    }

    for (int i = 1; temp_value != 0; temp_value >>= 8, i++) {
      arr_[arr_size - i] = temp_value & 0xff;
    }
  }

  // Type conversion function
  template <typename T,
            class = typename std::enable_if<
                std::numeric_limits<std::decay_t<T>>::is_integer ||
                std::numeric_limits<std::decay_t<T>>::is_iec559>::type>
  explicit operator T() const {
    uint128_t temp_value = 0;
    size_t copy_size = std::min(arr_size, size_t(16));
    for (int i = arr_size - copy_size; i < arr_size; i++) {
      temp_value = temp_value * 256 + arr_[i];
    }

    T result;
    if (*this < 0) {
      result = -temp_value;
    } else {
      result = temp_value;
    }

    return result;
  }

  explicit operator bool() const { return 0 != *this; }

  // Convert string to big integer
  WideInteger(const char *str, size_t str_len) {
    uint32_t operator_result = string_literal_operator(
        reinterpret_cast<const uint8_t *>(str), str_len, arr_, arr_size);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
  }

  // Conversion between byte streams
  WideInteger<Bits, Signed> &FromBigEndian(const std::vector<uint8_t> &bytes) {
    static_assert(!Signed, "Only unsigned numbers can do this");
    size_t bytes_size = bytes.size();
    size_t copy_size = std::min(arr_size, bytes_size);
    memset(arr_, 0, arr_size);
    memcpy(&arr_[arr_size - copy_size], &bytes[bytes_size - copy_size],
           copy_size);
    return *this;
  }

  std::vector<uint8_t> ToBigEndian() {
    static_assert(!Signed, "Only unsigned numbers can do this");

    std::vector<uint8_t> result;
    bool real_begin = false;
    for (int i = 0; i < arr_size; i++) {
      if (0 != arr_[i] && !real_begin) real_begin = true;
      if (real_begin) result.push_back(arr_[i]);
    }

    return result;
  }

  // Addition operator
  WideInteger<Bits, Signed> operator+(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::ADD);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator+=(const WideInteger<Bits, Signed> &rhs) {
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::ADD);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // Subtraction operator
  WideInteger<Bits, Signed> operator-(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::SUB);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator-=(const WideInteger<Bits, Signed> &rhs) {
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::SUB);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  constexpr WideInteger<Bits, Signed> operator-() const {
    WideInteger<Bits, Signed> result = *this;
    result.Neg();
    return result;
  }

  // Multiplication operator
  WideInteger<Bits, Signed> operator*(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::MUL);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator*=(const WideInteger<Bits, Signed> &rhs) {
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::MUL);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // Division operator
  WideInteger<Bits, Signed> operator/(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::DIV);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator/=(const WideInteger<Bits, Signed> &rhs) {
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::DIV);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // Modulo operator
  WideInteger<Bits, Signed> operator%(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::MOD);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator%=(const WideInteger<Bits, Signed> &rhs) {
    uint32_t operator_result =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::MOD);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // And operator
  WideInteger<Bits, Signed> operator&(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    result.negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::AND);
    return result;
  }

  constexpr WideInteger<Bits, Signed> &operator&=(
      const WideInteger<Bits, Signed> &rhs) noexcept {
    negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::AND);
    return *this;
  }

  // OR operator
  WideInteger<Bits, Signed> operator|(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    result.negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::OR);
    return result;
  }

  WideInteger<Bits, Signed> &operator|=(const WideInteger<Bits, Signed> &rhs) {
    negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::OR);
    return *this;
  }

  // XOR operator
  WideInteger<Bits, Signed> operator^(
      const WideInteger<Bits, Signed> &rhs) const {
    WideInteger<Bits, Signed> result;
    result.negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                result.arr_, arr_size, BinaryOperator::XOR);
    return result;
  }

  WideInteger<Bits, Signed> &operator^=(const WideInteger<Bits, Signed> &rhs) {
    negative_ =
        bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_,
                                arr_, arr_size, BinaryOperator::XOR);
    return *this;
  }

  // Negation operator
  WideInteger<Bits, Signed> operator~() const {
    WideInteger<Bits, Signed> result;
    if constexpr (Signed) {
      WideInteger<Bits, Signed> temp = -1;
      result = *this | temp;
    } else {
      WideInteger<Bits, Signed> temp;
      temp.Fill();
      result = *this | temp;
    }
    return result;
  }

  // Parity operator
  bool operator==(const WideInteger<Bits, Signed> &rhs) const {
    int result =
        bigint_cmp(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_size);
    return 0 == result;
  }

  bool operator!=(const WideInteger<Bits, Signed> &rhs) const {
    return !(*this == rhs);
  }

  // Comparison operator
  bool operator<(const WideInteger<Bits, Signed> &rhs) const {
    int result =
        bigint_cmp(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_size);
    return -1 == result;
  }

  bool operator>(const WideInteger<Bits, Signed> &rhs) const {
    return !(*this == rhs) && !(*this < rhs);
  }

  bool operator>=(const WideInteger<Bits, Signed> &rhs) const {
    return *this == rhs || *this > rhs;
  }

  bool operator<=(const WideInteger<Bits, Signed> &rhs) const {
    return *this == rhs || *this < rhs;
  }

  // Increment operator
  WideInteger<Bits, Signed> &operator++() {
    *this += WideInteger<Bits, Signed>(1);
    return *this;
  };

  WideInteger<Bits, Signed> &operator++(int) {
    WideInteger<Bits, Signed> result = *this;
    *this += WideInteger<Bits, Signed>(1);
    return result;
  };

  // Decrement operator
  WideInteger<Bits, Signed> &operator--() {
    *this -= WideInteger<Bits, Signed>(1);
    return *this;
  };

  WideInteger<Bits, Signed> &operator--(int) {
    WideInteger<Bits, Signed> result = *this;
    *this -= WideInteger<Bits, Signed>(1);
    return result;
  };

  // Left shift operator
  WideInteger<Bits, Signed> operator<<(uint32_t n) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result = bigint_sh(arr_, negative_, n, &(result.arr_[0]),
                                         arr_size, ShiftDirection::LEFT);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator<<=(uint32_t n) {
    uint32_t operator_result =
        bigint_sh(arr_, negative_, n, arr_, arr_size, ShiftDirection::LEFT);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // Right shift operator
  WideInteger<Bits, Signed> operator>>(uint32_t n) const {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result = bigint_sh(arr_, negative_, n, &(result.arr_[0]),
                                         arr_size, ShiftDirection::RIGHT);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  WideInteger<Bits, Signed> &operator>>=(uint32_t n) {
    uint32_t operator_result =
        bigint_sh(arr_, negative_, n, arr_, arr_size, ShiftDirection::RIGHT);
    negative_ = BigintResultFlag::NEGATIVE & operator_result;
    overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return *this;
  }

  // zk algorithm custom operator
  friend WideInteger<Bits, Signed> Exp(const WideInteger<Bits, Signed> &left,
                                       const WideInteger<Bits, Signed> &right,
                                       const WideInteger<Bits, Signed> &mod) {
    WideInteger<Bits, Signed> result;
    uint32_t operator_result = bigint_exp_mod(
        &(left.arr_[0]), left.negative_, &(right.arr_[0]), right.negative_,
        &(mod.arr_[0]), mod.negative_, result.arr_, arr_size);
    result.negative_ = BigintResultFlag::NEGATIVE & operator_result;
    result.overflow_ = BigintResultFlag::OVERFLOW & operator_result;
    return result;
  }

  bool Overflow() { return overflow_; }

  const uint8_t *Values() const { return arr_; }
  uint8_t *Values() {return arr_; }

 private:
  void Neg() { negative_ = !negative_; }

  void Fill() {
    for (int i = 0; i < arr_size; i++) {
      arr_[i] = 0xff;
    }
  }

 private:
  bool negative_ = false;
  bool overflow_ = false;
  uint8_t arr_[arr_size] = {};
};

// Type instance
template <size_t Bits>
using WideInt = WideInteger<Bits, true>;

template <size_t Bits>
using WideUint = WideInteger<Bits, false>;

using int256_t = WideInt<256>;
using uint256_t = WideUint<256>;

using int512_t = WideInt<512>;
using uint512_t = WideUint<512>;

template <size_t Bits, bool Signed>
string to_string(const WideInteger<Bits, Signed> &value) {
  const char *charmap = "0123456789";
  string result;
  WideInteger<Bits, Signed> helper = value;
  if (helper < 0) helper = -helper;
  do {
    result += charmap[int(helper % 10)];
    helper /= 10;
  } while (helper != 0);
  reverse(result.begin(), result.end());

  if (value < 0) result = "-" + result;
  return result;
}

}  // namespace std

std::int256_t operator"" _int256(const char *str_value, size_t n) {
  std::int256_t result(str_value, n);
  return result;
}

std::int512_t operator"" _int512(const char *str_value, size_t n) {
  std::int512_t result(str_value, n);
  return result;
}

std::uint256_t operator"" _uint256(const char *str_value, size_t n) {
  std::uint256_t result(str_value, n);
  return result;
}

std::uint512_t operator"" _uint512(const char *str_value, size_t n) {
  std::uint512_t result(str_value, n);
  return result;
}
