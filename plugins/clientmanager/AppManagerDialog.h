/*
 * AppManagerDialog.h - dialog for remote application management and task termination
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
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include "ComputerControlInterface.h"
#include "FeatureMessage.h"

class AppManagerDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AppManagerDialog( const ComputerControlInterfaceList& interfaces,
							   const Feature::Uid& featureUid,
							   QWidget* parent = nullptr );
	~AppManagerDialog() override;

	void handleAppListResponse( ComputerControlInterface::Pointer iface, const QVariantList& apps );

private Q_SLOTS:
	void refreshApps();
	void killSelectedProcess();
	void killCustomProcess();

private:
	ComputerControlInterfaceList m_interfaces;
	Feature::Uid m_featureUid;

	QTableWidget* m_tableWidget;
	QLineEdit* m_customProcessEdit;
	QPushButton* m_killSelectedButton;
	QPushButton* m_killCustomButton;
	QPushButton* m_refreshButton;
	QPushButton* m_closeButton;
	QLabel* m_statusLabel;
};
