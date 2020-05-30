////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
//
// Linux DRM implementation
// Copyright (C) 2020 Andrew Mickelson
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/VideoModeImpl.hpp>
#include <SFML/Window/Unix/DRM/DRMContext.hpp>
#include <SFML/Window/Unix/DRM/drm-common.h>
#include <SFML/System/Err.hpp>

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
std::vector<VideoMode> VideoModeImpl::getFullscreenModes()
{
    std::vector<VideoMode> modes;
    modes.push_back(getDesktopMode());
    return modes;
}


////////////////////////////////////////////////////////////
VideoMode VideoModeImpl::getDesktopMode()
{
    struct drm *drm = sf::priv::DRMContext::get_drm();
    return VideoMode( drm->mode->hdisplay, drm->mode->vdisplay );
}

} // namespace priv

} // namespace sf
