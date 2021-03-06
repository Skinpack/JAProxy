#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

template<typename T>
struct Result {
    std::optional<T> result{};
    bool resultSuccess = false;
    std::optional<std::string> errorMessage{};

    template<typename TT>
    static inline Result success(TT && result)
        noexcept(std::is_nothrow_constructible_v<TT, decltype(std::forward<TT>(result))>)
    {
        return Result{ std::forward<TT>(result), true, {} };
    }

    template<typename TT>
    static inline Result fail(TT && result, std::string message)
        noexcept(std::is_nothrow_constructible_v<TT, decltype(std::forward<TT>(result))>)
    {
        return Result{ std::forward<TT>(result), false, std::move(message) };
    }

    static inline Result fail(std::string message) noexcept
    {
        return Result{ {}, false, std::move(message) };
    }

    template<typename TT, typename Pred>
    static inline Result successOnPred(TT && result, std::string_view message, const Pred & isSuccessPred)
    {
        if (isSuccessPred(result)) {
            return Result::success(std::forward<TT>(result));
        } else {
            return Result::fail(std::forward<TT>(result), std::string(message));
        }
    }

    template<typename TT, typename Pred, typename ErrGetterT>
    static inline Result successOnPredLazy(TT && result, const ErrGetterT & getErrorMessage, const Pred & isSuccessPred)
    {
        if (isSuccessPred(result)) {
            return Result::success(std::forward<TT>(result));
        } else {
            return Result::fail(std::forward<TT>(result), std::string(getErrorMessage(result)));
        }
    }

    template<typename TT, std::enable_if_t<
        std::is_convertible_v<TT, T> &&
        std::is_convertible_v<decltype(std::declval<TT>() == 0), bool>
        , int> = 0>
    static inline Result successOnZero(TT && result, std::string_view message = "")
    {
        return successOnPred(std::forward<TT>(result), message, [](const T & res) { return res == 0; });
    }

    template<typename TT, typename ErrGetterT, std::enable_if_t<
        std::is_convertible_v<TT, T> &&
        std::is_convertible_v<decltype(std::declval<TT>() == 0), bool>
        , int> = 0>
    static inline Result successOnZeroLazy(TT && result, const ErrGetterT & getErrorMessage)
    {
        if (result == 0) {
            return success(std::forward<TT>(result));
        } else {
            return fail(std::forward<TT>(result), getErrorMessage(result));
        }
    }

    constexpr bool isSuccess() const noexcept
    {
        return resultSuccess;
    }

    constexpr operator bool() const noexcept
    {
        return isSuccess();
    }

    constexpr bool has_value() const noexcept
    {
        return result.has_value();
    }

    constexpr T & value() & noexcept
    {
        return result.value();
    }

    constexpr const T & value() const & noexcept
    {
        return result.value();
    }

    constexpr T & operator*() & noexcept
    {
        return value();
    }

    constexpr const T & operator*() const & noexcept
    {
        return value();
    }

    constexpr T *operator->() & noexcept
    {
        return std::addressof(value());
    }

    constexpr const T *operator->() const & noexcept
    {
        return std::addressof(value());
    }

    std::string_view errorStr() const & noexcept
    {
        return errorMessage.value_or("(no error message)");
    }
};
