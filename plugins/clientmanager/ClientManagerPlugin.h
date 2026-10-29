/*
 * ClientManagerPlugin.h - declaration of ClientManagerPlugin class
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "PluginInterface.h"

#include <QPointer>

class AppManagerDialog;
class HardwareInfoDialog;

class ClientManagerPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.ClientManager")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit ClientManagerPlugin( QObject* parent = nullptr );
	~ClientManagerPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("e849ad32-0941-45da-9c87-09f123c89b41") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("ClientManager");
	}

	QString description() const override
	{
		return tr( "Application manager, hardware inspector, and network discovery for Veyon" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Ahmad Nurohim");
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						 const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	const Feature m_appManagerFeature;
	const Feature m_hardwareInfoFeature;
	const Feature m_networkDiscoveryFeature;
	const FeatureList m_features;

	QPointer<AppManagerDialog> m_activeAppDialog;
	QPointer<HardwareInfoDialog> m_activeHardwareDialog;
};
