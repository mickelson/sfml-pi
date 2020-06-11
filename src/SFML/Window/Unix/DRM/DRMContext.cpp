////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
//
// Linux DRM implementation
// Copyright (C) 2020 Andrew Mickelson
// Copyright (C) 2013 Jonathan De Wachter (dewachter.jonathan@gmail.com)
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
#include <SFML/Window/Unix/DRM/DRMContext.hpp>
#include <SFML/Window/Unix/DRM/WindowImplDRM.hpp>
#include <SFML/Window/Unix/DRM/drm-common.h>
#include <SFML/OpenGL.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Sleep.hpp>

#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <errno.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gbm.h>

#ifndef EGL_EXT_platform_base
#define EGL_EXT_platform_base 1
typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT (EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
#endif
#endif // EGL_EXT_platform_base

namespace
{
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;
    PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC eglCreatePlatformWindowSurfaceEXT = NULL;
    static bool initialized = false;
    static struct drm my_drm;
    static struct gbm_device *my_gbm_device = NULL;
    static int context_count = 0;
    static EGLDisplay display = EGL_NO_DISPLAY;

    void cleanup()
    {
        if ( !initialized )
            return;

        drmModeSetCrtc( my_drm.fd,
            my_drm.original_crtc->crtc_id,
            my_drm.original_crtc->buffer_id,
            my_drm.original_crtc->x,
            my_drm.original_crtc->y,
            &my_drm.connector_id,
            1,
            &my_drm.original_crtc->mode );

        gbm_device_destroy( my_gbm_device );
        my_gbm_device = NULL;

        close( my_drm.fd );

        display = EGL_NO_DISPLAY;
        initialized=false;
    }

    void check_init()
    {
        if ( initialized )
            return;

        // device: Use environment variable "SFML_DRM_DEVICE" (or NULL if not set)
        char *device_str = getenv( "SFML_DRM_DEVICE" );
        if ( device_str && !*device_str )
            device_str=NULL;

        // mode: Use environment variable "SFML_DRM_MODE" (or NULL if not set)
        char *mode_str = getenv( "SFML_DRM_MODE" );
        
		// refresh: Use environment variable "SFML_DRM_REFRESH" (or 0 if not set)
		// use in combination with mode to request specific refresh rate for the mode
		// if multiple refresh rates for same mode might be supported
		int vrefresh = 0;
		char *refresh_str = getenv( "SFML_DRM_REFRESH" );

		if (refresh_str)
			vrefresh = atoi(refresh_str);

        if ( init_drm( &my_drm,
            device_str,          // device
            mode_str,            // requested mode
            vrefresh ) < 0 )     // vrefresh
        {
            sf::err() << "Error initializing drm" << std::endl;
            return;
        }

        my_gbm_device = gbm_create_device( my_drm.fd );

        std::atexit( cleanup );
        initialized = true;
    }

    bool has_ext(const char *extension_list, const char *ext)
    {
        const char *ptr = extension_list;
        int len = strlen(ext);

        if (ptr == NULL || *ptr == '\0')
                return false;

        while (true) {
                ptr = strstr(ptr, ext);
                if (!ptr)
                        return false;

                if (ptr[len] == ' ' || ptr[len] == '\0')
                        return true;

                ptr += len;
        }
    }


    EGLDisplay getInitializedDisplay()
    {
        check_init();

        if (display == EGL_NO_DISPLAY)
        {
            const char *egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

            if (has_ext(egl_exts_client, "EGL_EXT_platform_base"))
            {
                eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
                    "eglGetPlatformDisplayEXT");

                eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)eglGetProcAddress(
                    "eglCreatePlatformWindowSurfaceEXT");

                if ((!has_ext(egl_exts_client, "EGL_MESA_platform_gbm"))
                       && (!has_ext(egl_exts_client, "EGL_KHR_platform_gbm")))
                {
                    sf::err() << "Couldn't find required EGL extension: EGL_MESA_platform_gbm or EGL_KHR_platform_gbm" << std::endl;
                    abort(); // we can't do it without this extension
                }
            }
            else
                sf::err() << "EGL_EXT_platform_base extension not found!" << std::endl;

            if (eglGetPlatformDisplayEXT)
                // EGL_PLATFORM_GBM_KHR and EGL_PLATFORM_GBM_MESA are equal
                display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, my_gbm_device, NULL);
            else
                display = eglGetDisplay( (EGLNativeDisplayType)my_gbm_device );

            eglCheck( display );

            EGLint major, minor;
            eglCheck(eglInitialize(display, &major, &minor));

#if defined(SFML_OPENGL_ES)
            if ( !eglBindAPI( EGL_OPENGL_ES_API ) )
            {
                sf::err() << "failed to bind api EGL_OPENGL_ES_API" << std::endl;
            }
#else
            if ( !eglBindAPI( EGL_OPENGL_API ) )
            {
                sf::err() << "failed to bind api EGL_OPENGL_API" << std::endl;
            }
#endif
        }

        return display;
    }

    static void page_flip_handler(int fd, unsigned int frame,
        unsigned int sec, unsigned int usec, void *data )
    {
        // suppress unused param warning
        (void)fd, (void)frame, (void)sec, (void)usec;

        int *waiting_for_flip = (int *)data;
        *waiting_for_flip = 0;
    }
}


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared) :
m_display (EGL_NO_DISPLAY),
m_context (EGL_NO_CONTEXT),
m_surface (EGL_NO_SURFACE),
m_config  (NULL),
m_last_bo (NULL),
m_gbm_surface (NULL),
m_width   (0),
m_height  (0),
m_shown   (false),
m_scanout (false)
{
    context_count++;

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the default video settings
    m_config = getBestConfig(m_display, VideoMode::getDesktopMode().bitsPerPixel, ContextSettings());
    updateSettings();

    // Create EGL context
    createContext(shared);

    if ( shared )
        createSurface( shared->m_width, shared->m_height, VideoMode::getDesktopMode().bitsPerPixel, false );
    else // create a surface to force the GL to initialize (seems to be required for glGetString() etc
        createSurface( 1, 1, VideoMode::getDesktopMode().bitsPerPixel, false );
}


