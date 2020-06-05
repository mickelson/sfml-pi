////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
//
// Raspberry Pi dispmanx implementation
// Copyright (C) 2016 Andrew Mickelson
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
#include <SFML/Window/Unix/RPi/WindowImplRPi.hpp>
#include <SFML/Window/Unix/DRM/InputImplUDev.hpp>
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
WindowImplRPi::WindowImplRPi(WindowHandle handle)
{
    m_display = 0;
    m_nativeWindow.element = 0;
    m_nativeWindow.width = 0;
    m_nativeWindow.height = 0;

    sf::priv::InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplRPi::WindowImplRPi(VideoMode mode, const String& title, unsigned long style, const ContextSettings& settings)
{
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    uint32_t screen_width;
    uint32_t screen_height;

    VC_DISPMANX_ALPHA_T dispman_alpha;

    // Disable alpha to prevent app looking composed on whatever dispman
    // is showing - lifted from SDL source: video/raspberry/SDL_rpivideo.c
    //
    dispman_alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
    dispman_alpha.opacity = 0xFF;
    dispman_alpha.mask = 0;

    // create an EGL window surface
    graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = mode.width;
    dst_rect.height = mode.height;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = mode.width << 16;
    src_rect.height = mode.height << 16;

    m_display = vc_dispmanx_display_open( 0 /* LCD */);
    dispman_update = vc_dispmanx_update_start( 0 );

    dispman_element = vc_dispmanx_element_add( dispman_update, m_display,
       0/*layer*/, &dst_rect, 0/*src*/,
       &src_rect, DISPMANX_PROTECTION_NONE, &dispman_alpha, 0/*clamp*/, DISPMANX_NO_ROTATE );

    m_nativeWindow.element = dispman_element;
    m_nativeWindow.width = mode.width;
    m_nativeWindow.height = mode.height;
    vc_dispmanx_update_submit_sync( dispman_update );

    sf::priv::InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplRPi::~WindowImplRPi()
{
    if ( m_nativeWindow.element )
    {
        DISPMANX_UPDATE_HANDLE_T dispman_update;
        dispman_update = vc_dispmanx_update_start( 0 );
        vc_dispmanx_element_remove( dispman_update, m_nativeWindow.element );
        vc_dispmanx_update_submit_sync( dispman_update );
    }

    if ( m_display )
        vc_dispmanx_display_close( m_display );

    sf::priv::InputImpl::restoreTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplRPi::getSystemHandle() const
{
    // it's not a duck!
    return (WindowHandle)&m_nativeWindow;
}

////////////////////////////////////////////////////////////
Vector2i WindowImplRPi::getPosition() const
{
    // Not applicable
    return Vector2i(0, 0);
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setPosition(const Vector2i& position)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
Vector2u WindowImplRPi::getSize() const
{
    return sf::Vector2u( m_nativeWindow.width, m_nativeWindow.height );
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setSize(const Vector2u& size)
{
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setTitle(const String& title)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setIcon(unsigned int width, unsigned int height, const Uint8* pixels)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setVisible(bool visible)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setMouseCursorVisible(bool visible)
{
    // TODO: not implemented
}


////////////////////////////////////////////////////////////
void WindowImplRPi::setKeyRepeatEnabled(bool enabled)
{
    // TODO: not implemented
}


////////////////////////////////////////////////////////////
void WindowImplRPi::requestFocus()
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool WindowImplRPi::hasFocus() const
{
    return true;
}

void WindowImplRPi::processEvents()
{
    sf::Event ev;
    while ( sf::priv::InputImpl::checkEvent( ev ) )
        pushEvent( ev );
}

void WindowImplRPi::setMouseCursorGrabbed(bool grabbed)
{
    //TODO: not implemented
}

} // namespace priv
} // namespace sf
