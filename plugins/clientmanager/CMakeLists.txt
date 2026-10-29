include(BuildVeyonPlugin)

build_veyon_plugin(clientmanager
	NAME ClientManager
	SOURCES
	AppManagerDialog.cpp AppManagerDialog.h
	HardwareInfoDialog.cpp HardwareInfoDialog.h
	NetworkDiscoveryDialog.cpp NetworkDiscoveryDialog.h
	ClientManagerPlugin.cpp ClientManagerPlugin.h
	clientmanager.qrc)

target_link_libraries(clientmanager PRIVATE Qt6::Network)
if(VEYON_BUILD_WINDOWS)
	target_link_libraries(clientmanager PRIVATE psapi)
endif()