////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared, const ContextSettings& settings, const WindowImpl* owner, unsigned int bitsPerPixel) :
m_display (EGL_NO_DISPLAY),
m_context (EGL_NO_CONTEXT),
m_surface (EGL_NO_SURFACE),
m_config  (NULL),
m_last_bo (NULL),
m_gbm_surface (NULL),
m_width   (0),
m_height  (0),
m_shown   (false),
m_scanout (false)
{
    context_count++;

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the requested video settings
    m_config = getBestConfig(m_display, bitsPerPixel, settings);
    updateSettings();

    // Create EGL context
    createContext(shared);

    if ( owner )
    {
        Vector2u s = owner->getSize();
        createSurface( s.x, s.y, bitsPerPixel, true );
    }
}


////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared, const ContextSettings& settings, unsigned int width, unsigned int height) :
m_display (EGL_NO_DISPLAY),
m_context (EGL_NO_CONTEXT),
m_surface (EGL_NO_SURFACE),
m_config  (NULL),
m_last_bo (NULL),
m_gbm_surface (NULL),
m_width   (0),
m_height  (0),
m_shown   (false),
m_scanout (false)
{
    context_count++;

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the requested video settings
    m_config = getBestConfig(m_display, VideoMode::getDesktopMode().bitsPerPixel, settings);
    updateSettings();

    // Create EGL context
    createContext(shared);
    createSurface( width, height, VideoMode::getDesktopMode().bitsPerPixel, false );
}


