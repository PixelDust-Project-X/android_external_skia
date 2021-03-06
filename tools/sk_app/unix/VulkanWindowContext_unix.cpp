
/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "vk/GrVkVulkan.h"

#include "vk/GrVkInterface.h"
#include "vk/GrVkUtil.h"

#include "vk/VkTestUtils.h"

#include "WindowContextFactory_unix.h"
#include "../VulkanWindowContext.h"

#include <X11/Xlib-xcb.h>

namespace sk_app {

namespace window_context_factory {

WindowContext* NewVulkanForXlib(const XlibWindowInfo& info, const DisplayParams& displayParams) {
    PFN_vkGetInstanceProcAddr instProc;
    PFN_vkGetDeviceProcAddr devProc;
    if (!sk_gpu_test::LoadVkLibraryAndGetProcAddrFuncs(&instProc, &devProc)) {
        return nullptr;
    }

    auto createVkSurface = [&info, instProc](VkInstance instance) -> VkSurfaceKHR {
        static PFN_vkCreateXcbSurfaceKHR createXcbSurfaceKHR = nullptr;
        if (!createXcbSurfaceKHR) {
            createXcbSurfaceKHR =
                    (PFN_vkCreateXcbSurfaceKHR) instProc(instance, "vkCreateXcbSurfaceKHR");
        }

        VkSurfaceKHR surface;

        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo;
        memset(&surfaceCreateInfo, 0, sizeof(VkXcbSurfaceCreateInfoKHR));
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = nullptr;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.connection = XGetXCBConnection(info.fDisplay);
        surfaceCreateInfo.window = info.fWindow;

        VkResult res = createXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        if (VK_SUCCESS != res) {
            return VK_NULL_HANDLE;
        }

        return surface;
    };

    auto canPresent = [&info, instProc](VkInstance instance, VkPhysicalDevice physDev,
                              uint32_t queueFamilyIndex) {
        static PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR
                                                getPhysicalDeviceXcbPresentationSupportKHR = nullptr;
        if (!getPhysicalDeviceXcbPresentationSupportKHR) {
            getPhysicalDeviceXcbPresentationSupportKHR =
                (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)
                    instProc(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
        }


        Display* display = info.fDisplay;
        VisualID visualID = info.fVisualInfo->visualid;
        VkBool32 check = getPhysicalDeviceXcbPresentationSupportKHR(physDev,
                                                                    queueFamilyIndex,
                                                                    XGetXCBConnection(display),
                                                                    visualID);
        return (VK_FALSE != check);
    };
    WindowContext* context = new VulkanWindowContext(displayParams, createVkSurface, canPresent,
                                                     instProc, devProc);
    if (!context->isValid()) {
        delete context;
        return nullptr;
    }
    return context;
}

}  // namespace VulkanWindowContextFactory

}  // namespace sk_app
