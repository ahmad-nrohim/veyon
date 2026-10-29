/*
 * InternetAccessPlugin.cpp - implementation of InternetAccessPlugin class
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "InternetAccessPlugin.h"
#include "ComputerControlInterface.h"
#include "FeatureMessage.h"
#include "VeyonServerInterface.h"

#ifdef Q_OS_WIN
#include <QProcess>

static void setInternetBlocked( bool blocked )
{
	QProcess::execute( QStringLiteral("netsh"), {
		QStringLiteral("advfirewall"), QStringLiteral("firewall"),
		QStringLiteral("delete"), QStringLiteral("rule"),
		QStringLiteral("name=VeyonInternetBlock")
	} );

	if( blocked )
	{
		const QString publicRanges = QStringLiteral(
			"0.0.0.0-9.255.255.255,"
			"11.0.0.0-172.15.255.255,"
			"172.32.0.0-192.167.255.255,"
			"192.169.0.0-255.255.255.255"
		);

		QProcess::execute( QStringLiteral("netsh"), {
			QStringLiteral("advfirewall"), QStringLiteral("firewall"),
			QStringLiteral("add"), QStringLiteral("rule"),
			QStringLiteral("name=VeyonInternetBlock"),
			QStringLiteral("dir=out"), QStringLiteral("action=block"),
			QStringLiteral("protocol=any"),
			QStringLiteral("remoteip=%1").arg( publicRanges )
		} );
	}
}
#else
static void setInternetBlocked( bool blocked )
{
	Q_UNUSED(blocked);
}
#endif

InternetAccessPlugin::InternetAccessPlugin( QObject* parent ) :
	QObject( parent ),
	m_internetAccessFeature( QStringLiteral("InternetAccess"),
							 Feature::Flag::Mode | Feature::Flag::AllComponents,
							 Feature::Uid( "d3a58e21-9954-46c8-97ef-1981bdf01362" ),
							 Feature::Uid(),
							 tr( "Block internet" ), tr( "Unblock internet" ),
							 tr( "Block public internet access while keeping LAN, CBT servers, and local web intact." ),
							 QStringLiteral(":/internetaccess/internet-block.png") ),
	m_features( { m_internetAccessFeature } )
{
}

bool InternetAccessPlugin::controlFeature( Feature::Uid featureUid, Operation operation,
										  const QVariantMap& arguments,
										  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( hasFeature( featureUid ) == false )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::BlockInternet }, computerControlInterfaces );
		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::UnblockInternet }, computerControlInterfaces );
		return true;
	}

	return false;
}

bool InternetAccessPlugin::handleFeatureMessage( VeyonServerInterface& server,
												 const MessageContext& messageContext,
												 const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_internetAccessFeature.uid() )
	{
		const auto cmd = message.command<FeatureCommand>();
		if( cmd == FeatureCommand::BlockInternet )
		{
			setInternetBlocked( true );
			return true;
		}
		else if( cmd == FeatureCommand::UnblockInternet )
		{
			setInternetBlocked( false );
			return true;
		}
	}

	return false;
}
