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
#include <SFML/Window/WindowStyle.hpp> // important to be included first (conflict with None)
#include <SFML/Window/Unix/DRM/WindowImplDRM.hpp>
#include <SFML/Window/Unix/DRM/InputImplUDev.hpp>
#include <SFML/Window/Unix/DRM/DRMContext.hpp>
#include <SFML/Window/Unix/DRM/drm-common.h>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Err.hpp>

////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////
namespace sf
{
namespace priv
{

////////////////////////////////////////////////////////////
WindowImplDRM::WindowImplDRM(WindowHandle handle)
    : WindowImpl(),
    m_size( 0, 0 )
{
    sf::priv::InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplDRM::WindowImplDRM(VideoMode mode, const String& title, unsigned long style, const ContextSettings& settings)
    : WindowImpl(),
    m_size( mode.width, mode.height )

{
    sf::priv::InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplDRM::~WindowImplDRM()
{
    sf::priv::InputImpl::restoreTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplDRM::getSystemHandle() const
{
    struct drm *drm = sf::priv::DRMContext::get_drm();
    return (WindowHandle)drm->fd;
}

////////////////////////////////////////////////////////////
Vector2i WindowImplDRM::getPosition() const
{
    return Vector2i(0, 0);
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setPosition(const Vector2i& position)
{
}


////////////////////////////////////////////////////////////
Vector2u WindowImplDRM::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setSize(const Vector2u& size)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setTitle(const String& title)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setIcon(unsigned int width, unsigned int height, const Uint8* pixels)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setVisible(bool visible)
{
}

////////////////////////////////////////////////////////////
void WindowImplDRM::setMouseCursorVisible(bool visible)
{
    // TODO: not implemented
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setKeyRepeatEnabled(bool enabled)
{
    // TODO: not implemented
}


////////////////////////////////////////////////////////////
void WindowImplDRM::requestFocus()
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool WindowImplDRM::hasFocus() const
{
    return true;
}

void WindowImplDRM::processEvents()
{
    sf::Event ev;
    while ( sf::priv::InputImpl::checkEvent( ev ) )
        pushEvent( ev );
}

void WindowImplDRM::setMouseCursorGrabbed(bool grabbed)
{
    //TODO: not implemented
}

} // namespace priv
} // namespace sf
