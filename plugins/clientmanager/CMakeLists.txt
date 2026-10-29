include(BuildVeyonPlugin)

build_veyon_plugin(clientmanager
	NAME ClientManager
	SOURCES
	AppManagerDialog.cpp AppManagerDialog.h
	HardwareInfoDialog.cpp HardwareInfoDialog.h
	NetworkDiscoveryDialog.cpp NetworkDiscoveryDialog.h
	ClientManagerPlugin.cpp ClientManagerPlugin.h
	clientmanager.qrc)
