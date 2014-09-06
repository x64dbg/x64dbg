/**
 * UTF8 string library.
 *
 * Allows to use native UTF8 sequences as a string class. Has many overloaded
 * operators that provides such features as concatenation, types converting and
 * much more.
 *
 * Distributed under GPL v3
 *
 * Author:
 *      Grigory Gorelov (gorelov@grigory.info)
 *      See more information on grigory.info
 */

#ifndef _UTF8_Exception_H
#define _UTF8_Exception_H

#include <string>

namespace UTF8
{
/**
 *  Exception class. When something bad happens it is thowed by UTF8::String.
 */
class Exception
{
public:
    enum
    {
        UnspecifiedException = 1,
        StringToIntConversionError = 2,
        StringToDoubleConversionError = 3,
        FileNotFound = 4,
        StringIsNotACharacter = 5
    };

    /**
     *  Just a constructor
     */
    Exception(std::string error);

    /// Just a constructor
    Exception(const std::string & error, const int & StatusCode);

    /// Copying constructor
    Exception(const Exception & e);

    /// Returns error string
    std::string GetErrorString() const;

    /// Returns error code
    int GetErrorCode() const;

    /// Assing operator
    Exception & operator =(const Exception &);

private:
    std::string error;
    int StatusCode;

};
}

#endif  /* _EXCEPTION_H */
