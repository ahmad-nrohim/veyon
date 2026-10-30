/*
 * NetworkDiscoveryDialog.h - dialog for asynchronous subnet scanning and auto-discovery
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTcpSocket>
#include <QHostInfo>

class NetworkDiscoveryDialog : public QDialog
{
	Q_OBJECT
public:
	explicit NetworkDiscoveryDialog( QWidget* parent = nullptr );
	~NetworkDiscoveryDialog() override;

private Q_SLOTS:
	void startScan();
	void stopScan();
	void onSocketConnected();
	void onSocketError();
	void addDiscoveredToLab();

private:
	void checkNextBatch();
	bool isHostAlreadyRegistered( const QString& ip, const QString& hostname ) const;

	QLineEdit* m_subnetEdit;
	QPushButton* m_scanButton;
	QPushButton* m_stopButton;
	QPushButton* m_addToLabButton;
	QPushButton* m_closeButton;
	QProgressBar* m_progressBar;
	QTableWidget* m_tableWidget;
	QLabel* m_statusLabel;

	int m_currentHostIndex;
	int m_completedCount;
	bool m_isScanning;
	QString m_subnetPrefix;

	struct DiscoveredHost
	{
		QString ip;
		QString hostname;
	};
	QList<DiscoveredHost> m_discoveredHosts;
};
