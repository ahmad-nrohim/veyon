/*
 * InternetAccessPlugin.h - declaration of InternetAccessPlugin class
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "Feature.h"
#include "FeatureProviderInterface.h"
#include "PluginInterface.h"

class InternetAccessPlugin : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.InternetAccess")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	explicit InternetAccessPlugin( QObject* parent = nullptr );
	~InternetAccessPlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("d3a58e21-9954-46c8-97ef-1981bdf01362") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("InternetAccess");
	}

	QString description() const override
	{
		return tr( "Block or unblock internet access on client computers" );
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

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

private:
	enum class FeatureCommand
	{
		BlockInternet,
		UnblockInternet
	};

	const Feature m_internetAccessFeature;
	const FeatureList m_features;

};
