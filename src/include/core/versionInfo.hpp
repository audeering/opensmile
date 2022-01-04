
#ifndef __CORE_VERSION_INFO_HPP
#define __CORE_VERSION_INFO_HPP

#define APPNAME "openSMILE"
#define APPVERSION "3.0.1"
#define APPCPAUTHOR "audEERING GmbH"
#define APPCPYEAR "2022"

#include <core/git_version.hpp>

#ifndef OPENSMILE_BUILD_BRANCH
#define OPENSMILE_BUILD_BRANCH "unknown"
#endif
#ifndef OPENSMILE_BUILD_DATE
#define OPENSMILE_BUILD_DATE "unknown"
#endif
#ifndef OPENSMILE_SOURCE_REVISION
#define OPENSMILE_SOURCE_REVISION "unknown"
#endif

#endif  // __CORE_VERSION_INFO_HPP
