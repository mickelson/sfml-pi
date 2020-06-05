////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
//
// Linux UDEV keyboard input implementation
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
#include <SFML/Window/Unix/DRM/InputImplUDev.hpp>

#include <SFML/System/Mutex.hpp>
#include <SFML/System/Lock.hpp>
#include <SFML/System/Err.hpp>

#include <sstream>
#include <string>
#include <vector>
#include <queue>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <linux/input.h>
#include <errno.h>

namespace
{
    struct TouchSlot {
        int oldId;
        int id;
        sf::Vector2i pos;

        TouchSlot ()
        {
            oldId = -1;
            id = -1;
            pos.x = 0;
            pos.y = 0;
        }
    };

    sf::Mutex         inpMutex;                                // threadsafe? maybe...
    sf::Vector2i      mousePos;                                // current mouse position

    std::vector<int>  fds;                                     // list of open file descriptors for /dev/input
    std::vector<bool> mouseMap(sf::Mouse::ButtonCount, false); // track whether keys are down
    std::vector<bool> keyMap(sf::Keyboard::KeyCount, false);   // track whether mouse buttons are down

    int touchFd = -1;                                          // file descriptor we have seen MT events on; assumes only 1
    std::vector<TouchSlot> touchSlots;                         // track the state of each touch "slot"
    int currentSlot = 0;                                       // which slot are we currently updating?

    std::queue<sf::Event> eventQueue;                          // events received and waiting to be consumed
    const int MAX_QUEUE = 64;                                  // The maximum size we let eventQueue grow to

    termios newt, oldt;                                        // Terminal configurations

    bool altDown() { return ( keyMap[sf::Keyboard::LAlt] || keyMap[sf::Keyboard::RAlt] ); }
    bool controlDown() { return ( keyMap[sf::Keyboard::LControl] || keyMap[sf::Keyboard::RControl] ); }
    bool shiftDown() { return ( keyMap[sf::Keyboard::LShift] || keyMap[sf::Keyboard::RShift] ); }
    bool systemDown() { return ( keyMap[sf::Keyboard::LSystem] || keyMap[sf::Keyboard::RSystem] ); }

    void uninit( void )
    {
        for ( std::vector<int>::iterator itr=fds.begin(); itr != fds.end(); ++itr )
            close( *itr );
    }

#define BITS_PER_LONG           (sizeof(unsigned long) * 8)
#define NBITS(x)                ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)                  ((x)%BITS_PER_LONG)
#define LONG(x)                 ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

    //
    // Only keep fds that we think are a keyboard, mouse or touchpad/touchscreen
    //
    // Joysticks are handled in /src/SFML/Window/Unix/JoystickImpl.cpp
    //
    bool keep_fd( int fd )
    {
        unsigned long bitmask_ev[NBITS(EV_MAX)];
        unsigned long bitmask_key[NBITS(KEY_MAX)];
        unsigned long bitmask_abs[NBITS(ABS_MAX)];
        unsigned long bitmask_rel[NBITS(REL_MAX)];

        ioctl( fd, EVIOCGBIT( 0, sizeof( bitmask_ev ) ), &bitmask_ev );
        ioctl( fd, EVIOCGBIT( EV_KEY, sizeof( bitmask_key ) ), &bitmask_key );
        ioctl( fd, EVIOCGBIT( EV_ABS, sizeof( bitmask_abs ) ), &bitmask_abs );
        ioctl( fd, EVIOCGBIT( EV_REL, sizeof( bitmask_rel ) ), &bitmask_rel );

        // This is the keyboard test used by SDL
        //
        // the first 32 bits are ESC, numbers and Q to D;  If we have any of those,
        // consider it a keyboard device; do not test for KEY_RESERVED, though
        bool is_keyboard = ( bitmask_key[0] & 0xFFFFFFFE );

        bool is_abs = test_bit( EV_ABS, bitmask_ev )
            && test_bit( ABS_X, bitmask_abs ) && test_bit( ABS_Y, bitmask_abs );

        bool is_rel = test_bit( EV_REL, bitmask_ev )
            && test_bit( REL_X, bitmask_rel ) && test_bit( REL_Y, bitmask_rel );

        bool is_mouse = ( is_abs || is_rel ) && test_bit( BTN_MOUSE, bitmask_key );

        bool is_touch = is_abs && ( test_bit( BTN_TOOL_FINGER, bitmask_key ) || test_bit( BTN_TOUCH, bitmask_key) );

        return is_keyboard || is_mouse || is_touch;
    }

