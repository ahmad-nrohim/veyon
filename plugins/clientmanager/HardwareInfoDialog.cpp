/*
 * HardwareInfoDialog.cpp - dialog for inspecting remote client hardware specifications
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "HardwareInfoDialog.h"

HardwareInfoDialog::HardwareInfoDialog( const ComputerControlInterfaceList& interfaces,
									   const Feature::Uid& featureUid,
									   QWidget* parent ) :
	QDialog( parent ),
	m_interfaces( interfaces ),
	m_featureUid( featureUid ),
	m_tableWidget( new QTableWidget( this ) ),
	m_refreshButton( new QPushButton( tr( "Segarkan" ), this ) ),
	m_closeButton( new QPushButton( tr( "Tutup" ), this ) ),
	m_statusLabel( new QLabel( this ) )
{
	setWindowTitle( tr( "Spesifikasi Hardware Klien" ) );
	resize( 850, 420 );

	auto mainLayout = new QVBoxLayout( this );

	m_statusLabel->setText( tr( "Mengambil data spesifikasi dari %1 komputer klien..." ).arg( m_interfaces.size() ) );
	mainLayout->addWidget( m_statusLabel );

	m_tableWidget->setColumnCount( 6 );
	m_tableWidget->setHorizontalHeaderLabels( {
		tr( "Komputer" ), tr( "Model Prosesor" ), tr( "RAM (Total/Bebas)" ),
		tr( "Disk C: (Total/Sisa)" ), tr( "Sistem Operasi" ), tr( "Uptime" )
	} );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Stretch );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 4, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 5, QHeaderView::ResizeToContents );
	m_tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_tableWidget->setSelectionMode( QAbstractItemView::SingleSelection );
	m_tableWidget->setAlternatingRowColors( true );
	mainLayout->addWidget( m_tableWidget );

	auto bottomLayout = new QHBoxLayout();
	bottomLayout->addWidget( m_refreshButton );
	bottomLayout->addStretch();
	bottomLayout->addWidget( m_closeButton );
	mainLayout->addLayout( bottomLayout );

	connect( m_refreshButton, &QPushButton::clicked, this, &HardwareInfoDialog::refreshInfo );
	connect( m_closeButton, &QPushButton::clicked, this, &QDialog::accept );

	refreshInfo();
}

HardwareInfoDialog::~HardwareInfoDialog() = default;

void HardwareInfoDialog::refreshInfo()
{
	m_tableWidget->setRowCount( 0 );
	m_statusLabel->setText( tr( "Memuat informasi perangkat keras dari %1 komputer..." ).arg( m_interfaces.size() ) );

	for( const auto& iface : m_interfaces )
	{
		if( iface )
		{
			// Command 10 = QueryHardwareInfo
			iface->sendFeatureMessage( FeatureMessage( m_featureUid, FeatureMessage::Command( 10 ) ) );
		}
	}
}

void HardwareInfoDialog::handleHardwareInfoResponse( ComputerControlInterface::Pointer iface, const QVariantMap& info )
{
	const QString compName = iface ? iface->hostAddress() : tr( "Klien" );
	const QString hostname = info.value( QStringLiteral("hostname") ).toString();
	const QString label = hostname.isEmpty() ? compName : QStringLiteral( "%1 (%2)" ).arg( compName, hostname );

	const QString cpu = info.value( QStringLiteral("cpu") ).toString();
	const qlonglong ramTotalMB = info.value( QStringLiteral("ramTotalMB") ).toLongLong();
	const qlonglong ramAvailMB = info.value( QStringLiteral("ramAvailMB") ).toLongLong();
	const QString ram = QStringLiteral( "%1 GB / %2 GB" )
						.arg( QString::number( ramTotalMB / 1024.0, 'f', 1 ) )
						.arg( QString::number( ramAvailMB / 1024.0, 'f', 1 ) );

	const qlonglong diskTotalGB = info.value( QStringLiteral("diskTotalGB") ).toLongLong();
	const qlonglong diskFreeGB = info.value( QStringLiteral("diskFreeGB") ).toLongLong();
	const QString disk = QStringLiteral( "%1 GB / %2 GB" ).arg( diskTotalGB ).arg( diskFreeGB );

	const QString os = info.value( QStringLiteral("os") ).toString();

	const qlonglong uptimeSec = info.value( QStringLiteral("uptimeSec") ).toLongLong();
	const qlonglong hours = uptimeSec / 3600;
	const qlonglong mins = ( uptimeSec % 3600 ) / 60;
	const QString uptime = QStringLiteral( "%1j %2m" ).arg( hours ).arg( mins );

	const int row = m_tableWidget->rowCount();
	m_tableWidget->insertRow( row );

	auto itemComp = new QTableWidgetItem( label );
	auto itemCpu = new QTableWidgetItem( cpu );
	auto itemRam = new QTableWidgetItem( ram );
	auto itemDisk = new QTableWidgetItem( disk );
	auto itemOs = new QTableWidgetItem( os );
	auto itemUptime = new QTableWidgetItem( uptime );

	itemComp->setFlags( itemComp->flags() ^ Qt::ItemIsEditable );
	itemCpu->setFlags( itemCpu->flags() ^ Qt::ItemIsEditable );
	itemRam->setFlags( itemRam->flags() ^ Qt::ItemIsEditable );
	itemDisk->setFlags( itemDisk->flags() ^ Qt::ItemIsEditable );
	itemOs->setFlags( itemOs->flags() ^ Qt::ItemIsEditable );
	itemUptime->setFlags( itemUptime->flags() ^ Qt::ItemIsEditable );

	m_tableWidget->setItem( row, 0, itemComp );
	m_tableWidget->setItem( row, 1, itemCpu );
	m_tableWidget->setItem( row, 2, itemRam );
	m_tableWidget->setItem( row, 3, itemDisk );
	m_tableWidget->setItem( row, 4, itemOs );
	m_tableWidget->setItem( row, 5, itemUptime );

	m_statusLabel->setText( tr( "Data diterima dari %1 komputer." ).arg( m_tableWidget->rowCount() ) );
}
