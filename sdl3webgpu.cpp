/**
 * This is an extension of SDL3 for WebGPU, abstracting away the details of
 * OS-specific operations.
 * 
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * Most of this code comes from the wgpu-native triangle example:
 *   https://github.com/gfx-rs/wgpu-native/blob/master/examples/triangle/main.c
 * 
 * MIT License
 * Copyright (c) 2022-2024 Elie Michel and the wgpu-native authors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "sdl3webgpu.hpp"

#include <webgpu/webgpu_cpp.h>

#if defined(SDL_PLATFORM_MACOS)
#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#elif defined(SDL_PLATFORM_IOS)
#include <Foundation/Foundation.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <UIKit/UIKit.h>
#elif defined(SDL_PLATFORM_WIN32)
#include <windows.h>
#endif

#include <SDL3/SDL.h>

wgpu::Surface SDL_GetWGPUSurface(wgpu::Instance instance, SDL_Window *window) {
	SDL_PropertiesID props = SDL_GetWindowProperties(window);

#if defined(SDL_PLATFORM_MACOS)
	{
		id metal_layer = nil;
		NSWindow *ns_window = (__bridge NSWindow *)SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
		if (!ns_window)
			return nullptr;
		[ns_window.contentView setWantsLayer:YES];
		metal_layer = [CAMetalLayer layer];
		[ns_window.contentView setLayer:metal_layer];

		wgpu::SurfaceSourceMetalLayer fromMetalLayer;
		fromMetalLayer.sType = wgpu::SType::SurfaceSourceMetalLayer;
		fromMetalLayer.nextInChain = nullptr;
		fromMetalLayer.layer = metal_layer;

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromMetalLayer;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	}
#elif defined(SDL_PLATFORM_IOS)
	{
		UIWindow *ui_window = (__bridge UIWindow *)SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, NULL);
		if (!ui_window)
			return nullptr;

		UIView *ui_view = ui_window.rootViewController.view;
		CAMetalLayer *metal_layer = [CAMetalLayer new];
		metal_layer.opaque = true;
		metal_layer.frame = ui_view.frame;
		metal_layer.drawableSize = ui_view.frame.size;

		[ui_view.layer addSublayer:metal_layer];

		wgpu::SurfaceSourceMetalLayer fromMetalLayer;
		fromMetalLayer.sType = wgpu::SType::SurfaceSourceMetalLayer;
		fromMetalLayer.nextInChain = nullptr;
		fromMetalLayer.layer = metal_layer;

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromMetalLayer;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	}
#elif defined(SDL_PLATFORM_LINUX)
	if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
		void *x11_display = SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
		uint64_t x11_window =
			SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
		if (!x11_display || !x11_window)
			return nullptr;

		wgpu::SurfaceSourceXlibWindow fromXlibWindow;
		fromXlibWindow.sType = wgpu::SType::SurfaceSourceXlibWindow;
		fromXlibWindow.nextInChain = nullptr;
		fromXlibWindow.display = x11_display;
		fromXlibWindow.window = static_cast<uint32_t>(x11_window);

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromXlibWindow;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	} else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
		void *wayland_display = SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
		void *wayland_surface = SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
		if (!wayland_display || !wayland_surface)
			return nullptr;

		wgpu::SurfaceSourceWaylandSurface fromWaylandSurface;
		fromWaylandSurface.sType = wgpu::SType::SurfaceSourceWaylandSurface;
		fromWaylandSurface.nextInChain = nullptr;
		fromWaylandSurface.display = wayland_display;
		fromWaylandSurface.surface = wayland_surface;

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromWaylandSurface;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	}
	return nullptr;
#elif defined(SDL_PLATFORM_WIN32)
	{
		HWND hwnd = (HWND)SDL_GetPointerProperty(
			props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
		if (!hwnd)
			return nullptr;
		HINSTANCE hinstance = GetModuleHandle(NULL);

		wgpu::SurfaceSourceWindowsHWND fromWindowsHWND;
		fromWindowsHWND.sType = wgpu::SType::SurfaceSourceWindowsHWND;
		fromWindowsHWND.nextInChain = nullptr;
		fromWindowsHWND.hinstance = hinstance;
		fromWindowsHWND.hwnd = hwnd;

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromWindowsHWND;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	}
#elif defined(__EMSCRIPTEN__)
	{
		// Emscripten targets a native selector via standard string descriptors
		// in newer Dawn updates
		wgpu::SurfaceSourceCanvasHTMLSelector fromCanvasHTMLSelector;
		fromCanvasHTMLSelector.sType =
			wgpu::SType::SurfaceSourceCanvasHTMLSelector;
		fromCanvasHTMLSelector.nextInChain = nullptr;
		fromCanvasHTMLSelector.selector = "canvas";

		wgpu::SurfaceDescriptor surfaceDescriptor;
		surfaceDescriptor.nextInChain = &fromCanvasHTMLSelector;
		surfaceDescriptor.label = {};

		return instance.CreateSurface(&surfaceDescriptor);
	}
#else
#error "Unsupported WGPU_TARGET"
#endif
}