    void init()
    {
        static bool initialized = false;
        if ( initialized )
            return;

        initialized=true;

        for ( int i=0; i<32; i++ )
        {
            std::string name( "/dev/input/event" );
            std::ostringstream ss;
            ss << i;
            name += ss.str();

            int temp_fd = open( name.c_str(), O_RDONLY | O_NONBLOCK );

            if ( temp_fd < 0 )
            {
                if ( errno != ENOENT )
                    sf::err() << "Error opening " << name << ": " << strerror( errno ) << std::endl;

                continue;
            }

            if ( keep_fd( temp_fd ) )
                fds.push_back( temp_fd );
            else
                close( temp_fd );
        }

        atexit( uninit );
    }

    sf::Mouse::Button toMouseButton( int c )
    {
        switch ( c )
        {
            case BTN_LEFT:          return sf::Mouse::Left;
            case BTN_RIGHT:         return sf::Mouse::Right;
            case BTN_MIDDLE:        return sf::Mouse::Middle;
            case BTN_SIDE:          return sf::Mouse::XButton1;
            case BTN_EXTRA:         return sf::Mouse::XButton2;

            default:
                return sf::Mouse::ButtonCount;
        }
    }

    sf::Keyboard::Key toKey( int c )
    {
        switch ( c )
        {
            case KEY_ESC:           return sf::Keyboard::Escape;
            case KEY_1:             return sf::Keyboard::Num1;
            case KEY_2:             return sf::Keyboard::Num2;
            case KEY_3:             return sf::Keyboard::Num3;
            case KEY_4:             return sf::Keyboard::Num4;
            case KEY_5:             return sf::Keyboard::Num5;
            case KEY_6:             return sf::Keyboard::Num6;
            case KEY_7:             return sf::Keyboard::Num7;
            case KEY_8:             return sf::Keyboard::Num8;
            case KEY_9:             return sf::Keyboard::Num9;
            case KEY_0:             return sf::Keyboard::Num0;
            case KEY_MINUS:         return sf::Keyboard::Dash;
            case KEY_EQUAL:         return sf::Keyboard::Equal;
            case KEY_BACKSPACE:     return sf::Keyboard::BackSpace;
            case KEY_TAB:           return sf::Keyboard::Tab;
            case KEY_Q:             return sf::Keyboard::Q;
            case KEY_W:             return sf::Keyboard::W;
            case KEY_E:             return sf::Keyboard::E;
            case KEY_R:             return sf::Keyboard::R;
            case KEY_T:             return sf::Keyboard::T;
            case KEY_Y:             return sf::Keyboard::Y;
            case KEY_U:             return sf::Keyboard::U;
            case KEY_I:             return sf::Keyboard::I;
            case KEY_O:             return sf::Keyboard::O;
            case KEY_P:             return sf::Keyboard::P;
            case KEY_LEFTBRACE:     return sf::Keyboard::LBracket;
            case KEY_RIGHTBRACE:    return sf::Keyboard::RBracket;
            case KEY_KPENTER:
            case KEY_ENTER:         return sf::Keyboard::Return;
            case KEY_LEFTCTRL:      return sf::Keyboard::LControl;
            case KEY_A:             return sf::Keyboard::A;
            case KEY_S:             return sf::Keyboard::S;
            case KEY_D:             return sf::Keyboard::D;
            case KEY_F:             return sf::Keyboard::F;
            case KEY_G:             return sf::Keyboard::G;
            case KEY_H:             return sf::Keyboard::H;
            case KEY_J:             return sf::Keyboard::J;
            case KEY_K:             return sf::Keyboard::K;
            case KEY_L:             return sf::Keyboard::L;
            case KEY_SEMICOLON:     return sf::Keyboard::SemiColon;
            case KEY_APOSTROPHE:    return sf::Keyboard::Quote;
            case KEY_GRAVE:         return sf::Keyboard::Tilde;
            case KEY_LEFTSHIFT:     return sf::Keyboard::LShift;
            case KEY_BACKSLASH:     return sf::Keyboard::BackSlash;
            case KEY_Z:             return sf::Keyboard::Z;
            case KEY_X:             return sf::Keyboard::X;
            case KEY_C:             return sf::Keyboard::C;
            case KEY_V:             return sf::Keyboard::V;
            case KEY_B:             return sf::Keyboard::B;
            case KEY_N:             return sf::Keyboard::N;
            case KEY_M:             return sf::Keyboard::M;
            case KEY_COMMA:         return sf::Keyboard::Comma;
            case KEY_DOT:           return sf::Keyboard::Period;
            case KEY_SLASH:         return sf::Keyboard::Slash;
            case KEY_RIGHTSHIFT:    return sf::Keyboard::RShift;
            case KEY_KPASTERISK:    return sf::Keyboard::Multiply;
            case KEY_LEFTALT:       return sf::Keyboard::LAlt;
            case KEY_SPACE:         return sf::Keyboard::Space;
            case KEY_F1:            return sf::Keyboard::F1;
            case KEY_F2:            return sf::Keyboard::F2;
            case KEY_F3:            return sf::Keyboard::F3;
            case KEY_F4:            return sf::Keyboard::F4;
            case KEY_F5:            return sf::Keyboard::F5;
            case KEY_F6:            return sf::Keyboard::F6;
            case KEY_F7:            return sf::Keyboard::F7;
            case KEY_F8:            return sf::Keyboard::F8;
            case KEY_F9:            return sf::Keyboard::F9;
            case KEY_F10:           return sf::Keyboard::F10;
            case KEY_F11:           return sf::Keyboard::F11;
            case KEY_F12:           return sf::Keyboard::F12;
            case KEY_F13:           return sf::Keyboard::F13;
            case KEY_F14:           return sf::Keyboard::F14;
            case KEY_F15:           return sf::Keyboard::F15;
            case KEY_KP7:           return sf::Keyboard::Numpad7;
            case KEY_KP8:           return sf::Keyboard::Numpad8;
            case KEY_KP9:           return sf::Keyboard::Numpad9;
            case KEY_KPMINUS:       return sf::Keyboard::Subtract;
            case KEY_KP4:           return sf::Keyboard::Numpad4;
            case KEY_KP5:           return sf::Keyboard::Numpad5;
            case KEY_KP6:           return sf::Keyboard::Numpad6;
            case KEY_KPPLUS:        return sf::Keyboard::Add;
            case KEY_KP1:           return sf::Keyboard::Numpad1;
            case KEY_KP2:           return sf::Keyboard::Numpad2;
            case KEY_KP3:           return sf::Keyboard::Numpad3;
            case KEY_KP0:           return sf::Keyboard::Numpad0;
            case KEY_KPDOT:         return sf::Keyboard::Delete;
            case KEY_RIGHTCTRL:     return sf::Keyboard::RControl;
            case KEY_KPSLASH:       return sf::Keyboard::Divide;
            case KEY_RIGHTALT:      return sf::Keyboard::RAlt;
            case KEY_HOME:          return sf::Keyboard::Home;
            case KEY_UP:            return sf::Keyboard::Up;
            case KEY_PAGEUP:        return sf::Keyboard::PageUp;
            case KEY_LEFT:          return sf::Keyboard::Left;
            case KEY_RIGHT:         return sf::Keyboard::Right;
            case KEY_END:           return sf::Keyboard::End;
            case KEY_DOWN:          return sf::Keyboard::Down;
            case KEY_PAGEDOWN:      return sf::Keyboard::PageDown;
            case KEY_INSERT:        return sf::Keyboard::Insert;
            case KEY_DELETE:        return sf::Keyboard::Delete;
            case KEY_PAUSE:         return sf::Keyboard::Pause;
            case KEY_LEFTMETA:      return sf::Keyboard::LSystem;
            case KEY_RIGHTMETA:     return sf::Keyboard::RSystem;

            case KEY_RESERVED:
            case KEY_SYSRQ:
            case KEY_CAPSLOCK:
            case KEY_NUMLOCK:
            case KEY_SCROLLLOCK:
            default:
               return sf::Keyboard::Unknown;
        }
    }

