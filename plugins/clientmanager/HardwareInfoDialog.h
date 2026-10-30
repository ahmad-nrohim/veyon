/*
 * HardwareInfoDialog.h - dialog for inspecting remote client hardware specifications
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"

class HardwareInfoDialog : public QDialog
{
	Q_OBJECT
public:
	explicit HardwareInfoDialog( const ComputerControlInterfaceList& interfaces,
								 const Feature::Uid& featureUid,
								 QWidget* parent = nullptr );
	~HardwareInfoDialog() override;

	void handleHardwareInfoResponse( ComputerControlInterface::Pointer iface, const QVariantMap& info );

private Q_SLOTS:
	void refreshInfo();
	void exportToCsv();

private:
	ComputerControlInterfaceList m_interfaces;
	Feature::Uid m_featureUid;

	QTableWidget* m_tableWidget;
	QPushButton* m_refreshButton;
	QPushButton* m_exportButton;
	QPushButton* m_closeButton;
	QLabel* m_statusLabel;
};
