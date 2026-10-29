/*
 * ClientManagerPlugin.cpp - implementation of ClientManagerPlugin class
 *
 * Copyright (c) 2026 Ahmad Nurohim <ahmad.nrohim@gmail.com>
 *
 * This file is part of Veyon - https://veyon.io
 */

#include "ClientManagerPlugin.h"
#include "AppManagerDialog.h"
#include "HardwareInfoDialog.h"
#include "NetworkDiscoveryDialog.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "ComputerControlInterface.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>

static QVariantList queryRunningApps()
{
	QVariantList appList;

	EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
		if (!IsWindowVisible(hwnd)) return TRUE;
		if (GetWindowTextLengthW(hwnd) == 0) return TRUE;

		LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
		if (exStyle & WS_EX_TOOLWINDOW) return TRUE;

		wchar_t title[512] = {0};
		GetWindowTextW(hwnd, title, 512);

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == 0) return TRUE;

		HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		wchar_t path[MAX_PATH] = {0};
		DWORD size = MAX_PATH;
		if (hProc)
		{
			QueryFullProcessImageNameW(hProc, 0, path, &size);
			CloseHandle(hProc);
		}

		QString pName = QFileInfo(QString::fromWCharArray(path)).fileName();
		if (pName.isEmpty())
		{
			pName = QString::number(pid);
		}

		if (pName.compare(QStringLiteral("explorer.exe"), Qt::CaseInsensitive) == 0 &&
			QString::fromWCharArray(title) == QStringLiteral("Program Manager"))
		{
			return TRUE;
		}

		QVariantMap item;
		item[QStringLiteral("pid")] = static_cast<qlonglong>(pid);
		item[QStringLiteral("process")] = pName;
		item[QStringLiteral("title")] = QString::fromWCharArray(title);

		auto* list = reinterpret_cast<QVariantList*>(lParam);
		list->append(item);

		return TRUE;
	}, reinterpret_cast<LPARAM>(&appList));

	return appList;
}

static void killClientProcess(qlonglong pid, const QString& procName)
{
	if (pid > 0)
	{
		HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
		if (hProc)
		{
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
		}
	}
	if (!procName.isEmpty())
	{
		QProcess::execute(QStringLiteral("taskkill"), {
			QStringLiteral("/F"), QStringLiteral("/IM"), procName
		});
	}
}

static QVariantMap queryHardwareInfo()
{
	QVariantMap info;

	wchar_t compName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
	DWORD compSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW(compName, &compSize);
	info[QStringLiteral("hostname")] = QString::fromWCharArray(compName);

	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		wchar_t cpuName[256] = {0};
		DWORD dataSize = sizeof(cpuName);
		if (RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr, reinterpret_cast<LPBYTE>(cpuName), &dataSize) == ERROR_SUCCESS)
		{
			info[QStringLiteral("cpu")] = QString::fromWCharArray(cpuName).trimmed();
		}
		RegCloseKey(hKey);
	}

	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);
	if (GlobalMemoryStatusEx(&memStatus))
	{
		info[QStringLiteral("ramTotalMB")] = static_cast<qlonglong>(memStatus.ullTotalPhys / (1024 * 1024));
		info[QStringLiteral("ramAvailMB")] = static_cast<qlonglong>(memStatus.ullAvailPhys / (1024 * 1024));
	}

	ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
	if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvail, &totalBytes, &totalFreeBytes))
	{
		info[QStringLiteral("diskTotalGB")] = static_cast<qlonglong>(totalBytes.QuadPart / (1024ULL * 1024ULL * 1024ULL));
		info[QStringLiteral("diskFreeGB")] = static_cast<qlonglong>(totalFreeBytes.QuadPart / (1024ULL * 1024ULL * 1024ULL));
	}

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		wchar_t prodName[256] = {0};
		DWORD dataSize = sizeof(prodName);
		if (RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, reinterpret_cast<LPBYTE>(prodName), &dataSize) == ERROR_SUCCESS)
		{
			wchar_t dispVer[64] = {0};
			DWORD dispSize = sizeof(dispVer);
			RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr, reinterpret_cast<LPBYTE>(dispVer), &dispSize);
			QString osStr = QString::fromWCharArray(prodName);
			if (wcslen(dispVer) > 0)
			{
				osStr += QStringLiteral(" ") + QString::fromWCharArray(dispVer);
			}
			info[QStringLiteral("os")] = osStr;
		}
		RegCloseKey(hKey);
	}

	ULONGLONG ticks = GetTickCount64();
	info[QStringLiteral("uptimeSec")] = static_cast<qlonglong>(ticks / 1000);

	return info;
}
#else
static QVariantList queryRunningApps() { return {}; }
static void killClientProcess(qlonglong, const QString&) {}
static QVariantMap queryHardwareInfo() { return {}; }
#endif