    void pushEvent( sf::Event& ev )
    {
        if ( eventQueue.size() >= MAX_QUEUE )
            eventQueue.pop();

        eventQueue.push( ev );
    }

    TouchSlot& atSlot( int idx )
    {
        if ( idx >= touchSlots.size() )
            touchSlots.resize(idx + 1);
        return touchSlots.at( idx );
    }

    void processSlots()
    {
        for ( std::vector<TouchSlot>::iterator slot=touchSlots.begin(); slot != touchSlots.end(); ++slot )
        {
            sf::Event ev;

            ev.touch.x = slot->pos.x;
            ev.touch.y = slot->pos.y;

            if ( slot->oldId == slot->id )
            {
                ev.type = sf::Event::TouchMoved;
                ev.touch.finger = slot->id;
                pushEvent( ev );
            }
            else
            {
                if ( slot->oldId != -1 )
                {
                    ev.type = sf::Event::TouchEnded;
                    ev.touch.finger = slot->oldId;
                    pushEvent( ev );
                }
                if ( slot->id != -1 )
                {
                    ev.type = sf::Event::TouchBegan;
                    ev.touch.finger = slot->id;
                    pushEvent( ev );
                }

                slot->oldId = slot->id;
            }
        }
    }

    bool eventProcess( sf::Event &ev )
    {
        sf::Lock lock( inpMutex );

        // Ensure that we are initialized
        //
        init();

        // This is for handling the Backspace and DEL text events, which we
        // generate based on keystrokes (and not stdin)
        //
        static int doDeferredText=0;
        if ( doDeferredText )
        {
            ev.type = sf::Event::TextEntered;
            ev.text.unicode = doDeferredText;
            doDeferredText = 0;
            return true;
        }

        // Check all the open file descriptors for the next event
        //
        for ( std::vector<int>::iterator itr=fds.begin(); itr != fds.end(); ++itr )
        {
            struct input_event ie;
            int rd = read(
                *itr,
                &ie,
                sizeof( ie ) );

            while ( rd > 0 )
            {
                if ( ie.type == EV_KEY )
                {
                    sf::Mouse::Button mb = toMouseButton( ie.code );
                    if ( mb != sf::Mouse::ButtonCount )
                    {
                        ev.type = ie.value ? sf::Event::MouseButtonPressed : sf::Event::MouseButtonReleased;
                        ev.mouseButton.button = mb;
                        ev.mouseButton.x = mousePos.x;
                        ev.mouseButton.y = mousePos.y;

                        mouseMap[mb] = ie.value;
                        return true;
                    }
                    else
                    {
                        sf::Keyboard::Key kb = toKey( ie.code );

                        int special = 0;
                        if (( kb == sf::Keyboard::Delete )
                                || ( kb == sf::Keyboard::BackSpace ))
                            special = ( kb == sf::Keyboard::Delete ) ? 127 : 8;

                        if ( ie.value == 2 )
                        {
                            // key repeat events
                            //
                            if ( special )
                            {
                                ev.type = sf::Event::TextEntered;
                                ev.text.unicode = special;
                                return true;
                            }
                        }
                        else if ( kb != sf::Keyboard::Unknown )
                        {
                            // key down and key up events
                            //
                            ev.type = ie.value ? sf::Event::KeyPressed : sf::Event::KeyReleased;
                            ev.key.code = kb;
                            ev.key.alt = altDown();
                            ev.key.control = controlDown();
                            ev.key.shift = shiftDown();
                            ev.key.system = systemDown();

                            keyMap[kb] = ie.value;

                            if ( special && ie.value )
                                doDeferredText = special;

                            return true;
                        }
                    }
                }
                else if ( ie.type == EV_REL )
                {
                    bool posChange = false;
                    switch ( ie.code )
                    {
                    case REL_X:
                        mousePos.x += ie.value;
                        posChange = true;
                        break;

                    case REL_Y:
                        mousePos.y += ie.value;
                        posChange = true;
                        break;

                    case REL_WHEEL:
                        ev.type = sf::Event::MouseWheelMoved;
                        ev.mouseWheel.delta = ie.value;
                        ev.mouseWheel.x = mousePos.x;
                        ev.mouseWheel.y = mousePos.y;
                        return true;
                    }

                    if ( posChange )
                    {
                        ev.type = sf::Event::MouseMoved;
                        ev.mouseMove.x = mousePos.x;
                        ev.mouseMove.y = mousePos.y;
                        return true;
                    }
                }
                else if ( ie.type == EV_ABS )
                {
                    switch ( ie.code )
                    {
                    case ABS_MT_SLOT:
                        currentSlot = ie.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_TRACKING_ID:
                        atSlot(currentSlot).id = ie.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_POSITION_X:
                        atSlot(currentSlot).pos.x = ie.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_POSITION_Y:
                        atSlot(currentSlot).pos.y = ie.value;
                        touchFd = *itr;
                        break;
                    }
                }
                else if ( ie.type == EV_SYN && ie.code == SYN_REPORT &&
                          *itr == touchFd)
                {
                    // This pushes events directly to the queue, because it
                    // can generate more than one event.
                    processSlots();
                }

                rd = read( *itr, &ie, sizeof( ie ) );
            }

            if (( rd < 0 ) && ( errno != EAGAIN ))
                sf::err() << " Error: " << strerror( errno ) << std::endl;
        }

        // Finally check if there is a Text event on stdin
        //
        // We only clear the ICANON flag for the time of reading

        newt.c_lflag &= ~ICANON;
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        unsigned char c=0;

        fd_set rdfs;
        FD_ZERO( &rdfs );
        FD_SET( STDIN_FILENO, &rdfs );
        int sel = select( STDIN_FILENO+1, &rdfs, NULL, NULL, &tv );

        if ( sel > 0 && FD_ISSET( STDIN_FILENO, &rdfs ) )
            read( STDIN_FILENO, &c, 1 );

        if (( c == 127 ) || ( c == 8 ))  // Suppress 127 (DEL) to 8 (BACKSPACE)
            c = 0;
        else if ( c == 27 )  // ESC
        {
            // Suppress ANSI escape sequences
            //
            FD_ZERO( &rdfs );
            FD_SET( STDIN_FILENO, &rdfs );
            sel = select( STDIN_FILENO+1, &rdfs, NULL, NULL, &tv );
            if ( sel > 0 && FD_ISSET( STDIN_FILENO, &rdfs ) )
            {
                unsigned char buff[16];
                int rd = read( STDIN_FILENO, buff, 16 );
                c = 0;
            }
        }

        newt.c_lflag |= ICANON;
        tcsetattr( STDIN_FILENO, TCSANOW, &newt );

        if ( c > 0 )
        {
            // TODO: Proper unicode handling
            ev.type = sf::Event::TextEntered;
            ev.text.unicode = c;
            return true;
        }

        // No events available
        //
        return false;
    }

