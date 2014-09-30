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

#include "Exception.h"

UTF8::Exception::Exception(const std::string & error, const int & StatusCode)
{
    this->error = error;
    this->StatusCode = StatusCode;
}

UTF8::Exception::Exception(std::string error)
{
    this->error = error;
    this->StatusCode = UnspecifiedException;
}

UTF8::Exception::Exception(const UTF8::Exception & e)
{
    error = e.error;
    StatusCode = e.StatusCode;
}

std::string UTF8::Exception::GetErrorString() const
{
    return error;
}

int UTF8::Exception::GetErrorCode() const
{
    return StatusCode;
}

UTF8::Exception & UTF8::Exception::operator =(const UTF8::Exception & e)
{
    error = e.error;
    error = e.StatusCode;
    return *this;
}