ClientManagerPlugin::ClientManagerPlugin( QObject* parent ) :
	QObject( parent ),
	m_appManagerFeature( QStringLiteral("AppManager"),
						 Feature::Flag::Action | Feature::Flag::AllComponents,
						 Feature::Uid( "a219bc82-0193-4a11-b4ef-f993d07e1124" ),
						 Feature::Uid(),
						 tr( "Kelola Aplikasi" ), {},
						 tr( "Lihat daftar aplikasi aktif dan hentikan paksa proses di komputer klien." ),
						 QStringLiteral(":/clientmanager/app-manager.png") ),
	m_hardwareInfoFeature( QStringLiteral("HardwareInfo"),
						   Feature::Flag::Action | Feature::Flag::AllComponents,
						   Feature::Uid( "b831fa94-3991-4cf1-8a8b-18f4ad921102" ),
						   Feature::Uid(),
						   tr( "Info Hardware" ), {},
						   tr( "Inspeksi spesifikasi CPU, RAM, disk, dan sistem operasi komputer klien." ),
						   QStringLiteral(":/clientmanager/hardware-info.png") ),
	m_networkDiscoveryFeature( QStringLiteral("NetworkDiscovery"),
							   Feature::Flag::Action | Feature::Flag::AllComponents,
							   Feature::Uid( "c9421a11-5820-4bd2-9132-8491c1092044" ),
							   Feature::Uid(),
							   tr( "Pindai Jaringan" ), {},
							   tr( "Pindai subnet secara otomatis untuk menemukan komputer klien Veyon baru." ),
							   QStringLiteral(":/clientmanager/network-discovery.png") ),
	m_features( { m_appManagerFeature, m_hardwareInfoFeature, m_networkDiscoveryFeature } )
{
}

bool ClientManagerPlugin::controlFeature( Feature::Uid featureUid, Operation operation,
										 const QVariantMap& arguments,
										 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(featureUid)
	Q_UNUSED(operation)
	Q_UNUSED(arguments)
	Q_UNUSED(computerControlInterfaces)
	return true;
}

bool ClientManagerPlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
										const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() == m_appManagerFeature.uid() )
	{
		if( computerControlInterfaces.isEmpty() )
		{
			QMessageBox::information( master.mainWindow(), tr( "Informasi" ),
				tr( "Silakan pilih minimal satu komputer klien terlebih dahulu." ) );
			return false;
		}

		AppManagerDialog dialog( computerControlInterfaces, feature.uid(), master.mainWindow() );
		m_activeAppDialog = &dialog;
		dialog.exec();
		m_activeAppDialog = nullptr;
		return true;
	}

	if( feature.uid() == m_hardwareInfoFeature.uid() )
	{
		if( computerControlInterfaces.isEmpty() )
		{
			QMessageBox::information( master.mainWindow(), tr( "Informasi" ),
				tr( "Silakan pilih minimal satu komputer klien terlebih dahulu." ) );
			return false;
		}

		HardwareInfoDialog dialog( computerControlInterfaces, feature.uid(), master.mainWindow() );
		m_activeHardwareDialog = &dialog;
		dialog.exec();
		m_activeHardwareDialog = nullptr;
		return true;
	}

	if( feature.uid() == m_networkDiscoveryFeature.uid() )
	{
		NetworkDiscoveryDialog dialog( master.mainWindow() );
		dialog.exec();
		return true;
	}

	return false;
}

bool ClientManagerPlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
												const FeatureMessage& message )
{
	if( message.featureUid() == m_appManagerFeature.uid() )
	{
		if( m_activeAppDialog && message.command() == FeatureMessage::Command( 2 ) )
		{
			m_activeAppDialog->handleAppListResponse( computerControlInterface, message.argument( 0 ).toList() );
			return true;
		}
	}

	if( message.featureUid() == m_hardwareInfoFeature.uid() )
	{
		if( m_activeHardwareDialog && message.command() == FeatureMessage::Command( 11 ) )
		{
			m_activeHardwareDialog->handleHardwareInfoResponse( computerControlInterface, message.argument( 0 ).toMap() );
			return true;
		}
	}

	return false;
}

bool ClientManagerPlugin::handleFeatureMessage( VeyonServerInterface& server,
												const MessageContext& messageContext,
												const FeatureMessage& message )
{
	if( message.featureUid() == m_appManagerFeature.uid() )
	{
		if( message.command() == FeatureMessage::Command( 1 ) )
		{
			const auto apps = queryRunningApps();
			server.sendFeatureMessageReply( messageContext,
				FeatureMessage( m_appManagerFeature.uid(), FeatureMessage::Command( 2 ) ).addArgument( 0, apps ) );
			return true;
		}

		if( message.command() == FeatureMessage::Command( 3 ) )
		{
			const auto pid = message.argument( 0 ).toLongLong();
			const auto procName = message.argument( 1 ).toString();
			killClientProcess( pid, procName );

			const auto apps = queryRunningApps();
			server.sendFeatureMessageReply( messageContext,
				FeatureMessage( m_appManagerFeature.uid(), FeatureMessage::Command( 2 ) ).addArgument( 0, apps ) );
			return true;
		}
	}

	if( message.featureUid() == m_hardwareInfoFeature.uid() )
	{
		if( message.command() == FeatureMessage::Command( 10 ) )
		{
			const auto info = queryHardwareInfo();
			server.sendFeatureMessageReply( messageContext,
				FeatureMessage( m_hardwareInfoFeature.uid(), FeatureMessage::Command( 11 ) ).addArgument( 0, info ) );
			return true;
		}
	}

	return false;
}