////////////////////////////////////////////////////////////
DRMContext::~DRMContext()
{
    // Deactivate the current context
    EGLContext currentContext = eglCheck(eglGetCurrentContext());

    if (currentContext == m_context)
    {
        eglCheck(eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    }

    // Destroy context
    if (m_context != EGL_NO_CONTEXT)
    {
        eglCheck(eglDestroyContext(m_display, m_context));
        m_context = EGL_NO_CONTEXT;
    }

    // Destroy surface
    if (m_surface != EGL_NO_SURFACE)
    {
        eglCheck(eglDestroySurface(m_display, m_surface));
        m_surface = EGL_NO_SURFACE;
    }

    if ( m_last_bo )
        gbm_surface_release_buffer( m_gbm_surface, m_last_bo );

    if ( m_gbm_surface )
        gbm_surface_destroy( m_gbm_surface );

    context_count--;
    if ( context_count == 0 )
        cleanup();
}


////////////////////////////////////////////////////////////
bool DRMContext::makeCurrent(bool current)
{
    if (current)
    {
        return ( m_surface != EGL_NO_SURFACE && eglMakeCurrent(m_display, m_surface, m_surface, m_context) );
    }

    return (m_surface != EGL_NO_SURFACE && eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) );
}


////////////////////////////////////////////////////////////
void DRMContext::display()
{
    if (m_surface == EGL_NO_SURFACE)
        return;

    eglCheck( eglSwapBuffers( m_display, m_surface ) );

    if ( !m_scanout )
        return;

    //
    // handle display of buffer to the screen
    //
    struct drm_fb *fb = NULL;

    //
    // If first time, need to first call drmModeSetCrtc()
    //
    if ( !m_shown )
    {
        // This call must be preceeded by a single call to eglSwapBuffers()
        m_last_bo = gbm_surface_lock_front_buffer( m_gbm_surface );
        if (!m_last_bo)
            return;

        fb = drm_fb_get_from_bo( m_last_bo );
        if (!fb)
        {
            err() << "Could not get FB from buffer object" << std::endl;
            return;
        }

        if ( drmModeSetCrtc( my_drm.fd, my_drm.crtc_id, fb->fb_id, 0, 0,
            &my_drm.connector_id, 1, my_drm.mode ) )
        {
            err() << "Failed to set mode: " << strerror(errno) << std::endl;
            abort();
        }

        eglCheck( eglSwapBuffers( m_display, m_surface ) );

        m_shown = true;
    }

    //
    // Do page flip
    //

    // This call must be preceeded by a single call to eglSwapBuffers()
    struct gbm_bo *bo = gbm_surface_lock_front_buffer( m_gbm_surface );
    if (!bo)
        return;

    fb = drm_fb_get_from_bo( bo );
    if (!fb)
    {
        err() << "Failed to get FB from buffer object" << std::endl;
        return;
    }

    int waiting_for_flip = 1;
    fd_set fds;
    drmEventContext evctx;
    evctx.version = 2;
    evctx.page_flip_handler = page_flip_handler;

    int ret = drmModePageFlip( my_drm.fd, my_drm.crtc_id, fb->fb_id,
        DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip );

    if ( ret )
        return;

    while ( waiting_for_flip )
    {
        FD_ZERO( &fds );
        FD_SET( 0, &fds );
        FD_SET( my_drm.fd, &fds );

        ret = select( my_drm.fd + 1, &fds, NULL, NULL, NULL );
        if ( ret < 0 )
        {
            // select err
            err() << "Error on select() after drm page flip" << std::endl;
            return;
        }
        else if ( ret == 0 )
        {
            // select timeout
            return;
        }
        else if ( FD_ISSET( 0, &fds ) )
        {
            // user interrupted
            return;
        }
        drmHandleEvent( my_drm.fd, &evctx );
    }

    if ( m_last_bo )
        gbm_surface_release_buffer( m_gbm_surface, m_last_bo );

    m_last_bo = bo;
}


////////////////////////////////////////////////////////////
void DRMContext::setVerticalSyncEnabled(bool enabled)
{
    eglCheck(eglSwapInterval(m_display, enabled ? 1 : 0));
}


