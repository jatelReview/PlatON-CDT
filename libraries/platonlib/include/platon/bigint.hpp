#pragma once

#include <climits>
#include <algorithm>
#include "chain.hpp"

#include <string>

namespace std
{

  template <size_t Bits, bool Signed>
  class wide_integer
  {
  public:
    static constexpr size_t arr_size = Bits / 8;

  public:
    wide_integer() = default;

    template <size_t Bits2, bool Signed2>
    constexpr wide_integer(const wide_integer<Bits2, Signed2> &rhs) noexcept
    {
      size_t copy_size = std::min(arr_size, rhs.arr_size);
      memset(arr_, 0, arr_size);

      // The left and right operands have the same sign
      if (Signed == Signed2)
      {
        negative_ = rhs.negative_;
        for (int i = 1; i <= copy_size; ++i)
        {
          arr_[arr_size - i] = rhs.arr_[arr_size - i];
        }
      }

      // Unsigned on the left, signed on the right
      if (Signed && !Signed2)
      {
        if (rhs.negative_)
        {
          wide_integer<Bits2, Signed2> complement = (~rhs) + 1;
          for (int i = 1; i <= copy_size; ++i)
          {
            arr_[arr_size - i] = rhs.arr_[arr_size - i];
          }
        }
        else
        {
          for (int i = 1; i <= copy_size; ++i)
          {
            arr_[arr_size - i] = rhs.arr_[arr_size - i];
          }
        }
      }

      // Signed on the left, unsigned on the right
      if (!Signed && Signed2)
      {
        if (negative_)
        {
          negative_ = false;
        }
        for (int i = 1; i <= copy_size; ++i)
        {
          arr_[arr_size - i] = rhs.arr_[arr_size - i];
        }
      }
    }

    template <typename T, class = typename std::enable_if<
                              std::numeric_limits<std::decay_t<T>>::is_integer ||
                              std::numeric_limits<std::decay_t<T>>::is_iec559>::type>
    constexpr wide_integer(const T &num)
    {
      uint128_t temp_value = 0;
      if constexpr (Signed)
      {
        if (num < 0)
        {
          int128_t neg_value = num;
          negative_ = true;
          temp_value = -neg_value;
        }
        else
        {
          temp_value = num;
        }
      }
      else
      {
        temp_value = num;
      }

      for (int i = 1; temp_value != 0; temp_value >>= 8, i++)
      {
        arr_[arr_size - i] = temp_value & 0xff;
      }
    }

    // Type conversion function
    template <typename T, class = typename std::enable_if<
                              std::numeric_limits<std::decay_t<T>>::is_integer ||
                              std::numeric_limits<std::decay_t<T>>::is_iec559>::type>
    explicit constexpr operator T() const noexcept
    {
      uint128_t temp_value = 0;
      size_t copy_size = std::min(arr_size, size_t(16));
      for (int i = arr_size - copy_size; i < arr_size; i++)
      {
        temp_value = temp_value * 256 + arr_[i];
      }

      T result;
      if (*this < 0)
      {
        result = -temp_value;
      }
      else
      {
        result = temp_value;
      }

      return result;
    }

    explicit constexpr operator bool() const noexcept
    {
      return 0 != *this;
    }

    // Conversion between byte streams
    wide_integer<Bits, Signed> &fromBigEndian(const std::vector<uint8_t> &bytes)
    {
      static_assert(!Signed, "Only unsigned numbers can do this");
      size_t bytes_size = bytes.size();
      size_t copy_size = std::min(arr_size, bytes_size);
      memset(arr_, 0, arr_size);
      for (int i = 1; i <= copy_size; ++i)
      {
        arr_[arr_size - i] = bytes[bytes_size - i];
      }

      return *this;
    }

    std::vector<uint8_t> toBigEndian()
    {
      return std::vector<uint8_t>(&arr_[0], &arr_[arr_size - 1]);
    }

