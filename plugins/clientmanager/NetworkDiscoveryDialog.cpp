/*
 * NetworkDiscoveryDialog.cpp - dialog for asynchronous subnet scanning and auto-discovery
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "NetworkDiscoveryDialog.h"
#include "NetworkObject.h"
#include "ObjectManager.h"
#include "ConfigurationManager.h"
#include "VeyonCore.h"

#include <QNetworkInterface>
#include <QMessageBox>
#include <QTimer>

NetworkDiscoveryDialog::NetworkDiscoveryDialog( QWidget* parent ) :
	QDialog( parent ),
	m_subnetEdit( new QLineEdit( this ) ),
	m_scanButton( new QPushButton( tr( "Mulai Pemindaian" ), this ) ),
	m_stopButton( new QPushButton( tr( "Hentikan" ), this ) ),
	m_addToLabButton( new QPushButton( tr( "Tambahkan ke Lokasi Lab" ), this ) ),
	m_closeButton( new QPushButton( tr( "Tutup" ), this ) ),
	m_progressBar( new QProgressBar( this ) ),
	m_tableWidget( new QTableWidget( this ) ),
	m_statusLabel( new QLabel( this ) ),
	m_currentHostIndex( 1 ),
	m_completedCount( 0 ),
	m_isScanning( false )
{
	setWindowTitle( tr( "Network Discovery - Deteksi Otomatis PC Klien" ) );
	resize( 650, 450 );

	// Auto-detect local subnet prefix
	m_subnetPrefix = QStringLiteral( "192.168.0" );
	const auto interfaces = QNetworkInterface::allInterfaces();
	for( const auto& iface : interfaces )
	{
		if( ( iface.flags() & QNetworkInterface::IsUp ) &&
			( iface.flags() & QNetworkInterface::IsRunning ) &&
			!( iface.flags() & QNetworkInterface::IsLoopBack ) )
		{
			for( const auto& entry : iface.addressEntries() )
			{
				const auto ip = entry.ip();
				if( ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback() )
				{
					const auto parts = ip.toString().split( QLatin1Char( '.' ) );
					if( parts.size() == 4 )
					{
						m_subnetPrefix = QStringLiteral( "%1.%2.%3" ).arg( parts[0], parts[1], parts[2] );
						break;
					}
				}
			}
		}
	}
	m_subnetEdit->setText( m_subnetPrefix );

	auto mainLayout = new QVBoxLayout( this );

	auto inputLayout = new QHBoxLayout();
	inputLayout->addWidget( new QLabel( tr( "Subnet IP:" ), this ) );
	inputLayout->addWidget( m_subnetEdit, 1 );
	inputLayout->addWidget( m_scanButton );
	inputLayout->addWidget( m_stopButton );
	m_stopButton->setEnabled( false );
	mainLayout->addLayout( inputLayout );

	m_progressBar->setRange( 0, 254 );
	m_progressBar->setValue( 0 );
	mainLayout->addWidget( m_progressBar );

	m_statusLabel->setText( tr( "Ketik subnet IP lalu klik 'Mulai Pemindaian' untuk mencari komputer klien." ) );
	mainLayout->addWidget( m_statusLabel );

	m_tableWidget->setColumnCount( 3 );
	m_tableWidget->setHorizontalHeaderLabels( { tr( "Alamat IP" ), tr( "Nama Host" ), tr( "Status Veyon" ) } );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
	m_tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_tableWidget->setAlternatingRowColors( true );
	mainLayout->addWidget( m_tableWidget );

	auto bottomLayout = new QHBoxLayout();
	bottomLayout->addWidget( m_addToLabButton );
	bottomLayout->addStretch();
	bottomLayout->addWidget( m_closeButton );
	mainLayout->addLayout( bottomLayout );

	connect( m_scanButton, &QPushButton::clicked, this, &NetworkDiscoveryDialog::startScan );
	connect( m_stopButton, &QPushButton::clicked, this, &NetworkDiscoveryDialog::stopScan );
	connect( m_addToLabButton, &QPushButton::clicked, this, &NetworkDiscoveryDialog::addDiscoveredToLab );
	connect( m_closeButton, &QPushButton::clicked, this, &QDialog::accept );
}

NetworkDiscoveryDialog::~NetworkDiscoveryDialog()
{
	stopScan();
}

void NetworkDiscoveryDialog::startScan()
{
	m_subnetPrefix = m_subnetEdit->text().trimmed();
	if( m_subnetPrefix.endsWith( QLatin1Char( '.' ) ) )
	{
		m_subnetPrefix.chop( 1 );
	}

	m_tableWidget->setRowCount( 0 );
	m_discoveredHosts.clear();
	m_currentHostIndex = 1;
	m_completedCount = 0;
	m_isScanning = true;

	m_scanButton->setEnabled( false );
	m_stopButton->setEnabled( true );
	m_progressBar->setValue( 0 );
	m_statusLabel->setText( tr( "Memindai subnet %1.1 s/d %1.254 pada port 11100..." ).arg( m_subnetPrefix ) );

	checkNextBatch();
}

void NetworkDiscoveryDialog::stopScan()
{
	m_isScanning = false;
	m_scanButton->setEnabled( true );
	m_stopButton->setEnabled( false );
}

void NetworkDiscoveryDialog::checkNextBatch()
{
	if( !m_isScanning )
	{
		return;
	}

	// Scan in concurrent batches of 32 sockets
	const int batchSize = 32;
	for( int i = 0; i < batchSize && m_currentHostIndex <= 254; ++i, ++m_currentHostIndex )
	{
		const QString targetIp = QStringLiteral( "%1.%2" ).arg( m_subnetPrefix ).arg( m_currentHostIndex );

		auto socket = new QTcpSocket( this );
		socket->setProperty( "targetIp", targetIp );

		connect( socket, &QTcpSocket::connected, this, &NetworkDiscoveryDialog::onSocketConnected );
		connect( socket, &QAbstractSocket::errorOccurred, this, &NetworkDiscoveryDialog::onSocketError );

		// 800ms timeout for LAN probe
		QTimer::singleShot( 800, socket, [socket, this]() {
			if( socket->state() != QAbstractSocket::ConnectedState )
			{
				socket->abort();
				onSocketError();
			}
		} );

		socket->connectToHost( targetIp, 11100 );
	}
}

void NetworkDiscoveryDialog::onSocketConnected()
{
	auto socket = qobject_cast<QTcpSocket*>( sender() );
	if( !socket )
	{
		return;
	}

	const QString ip = socket->property( "targetIp" ).toString();
	socket->disconnectFromHost();
	socket->deleteLater();

	++m_completedCount;
	m_progressBar->setValue( m_completedCount );

	DiscoveredHost host;
	host.ip = ip;
	host.hostname = ip;
	m_discoveredHosts.append( host );

	const int row = m_tableWidget->rowCount();
	m_tableWidget->insertRow( row );

	auto itemIp = new QTableWidgetItem( ip );
	auto itemHost = new QTableWidgetItem( tr( "Mencari hostname..." ) );
	auto itemStatus = new QTableWidgetItem( tr( "Aktif (Port 11100)" ) );

	itemIp->setFlags( itemIp->flags() ^ Qt::ItemIsEditable );
	itemHost->setFlags( itemHost->flags() ^ Qt::ItemIsEditable );
	itemStatus->setFlags( itemStatus->flags() ^ Qt::ItemIsEditable );

	m_tableWidget->setItem( row, 0, itemIp );
	m_tableWidget->setItem( row, 1, itemHost );
	m_tableWidget->setItem( row, 2, itemStatus );

	// Asynchronous reverse DNS lookup
	QHostInfo::lookupHost( ip, this, [this, row, ip]( const QHostInfo& hostInfo ) {
		if( row < m_tableWidget->rowCount() )
		{
			const QString resolvedName = hostInfo.hostName().isEmpty() ? ip : hostInfo.hostName();
			m_tableWidget->setItem( row, 1, new QTableWidgetItem( resolvedName ) );
			for( auto& h : m_discoveredHosts )
			{
				if( h.ip == ip )
				{
					h.hostname = resolvedName;
					break;
				}
			}
		}
	} );

	if( m_currentHostIndex <= 254 )
	{
		checkNextBatch();
	}
	else if( m_completedCount >= 254 )
	{
		stopScan();
		m_statusLabel->setText( tr( "Pemindaian selesai. Ditemukan %1 komputer aktif." ).arg( m_discoveredHosts.size() ) );
	}
}

void NetworkDiscoveryDialog::onSocketError()
{
	auto socket = qobject_cast<QTcpSocket*>( sender() );
	if( socket )
	{
		socket->deleteLater();
	}

	++m_completedCount;
	m_progressBar->setValue( m_completedCount );

	if( m_currentHostIndex <= 254 )
	{
		checkNextBatch();
	}
	else if( m_completedCount >= 254 )
	{
		stopScan();
		m_statusLabel->setText( tr( "Pemindaian selesai. Ditemukan %1 komputer aktif." ).arg( m_discoveredHosts.size() ) );
	}
}

void NetworkDiscoveryDialog::addDiscoveredToLab()
{
	if( m_discoveredHosts.isEmpty() )
	{
		QMessageBox::information( this, tr( "Informasi" ), tr( "Tidak ada komputer aktif yang ditemukan." ) );
		return;
	}

	// Access Veyon's stored NetworkObjects
	const auto jsonArray = VeyonCore::config().value( QStringLiteral("NetworkObjects"), QStringLiteral("BuiltinDirectory"), QJsonArray() ).toJsonArray();
	ObjectManager<NetworkObject> objectManager( jsonArray );

	// Find or create default location "Lab TKJ"
	NetworkObject labLocation;
	for( const auto& objVal : objectManager.objects() )
	{
		const NetworkObject obj( objVal.toObject() );
		if( obj.type() == NetworkObject::Type::Location )
		{
			labLocation = obj;
			break;
		}
	}

	if( labLocation.uid().isNull() )
	{
		labLocation = NetworkObject( NetworkObject::Type::Location, QStringLiteral("Lab TKJ"), {}, {}, {}, QUuid::createUuid() );
		objectManager.add( labLocation );
	}

	int addedCount = 0;
	for( const auto& host : m_discoveredHosts )
	{
		bool exists = false;
		for( const auto& objVal : objectManager.objects() )
		{
			const NetworkObject obj( objVal.toObject() );
			if( obj.type() == NetworkObject::Type::Host && obj.hostAddress() == host.ip )
			{
				exists = true;
				break;
			}
		}

		if( !exists )
		{
			const QString pcName = host.hostname.isEmpty() ? host.ip : host.hostname;
			objectManager.add( NetworkObject( NetworkObject::Type::Host, pcName, host.ip, {}, {}, QUuid::createUuid(), labLocation.uid() ) );
			++addedCount;
		}
	}

	VeyonCore::config().setValue( QStringLiteral("NetworkObjects"), objectManager.objects(), QStringLiteral("BuiltinDirectory") );

	ConfigurationManager configManager;
	configManager.saveConfiguration();

	QMessageBox::information( this, tr( "Sukses" ),
		tr( "%1 komputer baru berhasil ditambahkan ke lokasi '%2'!" ).arg( addedCount ).arg( labLocation.name() ) );
}
