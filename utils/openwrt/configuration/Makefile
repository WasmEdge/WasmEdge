include $(TOPDIR)/rules.mk

PKG_NAME:=WasmEdge
PKG_VERSION:=0.14.0
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)
WASMEDGE_SOVERSION:=0.1.0
WASMEDGE_SONAME:=0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
    SECTION:=base
    CATEGORY:=Runtime
    TITLE:=$(PKG_NAME) -prints a $(PKG_NAME) message
    DEPENDS:=+libpthread +libstdcpp
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
	$(CP) -rp $(PKG_INSTALL_DIR)/usr/lib/libwasmedge.so $(1)/usr/lib/
	$(CP) -rp $(PKG_INSTALL_DIR)/usr/lib/libwasmedge.so.$(WASMEDGE_SONAME) $(1)/usr/lib/
	$(CP) -rp $(PKG_INSTALL_DIR)/usr/lib/libwasmedge.so.$(WASMEDGE_SOVERSION) $(1)/usr/lib/
endef

CMAKE_OPTIONS += \
        -DWASMEDGE_USE_LLVM=OFF \

$(eval $(call BuildPackage,$(PKG_NAME)))
