#ifndef UTILS_EXCEPTION_HPP
#define UTILS_EXCEPTION_HPP

#include <exception>
#include <string>

// `DEFINE_EXCEPTION_CLASS(A)` creates an `A_TEMPLATE<>` class and an `A` alias
// which hides the template parameter. This way we can define exceptions in
// headers only and use it like a regular class using the alias. The point of
// this class in general is having different exception classes with a string
// parameter returned on calls to `what`.
//
// # Example usage:
// ```
// #include <iostream>
// #include "utils/exception.hpp"
// DEFINE_EXCEPTION_CLASS(MyException)
// int main(int argc, char* argv[]) {
//     (void)argc;
//     (void)argv;
//     try {
//         throw MyException("which is not really important.");
//     }
//     catch(MyException e) {
//         cerr << "Catched exception " << a.what() << std::endl;
//     }
// }
//
// ```
#define DEFINE_EXCEPTION_CLASS(CLASS_NAME)                                     \
  template <typename T = void>                                                 \
  class CLASS_NAME##_TEMPLATE : public std::exception {                        \
  private:                                                                     \
    std::string message;                                                       \
                                                                               \
  public:                                                                      \
    explicit CLASS_NAME##_TEMPLATE(std::string new_message)                    \
        : message(std::move(new_message)) {}                                   \
    const char *what() const noexcept override { return message.c_str(); }     \
  };                                                                           \
  using CLASS_NAME = CLASS_NAME##_TEMPLATE<>;

#endif
