#ifndef DELILAExceptions_hpp
#define DELILAExceptions_hpp 1

#include <stdexcept>
#include <string>

namespace DELILA
{

/**
 * @brief Base exception class for all DELILA-specific exceptions
 */
class DELILAException : public std::runtime_error
{
 public:
  explicit DELILAException(const std::string &msg) : std::runtime_error(msg) {}
};

/**
 * @brief Exception thrown when file operations fail
 */
class FileException : public DELILAException
{
 public:
  explicit FileException(const std::string &msg) : DELILAException(msg) {}
};

/**
 * @brief Exception thrown when configuration is invalid
 */
class ConfigException : public DELILAException
{
 public:
  explicit ConfigException(const std::string &msg) : DELILAException(msg) {}
};

/**
 * @brief Exception thrown when JSON parsing fails
 */
class JSONException : public DELILAException
{
 public:
  explicit JSONException(const std::string &msg) : DELILAException(msg) {}
};

/**
 * @brief Exception thrown when input validation fails
 */
class ValidationException : public DELILAException
{
 public:
  explicit ValidationException(const std::string &msg)
      : DELILAException(msg)
  {
  }
};

/**
 * @brief Exception thrown when array/vector bounds are violated
 */
class RangeException : public DELILAException
{
 public:
  explicit RangeException(const std::string &msg) : DELILAException(msg) {}
};

/**
 * @brief Exception thrown during data processing errors
 */
class ProcessingException : public DELILAException
{
 public:
  explicit ProcessingException(const std::string &msg) : DELILAException(msg)
  {
  }
};

}  // namespace DELILA

#endif
