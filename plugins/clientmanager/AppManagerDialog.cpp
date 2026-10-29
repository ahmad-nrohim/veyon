/*
 * AppManagerDialog.cpp - dialog for remote application management and task termination
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "AppManagerDialog.h"
#include <QMessageBox>
#include <QTimer>

AppManagerDialog::AppManagerDialog( const ComputerControlInterfaceList& interfaces,
								   const Feature::Uid& featureUid,
								   QWidget* parent ) :
	QDialog( parent ),
	m_interfaces( interfaces ),
	m_featureUid( featureUid ),
	m_tableWidget( new QTableWidget( this ) ),
	m_customProcessEdit( new QLineEdit( this ) ),
	m_killSelectedButton( new QPushButton( tr( "Tutup Proses Terpilih" ), this ) ),
	m_killCustomButton( new QPushButton( tr( "Hentikan Proses" ), this ) ),
	m_refreshButton( new QPushButton( tr( "Segarkan" ), this ) ),
	m_closeButton( new QPushButton( tr( "Tutup" ), this ) ),
	m_statusLabel( new QLabel( this ) )
{
	setWindowTitle( tr( "Manajemen Aplikasi Klien" ) );
	resize( 750, 480 );

	auto mainLayout = new QVBoxLayout( this );

	m_statusLabel->setText( tr( "Menghubungi %1 komputer klien..." ).arg( m_interfaces.size() ) );
	mainLayout->addWidget( m_statusLabel );

	m_tableWidget->setColumnCount( 4 );
	m_tableWidget->setHorizontalHeaderLabels( {
		tr( "Komputer" ), tr( "PID" ), tr( "Nama Proses" ), tr( "Judul Jendela" )
	} );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
	m_tableWidget->horizontalHeader()->setSectionResizeMode( 3, QHeaderView::Stretch );
	m_tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_tableWidget->setSelectionMode( QAbstractItemView::SingleSelection );
	m_tableWidget->setAlternatingRowColors( true );
	mainLayout->addWidget( m_tableWidget );

	auto actionLayout = new QHBoxLayout();
	m_customProcessEdit->setPlaceholderText( tr( "Nama proses untuk ditutup (misal: chrome.exe atau Game.exe)" ) );
	actionLayout->addWidget( m_customProcessEdit, 2 );
	actionLayout->addWidget( m_killCustomButton, 1 );
	actionLayout->addWidget( m_killSelectedButton, 1 );
	mainLayout->addLayout( actionLayout );

	auto bottomLayout = new QHBoxLayout();
	bottomLayout->addWidget( m_refreshButton );
	bottomLayout->addStretch();
	bottomLayout->addWidget( m_closeButton );
	mainLayout->addLayout( bottomLayout );

	connect( m_refreshButton, &QPushButton::clicked, this, &AppManagerDialog::refreshApps );
	connect( m_killSelectedButton, &QPushButton::clicked, this, &AppManagerDialog::killSelectedProcess );
	connect( m_killCustomButton, &QPushButton::clicked, this, &AppManagerDialog::killCustomProcess );
	connect( m_closeButton, &QPushButton::clicked, this, &QDialog::accept );

	refreshApps();
}

AppManagerDialog::~AppManagerDialog() = default;

void AppManagerDialog::refreshApps()
{
	m_tableWidget->setRowCount( 0 );
	m_statusLabel->setText( tr( "Memuat daftar aplikasi dari %1 komputer..." ).arg( m_interfaces.size() ) );

	for( const auto& iface : m_interfaces )
	{
		if( iface )
		{
			// Command 1 = QueryAppList
			iface->sendFeatureMessage( FeatureMessage( m_featureUid, FeatureMessage::Command( 1 ) ) );
		}
	}
}

void AppManagerDialog::handleAppListResponse( ComputerControlInterface::Pointer iface, const QVariantList& apps )
{
	const QString compName = iface ? iface->hostAddress() : tr( "Klien" );

	for( const auto& appVar : apps )
	{
		const auto map = appVar.toMap();
		const int row = m_tableWidget->rowCount();
		m_tableWidget->insertRow( row );

		auto itemComp = new QTableWidgetItem( compName );
		auto itemPid = new QTableWidgetItem( QString::number( map.value( QStringLiteral("pid") ).toLongLong() ) );
		auto itemProc = new QTableWidgetItem( map.value( QStringLiteral("process") ).toString() );
		auto itemTitle = new QTableWidgetItem( map.value( QStringLiteral("title") ).toString() );

		itemComp->setFlags( itemComp->flags() ^ Qt::ItemIsEditable );
		itemPid->setFlags( itemPid->flags() ^ Qt::ItemIsEditable );
		itemProc->setFlags( itemProc->flags() ^ Qt::ItemIsEditable );
		itemTitle->setFlags( itemTitle->flags() ^ Qt::ItemIsEditable );

		m_tableWidget->setItem( row, 0, itemComp );
		m_tableWidget->setItem( row, 1, itemPid );
		m_tableWidget->setItem( row, 2, itemProc );
		m_tableWidget->setItem( row, 3, itemTitle );
	}

	m_statusLabel->setText( tr( "Ditemukan %1 jendela aplikasi aktif." ).arg( m_tableWidget->rowCount() ) );
}

void AppManagerDialog::killSelectedProcess()
{
	const int row = m_tableWidget->currentRow();
	if( row < 0 )
	{
		QMessageBox::information( this, tr( "Informasi" ), tr( "Silakan pilih baris proses yang ingin ditutup." ) );
		return;
	}

	const QString procName = m_tableWidget->item( row, 2 )->text();
	const qlonglong pid = m_tableWidget->item( row, 1 )->text().toLongLong();

	// Command 3 = KillProcess, argument 0 = pid, argument 1 = procName
	for( const auto& iface : m_interfaces )
	{
		if( iface )
		{
			iface->sendFeatureMessage( FeatureMessage( m_featureUid, FeatureMessage::Command( 3 ) )
									   .addArgument( 0, pid )
									   .addArgument( 1, procName ) );
		}
	}

	m_statusLabel->setText( tr( "Perintah penghentian proses '%1' (PID: %2) telah dikirim." ).arg( procName ).arg( pid ) );
	QTimer::singleShot( 1000, this, &AppManagerDialog::refreshApps );
}

void AppManagerDialog::killCustomProcess()
{
	const QString procName = m_customProcessEdit->text().trimmed();
	if( procName.isEmpty() )
	{
		QMessageBox::information( this, tr( "Informasi" ), tr( "Ketikkan nama proses (contoh: chrome.exe)." ) );
		return;
	}

	for( const auto& iface : m_interfaces )
	{
		if( iface )
		{
			iface->sendFeatureMessage( FeatureMessage( m_featureUid, FeatureMessage::Command( 3 ) )
									   .addArgument( 0, static_cast<qlonglong>( 0 ) )
									   .addArgument( 1, procName ) );
		}
	}

	m_statusLabel->setText( tr( "Perintah penutupan proses '%1' telah dikirim ke semua klien." ).arg( procName ) );
	m_customProcessEdit->clear();
	QTimer::singleShot( 1000, this, &AppManagerDialog::refreshApps );
}
