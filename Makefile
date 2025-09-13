include $(TOPDIR)/rules.mk

PKG_NAME:=airplay2-lite
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/yourusername/airplay2-lite.git
PKG_SOURCE_VERSION:=HEAD
PKG_MIRROR_HASH:=skip

PKG_MAINTAINER:=Your Name <your.email@example.com>
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=libopenssl libavahi-client libavahi-common libdaemon

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/airplay2-lite
  SECTION:=multimedia
  CATEGORY:=Multimedia
  TITLE:=Lightweight AirPlay 2 Server
  DEPENDS:=+libopenssl +libavahi-client +libavahi-common +libdaemon +alsa-lib
  URL:=https://github.com/yourusername/airplay2-lite
endef

define Package/airplay2-lite/description
  A lightweight AirPlay 2 server implementation optimized for OpenWRT
  with minimal resource usage for low-capacity systems.
endef

CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DWITH_AVAHI=ON \
	-DWITH_ALSA=ON \
	-DWITH_OPENSSL=ON \
	-DWITH_SYSTEMD=OFF \
	-DCMAKE_C_FLAGS="-Os -ffunction-sections -fdata-sections -mips32r2 -mtune=24kc" \
	-DCMAKE_CXX_FLAGS="-Os -ffunction-sections -fdata-sections -mips32r2 -mtune=24kc" \
	-DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections -Wl,--strip-all"

define Package/airplay2-lite/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/airplay2-lite $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/airplay2-lite.init $(1)/etc/init.d/airplay2-lite
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) ./files/airplay2-lite.config $(1)/etc/config/airplay2-lite
endef

$(eval $(call BuildPackage,airplay2-lite))