////////////////////////////////////////////////////////////
void DRMContext::createContext(DRMContext* shared)
{
    const EGLint contextVersion[] = {
        EGL_CONTEXT_CLIENT_VERSION, 1,
        EGL_NONE
    };

    EGLContext toShared;

    if (shared)
        toShared = shared->m_context;
    else
        toShared = EGL_NO_CONTEXT;

    if (toShared != EGL_NO_CONTEXT)
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // Create EGL context
    m_context = eglCheck(eglCreateContext(m_display, m_config, toShared, contextVersion));
    if ( m_context == EGL_NO_CONTEXT )
        err() << "Failed to create EGL context" << std::endl;
}


////////////////////////////////////////////////////////////
void DRMContext::createSurface( int width, int height, int bpp, bool scanout )
{
    uint32_t flags = GBM_BO_USE_RENDERING;

    m_scanout = scanout;
    if ( m_scanout )
        flags |= GBM_BO_USE_SCANOUT;

    m_gbm_surface = gbm_surface_create(
        my_gbm_device,
        width,
        height,
        GBM_FORMAT_ARGB8888,
        flags );

    if ( !m_gbm_surface )
    {
        err() << "Failed to create gbm surface." << std::endl;
        return;
    }

    m_width = width;
    m_height = height;

    if (eglCreatePlatformWindowSurfaceEXT)
    {
        m_surface = eglCheck(eglCreatePlatformWindowSurfaceEXT(m_display, m_config, (void *)m_gbm_surface, NULL));
    }
    else
    {
        m_surface = eglCheck(eglCreateWindowSurface(m_display, m_config, (EGLNativeWindowType)m_gbm_surface, NULL));
    }

    if ( m_surface == EGL_NO_SURFACE )
    {
        err() << "Failed to create EGL Surface" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void DRMContext::destroySurface()
{
    eglCheck(eglDestroySurface(m_display, m_surface));
    m_surface = EGL_NO_SURFACE;

    gbm_surface_destroy( m_gbm_surface );
    m_gbm_surface = NULL;

    // Ensure that this context is no longer active since our surface is now destroyed
    setActive(false);
}


////////////////////////////////////////////////////////////
EGLConfig DRMContext::getBestConfig(EGLDisplay display, unsigned int bitsPerPixel, const ContextSettings& settings)
{
    // Set our video settings constraint
    const EGLint attributes[] = {
        EGL_BUFFER_SIZE, bitsPerPixel,
        EGL_DEPTH_SIZE, settings.depthBits,
        EGL_STENCIL_SIZE, settings.stencilBits,
        EGL_SAMPLE_BUFFERS, settings.antialiasingLevel,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,

        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#if defined(SFML_OPENGL_ES)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_NONE
    };

    EGLint configCount;
    EGLConfig configs[1];

    // Ask EGL for the best config matching our video settings
    eglCheck(eglChooseConfig(display, attributes, configs, 1, &configCount));

    return configs[0];
}


////////////////////////////////////////////////////////////
void DRMContext::updateSettings()
{
    EGLint tmp;

    // Update the internal context settings with the current config
    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_DEPTH_SIZE, &tmp));
    m_settings.depthBits = tmp;

    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_STENCIL_SIZE, &tmp));
    m_settings.stencilBits = tmp;

    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_SAMPLES, &tmp));
    m_settings.antialiasingLevel = tmp;

    m_settings.majorVersion = 1;
    m_settings.minorVersion = 1;
    m_settings.attributeFlags = ContextSettings::Default;
}

////////////////////////////////////////////////////////////
GlFunctionPointer DRMContext::getFunction(const char* name)
{
    return reinterpret_cast<GlFunctionPointer>(eglGetProcAddress(name));
}

////////////////////////////////////////////////////////////
struct drm *DRMContext::get_drm()
{
    check_init();
    return &my_drm;
}

} // namespace priv

} // namespace sf