    // Addition operator
    constexpr wide_integer<Bits, Signed> operator+(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::ADD);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator+=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::ADD);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    // Subtraction operator
    constexpr wide_integer<Bits, Signed> operator-(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::SUB);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator-=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::SUB);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    constexpr wide_integer<Bits, Signed> operator-() const
    {
      wide_integer<Bits, Signed> result = *this;
      result.neg();
      return result;
    }

    // Multiplication operator
    constexpr wide_integer<Bits, Signed> operator*(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::MUL);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator*=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::MUL);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    // Division operator
    constexpr wide_integer<Bits, Signed> operator/(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::DIV);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator/=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::DIV);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    // Modulo operator
    constexpr wide_integer<Bits, Signed> operator%(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::MOD);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator%=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      uint32_t operator_result = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::MOD);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    // And operator
    constexpr wide_integer<Bits, Signed> operator&(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      result.negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::AND);
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator&=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::AND);
      return *this;
    }

    // OR operator
    constexpr wide_integer<Bits, Signed> operator|(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      result.negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::OR);
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator|=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::OR);
      return *this;
    }

    // XOR operator
    constexpr wide_integer<Bits, Signed> operator^(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      wide_integer<Bits, Signed> result;
      result.negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, result.arr_, arr_size, BinaryOperator::XOR);
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator^=(
        const wide_integer<Bits, Signed> &rhs) noexcept
    {
      negative_ = bigint_binary_operators(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_, arr_size, BinaryOperator::XOR);
      return *this;
    }

    // Negation operator
    constexpr wide_integer<Bits, Signed> operator~() const noexcept
    {
      wide_integer<Bits, Signed> result;
      if constexpr (Signed)
      {
        wide_integer<Bits, Signed> temp = -1;
        result = *this | temp;
      }
      else
      {
        wide_integer<Bits, Signed> temp;
        temp.fill();
        result = *this | temp;
      }
      return result;
    }

    // Parity operator
    constexpr bool operator==(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      int result = bigint_cmp(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_size);
      return 0 == result;
    }

    constexpr bool operator!=(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      return !(*this == rhs);
    }

    // Comparison operator
    constexpr bool operator<(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      int result = bigint_cmp(arr_, negative_, &(rhs.arr_[0]), rhs.negative_, arr_size);
      return -1 == result;
    }

    constexpr bool operator>(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      return !(*this == rhs) && !(*this < rhs);
    }

    constexpr bool operator>=(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      return *this == rhs || *this > rhs;
    }

    constexpr bool operator<=(
        const wide_integer<Bits, Signed> &rhs) const noexcept
    {
      return *this == rhs || *this < rhs;
    }

    // Increment operator
    constexpr wide_integer<Bits, Signed> &operator++() noexcept
    {
      *this += wide_integer<Bits, Signed>(1);
      return *this;
    };

    constexpr wide_integer<Bits, Signed> &operator++(int) noexcept
    {
      wide_integer<Bits, Signed> result = *this;
      *this += wide_integer<Bits, Signed>(1);
      return result;
    };

    // Decrement operator
    constexpr wide_integer<Bits, Signed> &operator--() noexcept
    {
      *this -= wide_integer<Bits, Signed>(1);
      return *this;
    };

    constexpr wide_integer<Bits, Signed> &operator--(int) noexcept
    {
      wide_integer<Bits, Signed> result = *this;
      *this -= wide_integer<Bits, Signed>(1);
      return result;
    };

    // Left shift operator
    constexpr wide_integer<Bits, Signed> operator<<(uint32_t n) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_sh(arr_, negative_, n, &(result.arr_[0]), arr_size, ShiftDirection::LEFT);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator<<=(uint32_t n) noexcept
    {
      uint32_t operator_result = bigint_sh(arr_, negative_, n, arr_, arr_size, ShiftDirection::LEFT);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    // Right shift operator
    constexpr wide_integer<Bits, Signed> operator>>(uint32_t n) const noexcept
    {
      wide_integer<Bits, Signed> result;
      uint32_t operator_result = bigint_sh(arr_, negative_, n, &(result.arr_[0]), arr_size, ShiftDirection::RIGHT);
      result.negative_ = 0x01 & operator_result;
      result.overflow_ = 0x02 & operator_result;
      return result;
    }

    constexpr wide_integer<Bits, Signed> &operator>>=(uint32_t n) noexcept
    {
      uint32_t operator_result = bigint_sh(arr_, negative_, n, arr_, arr_size, ShiftDirection::RIGHT);
      negative_ = 0x01 & operator_result;
      overflow_ = 0x02 & operator_result;
      return *this;
    }

    constexpr bool overflow() noexcept
    {
      return overflow_;
    }

  private:
    void neg()
    {
      negative_ = !negative_;
    }

    void fill()
    {
      for (int i = 0; i < arr_size; i++)
      {
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
  using wide_int = wide_integer<Bits, true>;

  template <size_t Bits>
  using wide_uint = wide_integer<Bits, false>;

  using int256_t = wide_int<256>;
  using uint256_t = wide_uint<256>;

  using int512_t = wide_int<512>;
  using uint512_t = wide_uint<512>;

  template <size_t Bits, bool Signed>
  string to_string(const wide_integer<Bits, Signed> &value)
  {
    const char *charmap = "0123456789";
    string result;
    wide_integer<Bits, Signed> helper = value;
    if (helper < 0)
      helper = -helper;
    do
    {
      result += charmap[int(helper % 10)];
      helper /= 10;
    } while (helper != 0);
    reverse(result.begin(), result.end());

    if (value < 0)
      result = "-" + result;
    return result;
  }

} // namespace std

constexpr std::int256_t operator"" _int256(const char *str_value, size_t n)
{
  std::int256_t result;
  for (int i = 0; i < n; i++)
  {
    if (i == n - 1 && '-' == str_value[n - 1 - i])
      continue;

    std::int256_t temp = str_value[n - 1 - i] - '0';
    for (int j = 0; j < i; j++)
    {
      temp *= 10;
      if (temp.overflow())
        break;
    }

    if (temp.overflow())
      break;
    result += temp;
  }

  if ('-' == str_value[0])
    result = -result;

  return result;
}

constexpr std::int512_t operator"" _int512(const char *str_value, size_t n)
{
  std::int512_t result;
  for (int i = 0; i < n; i++)
  {
    if (i == n - 1 && '-' == str_value[n - 1 - i])
      continue;

    std::int512_t temp = str_value[n - 1 - i] - '0';
    for (int j = 0; j < i; j++)
    {
      temp *= 10;
      if (temp.overflow())
        break;
    }

    if (temp.overflow())
      break;
    result += temp;
  }

  if ('-' == str_value[0])
    result = -result;

  return result;
}

constexpr std::uint256_t operator"" _uint256(const char *str_value, size_t n)
{
  std::uint256_t result;
  for (int i = 0; i < n; i++)
  {
    std::uint256_t temp = str_value[n - 1 - i] - '0';
    for (int j = 0; j < i; j++)
    {
      temp *= 10;
      if (temp.overflow())
        break;
    }

    if (temp.overflow())
      break;
    result += temp;
  }

  return result;
}

constexpr std::uint512_t operator"" _uint512(const char *str_value, size_t n)
{
  std::uint512_t result;
  for (int i = 0; i < n; i++)
  {
    std::uint512_t temp = str_value[n - 1 - i] - '0';
    for (int j = 0; j < i; j++)
    {
      temp *= 10;
      if (temp.overflow())
        break;
    }

    if (temp.overflow())
      break;
    result += temp;
  }

  return result;
}
