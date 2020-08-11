// **************************************************************************
/** @namespace FACT

@brief Namespace to help with some general things in the program initialization

*/
// **************************************************************************
#include "FACT.h"

#include <iostream>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

// --------------------------------------------------------------------------
//
//! Print version information about FACT++
//!
//! From help2man:
//!
//! The first line of the --version information is assumed to be in one
//! of the following formats:
//!
//! \verbatim
//!  - <version>
//!  - <program> <version>
//!  - {GNU,Free} <program> <version>
//!  - <program> ({GNU,Free} <package>) <version>
//!  - <program> - {GNU,Free} <package> <version>
//! \endverbatim
//!
//!  and separated from any copyright/author details by a blank line.
//!
//! Handle multi-line bug reporting sections of the form:
//!
//! \verbatim
//!  - Report <program> bugs to <addr>
//!  - GNU <package> home page: <url>
//!  - ...
//! \endverbatim
//!
//! @param name
//!     name of the program (usually argv[0]). A possible leading "lt-"
//!     is removed.
//!
void FACT::PrintVersion(const char *name)
{
#if BOOST_VERSION < 104600
    const std::string n = boost::filesystem::path(name).filename();
#else
    const std::string n = boost::filesystem::path(name).filename().string();
#endif

    std::cout <<
        n << " - " PACKAGE_STRING "\n"
        "\n"
        "Written by Thomas Bretz et al.\n"
        "\n"
        "Report bugs to <" PACKAGE_BUGREPORT ">\n"
        "Home page: " PACKAGE_URL "\n"
        "\n"
        "Copyright (C) 2011 by the FACT Collaboration.\n"
        "This is free software; see the source for copying conditions.\n"
        << std::endl;
}
