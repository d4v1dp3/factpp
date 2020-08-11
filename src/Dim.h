#ifndef FACT_Dim
#define FACT_Dim

#include "DimSetup.h"

#include <string>
#include <vector>

#include "dic.hxx"

namespace Dim
{
    // --------------------------------------------------------------------------
    //
    //! Simplification wrapper to send a command without data
    //!
    //! Example:
    //!   - Dim::SendCommand("SERVER/COMMAND");
    //!
    //! @param command
    //!     Dim command identifier
    //!
    //! @returns
    //!     the return value of DimClient::sendCommand
    //!
    inline bool SendCommand(const std::string &command)
    {
        return DimClient::sendCommand(command.c_str(), NULL, 0);
    }
    inline void SendCommandNB(const std::string &command)
    {
        DimClient::sendCommandNB(command.c_str(), NULL, 0);
    }

    // --------------------------------------------------------------------------
    //
    //! Simplification wrapper to send a command with the given data
    //!
    //! Example:
    //!   - Dim::SendCommand("SERVER/COMMAND", uint16_t(42));
    //!   - struct tm t; Dim::SendCommand("SERVER/TIME", t);
    //!
    //! @param command
    //!     Dim command identifier
    //!
    //! @param t
    //!     object to be sent, the pointer to the data to be sent is
    //!     set to &t
    //!
    //! @tparam T
    //!     type of the data to be sent. The size of the data to be sent
    //!     is determined as sizeof(T)
    //!
    //! @returns
    //!     the return value of DimClient::sendCommand
    //!
    template<typename T>
        inline bool SendCommand(const std::string &command, const T &t)
    {
        return DimClient::sendCommand(command.c_str(), const_cast<T*>(&t), sizeof(t));
    }

    template<>
        inline bool SendCommand(const std::string &command, const std::string &t)
    {
        return DimClient::sendCommand(command.c_str(), const_cast<char*>(t.c_str()), t.length()+1);
    }

    template<typename T>
        inline bool SendCommand(const std::string &command, const std::vector<T> &v)
    {
        return DimClient::sendCommand(command.c_str(), const_cast<char*>(v.data()), v.size()*sizeof(T));
    }

    inline bool SendCommand(const std::string &command, const void *d, size_t s)
    {
        return DimClient::sendCommand(command.c_str(), const_cast<void*>(d), s);
    }

    // -------------------------------------------------------------------------

    template<typename T>
        inline void SendCommandNB(const std::string &command, const T &t)
    {
        DimClient::sendCommandNB(command.c_str(), const_cast<T*>(&t), sizeof(t));
    }

    template<>
        inline void SendCommandNB(const std::string &command, const std::string &t)
    {
        DimClient::sendCommandNB(command.c_str(), const_cast<char*>(t.c_str()), t.length()+1);
    }

    template<typename T>
        inline void SendCommandNB(const std::string &command, const std::vector<T> &v)
    {
        DimClient::sendCommandNB(command.c_str(), const_cast<T*>(v.data()), v.size()*sizeof(T));
    }

    inline void SendCommandNB(const std::string &command, const void *d, size_t s)
    {
        DimClient::sendCommandNB(command.c_str(), const_cast<void*>(d), s);
    }
}

#endif