    // assumes inpMutex is locked
    void update()
    {
        sf::Event ev;
        while ( eventProcess( ev ) )
        {
            pushEvent( ev );
        }
    }
};

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
bool InputImpl::isKeyPressed(Keyboard::Key key)
{
    Lock lock( inpMutex );
    if (( key < 0 ) || ( key >= keyMap.size() ))
        return false;

    update();
    return keyMap[key];
}


////////////////////////////////////////////////////////////
void InputImpl::setVirtualKeyboardVisible(bool /*visible*/)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool InputImpl::isMouseButtonPressed(Mouse::Button button)
{
    Lock lock( inpMutex );
    if (( button < 0 ) || ( button >= mouseMap.size() ))
        return false;

    update();
    return mouseMap[button];
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getMousePosition()
{
    Lock lock( inpMutex );
    return mousePos;
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getMousePosition(const Window& relativeTo)
{
    return getMousePosition();
}


////////////////////////////////////////////////////////////
void InputImpl::setMousePosition(const Vector2i& position)
{
    Lock lock( inpMutex );
    mousePos = position;
}


////////////////////////////////////////////////////////////
void InputImpl::setMousePosition(const Vector2i& position, const Window& relativeTo)
{
    setMousePosition( position );
}


////////////////////////////////////////////////////////////
bool InputImpl::isTouchDown(unsigned int finger)
{
    for ( std::vector<TouchSlot>::iterator slot=touchSlots.begin(); slot != touchSlots.end(); ++slot )
    {
        if ( slot->id == finger )
            return true;
    }

    return false;
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getTouchPosition(unsigned int finger)
{
    for ( std::vector<TouchSlot>::iterator slot=touchSlots.begin(); slot != touchSlots.end(); ++slot )
    {
        if ( slot->id == finger )
            return slot->pos;
    }

    return Vector2i();
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getTouchPosition(unsigned int finger, const Window& /*relativeTo*/)
{
    return getTouchPosition( finger );
}

////////////////////////////////////////////////////////////
bool InputImpl::checkEvent( sf::Event &ev )
{
    Lock lock( inpMutex );
    if ( !eventQueue.empty() )
    {
        ev = eventQueue.front();
        eventQueue.pop();

        return true;
    }

    if ( eventProcess( ev ) )
    {
        return true;
    }
    else
    {
        // In the case of multitouch, eventProcess() could have returned false
        // but added events directly to the queue.  (This is ugly, but I'm not
        // sure of a good way to handle generating multiple events at once.)
        if ( !eventQueue.empty() )
        {
            ev = eventQueue.front();
            eventQueue.pop();

            return true;
        }
    }

    return false;
}

void InputImpl::setTerminalConfig()
{
    sf::Lock lock( inpMutex );
    init();

    tcgetattr(STDIN_FILENO, &newt);          // get current terminal config
    oldt = newt;                             // create a backup
    newt.c_lflag &= ~ECHO;                   // disable console feedback
    newt.c_lflag |= ICANON;                  // disable noncanonical mode
    newt.c_iflag |= IGNCR;                   // ignore carriage return
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // set our new config
    tcflush(STDIN_FILENO, TCIFLUSH);         // flush the buffer
}

void InputImpl::restoreTerminalConfig()
{
    sf::Lock lock( inpMutex );
    init();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore terminal config
    tcflush(STDIN_FILENO, TCIFLUSH);         // flush the buffer
}

} // namespace priv

} // namespace sf
