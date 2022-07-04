include $(TOPDIR)/rules.mk
 
PKG_NAME:=WasmEdge
PKG_VERSION:=0.10.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
    SECTION:=base
    CATEGORY:=Runtime
    TITLE:=$(PKG_NAME) -prints a $(PKG_NAME) message
    DEPENDS:=+libpthread +libubox +libstdcpp +boost +libxml2
endef

define Package/$(PKG_NAME)/description
	WasmEdge is a lightweight, high-performance, and extensible 
	WebAssembly runtime. It is the fastest Wasm VM today. 
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	mkdir -p $(1)/usr/include/wasmedge
	$(INSTALL_DIR) $(1)/usr/bin $(1)/usr/lib $(1)/usr/include/wasmedge
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/wasmedge $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/wasmedge/* $(1)/usr/include/wasmedge/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libwasmedge_c.so $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/wasmedge/libwasmedgePluginWasmEdgeProcess.so $(1)/usr/lib/
endef

CMAKE_BINARY_DIR = $(PKG_BUILD_DIR)/build
CMAKE_OPTIONS += \
        -DWASMEDGE_BUILD_AOT_RUNTIME=OFF \
        -DOUTPUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/build" \


$(eval $(call BuildPackage,$(PKG_NAME)))

