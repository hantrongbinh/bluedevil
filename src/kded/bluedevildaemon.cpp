/*************************************************************************************
 *  Copyright (C) 2010-2012 by Alejandro Fiestas Olivares <afiestas@kde.org>         *
 *  Copyright (C) 2010 Eduardo Robles Elvira <edulix@gmail.com>                      *
 *  Copyright (C) 2010 UFO Coders <info@ufocoders.com>                               *
 *  Copyright (C) 2014-2015 David Rosca <nowrep@gmail.com>                           *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "bluedevildaemon.h"
#include "bluezagent.h"
#include "obexftp.h"
#include "obexagent.h"
#include "devicemonitor.h"
#include "debug_p.h"
#include "version.h"

#include <QTimer>
#include <QDBusMetaType>

#include <KAboutData>
#include <KPluginFactory>
#include <KLocalizedString>

#include <BluezQt/Device>
#include <BluezQt/Adapter>
#include <BluezQt/PendingCall>
#include <BluezQt/InitManagerJob>
#include <BluezQt/InitObexManagerJob>

K_PLUGIN_FACTORY_WITH_JSON(BlueDevilFactory,
                           "bluedevil.json",
                           registerPlugin<BlueDevilDaemon>();)

typedef QMap<QString, QString> DeviceInfo;
typedef QMap<QString, DeviceInfo> QMapDeviceInfo;

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(QMapDeviceInfo)

struct BlueDevilDaemon::Private
{
    BluezQt::Manager *m_manager;
    BluezQt::ObexManager *m_obexManager;

    QTimer m_timer;
    ObexFtp *m_obexFtp;
    ObexAgent *m_obexAgent;
    BluezAgent *m_bluezAgent;
    DeviceMonitor *m_deviceMonitor;
};

BlueDevilDaemon::BlueDevilDaemon(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
    , d(new Private)
{
    qDBusRegisterMetaType<DeviceInfo>();
    qDBusRegisterMetaType<QMapDeviceInfo>();

    d->m_manager = new BluezQt::Manager(this);
    d->m_obexManager = new BluezQt::ObexManager(this);
    d->m_obexFtp = new ObexFtp(this);
    d->m_obexAgent = new ObexAgent(this);
    d->m_bluezAgent = new BluezAgent(this);
    d->m_deviceMonitor = new DeviceMonitor(this);

    d->m_timer.setSingleShot(true);
    connect(&d->m_timer, &QTimer::timeout, this, &BlueDevilDaemon::stopDiscovering);

    KAboutData aboutData(
        QStringLiteral("bluedevildaemon"),
        i18n("Bluetooth Daemon"),
        BLUEDEVIL_VERSION,
        i18n("Bluetooth Daemon"),
        KAboutLicense::GPL,
        i18n("(c) 2010, UFO Coders")
    );

    aboutData.addAuthor(QStringLiteral("David Rosca"), i18n("Maintainer"),
                        QStringLiteral("nowrep@gmail.com"), QStringLiteral("http://david.rosca.cz"));

    aboutData.addAuthor(QStringLiteral("Alejandro Fiestas Olivares"), i18n("Previous Maintainer"),
                        QStringLiteral("afiestas@kde.org"), QStringLiteral("http://www.afiestas.org"));

    aboutData.addAuthor(QStringLiteral("Eduardo Robles Elvira"), i18n("Previous Maintainer"),
                        QStringLiteral("edulix@gmail.com"), QStringLiteral("http://blog.edulix.es"));

    aboutData.setComponentName(QStringLiteral("bluedevil"));
    KAboutData::registerPluginData(aboutData);

    // Initialize BluezQt
    BluezQt::InitManagerJob *job = d->m_manager->init();
    job->start();
    connect(job, &BluezQt::InitManagerJob::result, this, &BlueDevilDaemon::initJobResult);

    BluezQt::InitObexManagerJob *initJob = d->m_obexManager->init();
    initJob->start();
    connect(initJob, &BluezQt::InitObexManagerJob::result, this, &BlueDevilDaemon::initObexJobResult);

    qCDebug(BLUEDAEMON) << "Created";
}

BlueDevilDaemon::~BlueDevilDaemon()
{
    d->m_manager->unregisterAgent(d->m_bluezAgent);;
    d->m_obexManager->unregisterAgent(d->m_obexAgent);

    d->m_deviceMonitor->saveState();

    qCDebug(BLUEDAEMON) << "Destroyed";

    delete d;
}

bool BlueDevilDaemon::isOnline()
{
    return d->m_manager->isBluetoothOperational();
}

QMapDeviceInfo BlueDevilDaemon::allDevices()
{
    QMapDeviceInfo devices;

    Q_FOREACH (BluezQt::DevicePtr device, d->m_manager->devices()) {
        devices[device->address()] = deviceToInfo(device);
    }

    return devices;
}

DeviceInfo BlueDevilDaemon::device(const QString &address)
{
    BluezQt::DevicePtr device = d->m_manager->deviceForAddress(address);
    return deviceToInfo(device);
}

void BlueDevilDaemon::startDiscovering(quint32 timeout)
{
    if (!d->m_manager->usableAdapter()) {
        return;
    }

    qCDebug(BLUEDAEMON) << "Start discovering for" << timeout << "ms";

    d->m_manager->usableAdapter()->startDiscovery();

    if (timeout > 0) {
        d->m_timer.start(timeout);
    }
}

void BlueDevilDaemon::stopDiscovering()
{
    if (!d->m_manager->usableAdapter()) {
        return;
    }

    qCDebug(BLUEDAEMON) << "Stop discovering";

    if (d->m_manager->usableAdapter()->isDiscovering()) {
        d->m_manager->usableAdapter()->stopDiscovery();
    }
}

BluezQt::Manager *BlueDevilDaemon::manager() const
{
    return d->m_manager;
}

BluezQt::ObexManager *BlueDevilDaemon::obexManager() const
{
    return d->m_obexManager;
}

void BlueDevilDaemon::initJobResult(BluezQt::InitManagerJob *job)
{
    if (job->error()) {
        qCDebug(BLUEDAEMON) << "Error initializing manager:" << job->errorText();
        return;
    }

    operationalChanged(d->m_manager->isOperational());
    connect(d->m_manager, &BluezQt::Manager::operationalChanged, this, &BlueDevilDaemon::operationalChanged);
}

void BlueDevilDaemon::initObexJobResult(BluezQt::InitObexManagerJob *job)
{
    if (job->error()) {
        qCDebug(BLUEDAEMON) << "Error initializing obex manager:" << job->errorText();
        return;
    }

    obexOperationalChanged(d->m_obexManager->isOperational());
    connect(d->m_obexManager, &BluezQt::ObexManager::operationalChanged, this, &BlueDevilDaemon::obexOperationalChanged);
}

void BlueDevilDaemon::operationalChanged(bool operational)
{
    qCDebug(BLUEDAEMON) << "Bluetooth operational changed" << operational;

    if (operational) {
        BluezQt::PendingCall *rCall = d->m_manager->registerAgent(d->m_bluezAgent);
        connect(rCall, &BluezQt::PendingCall::finished, this, &BlueDevilDaemon::agentRegisted);

        BluezQt::PendingCall *rdCall = d->m_manager->requestDefaultAgent(d->m_bluezAgent);
        connect(rdCall, &BluezQt::PendingCall::finished, this, &BlueDevilDaemon::agentRequestedDefault);
    } else {
        // Attempt to start bluetoothd
        BluezQt::Manager::startService();
    }
}

void BlueDevilDaemon::obexOperationalChanged(bool operational)
{
    qCDebug(BLUEDAEMON) << "ObexManager operational changed" << operational;

    if (operational) {
        BluezQt::PendingCall *call = d->m_obexManager->registerAgent(d->m_obexAgent);
        connect(call, &BluezQt::PendingCall::finished, this, &BlueDevilDaemon::obexAgentRegistered);
    } else {
        // Attempt to start obexd
        BluezQt::ObexManager::startService();
    }
}

void BlueDevilDaemon::agentRegisted(BluezQt::PendingCall *call)
{
    if (call->error()) {
        qCWarning(BLUEDAEMON) << "Error registering Agent" << call->errorText();
    } else {
        qCDebug(BLUEDAEMON) << "Agent registered";
    }
}

void BlueDevilDaemon::agentRequestedDefault(BluezQt::PendingCall *call)
{
    if (call->error()) {
        qCWarning(BLUEDAEMON) << "Error requesting default Agent" << call->errorText();
    } else {
        qCDebug(BLUEDAEMON) << "Requested default Agent";
    }
}

void BlueDevilDaemon::obexAgentRegistered(BluezQt::PendingCall *call)
{
    if (call->error()) {
        qCWarning(BLUEDAEMON) << "Error registering ObexAgent" << call->errorText();
    } else {
        qCDebug(BLUEDAEMON) << "ObexAgent registered";
    }
}

DeviceInfo BlueDevilDaemon::deviceToInfo(BluezQt::DevicePtr device) const
{
    DeviceInfo info;

    if (!device) {
        return info;
    }

    info[QStringLiteral("name")] = device->name();
    info[QStringLiteral("icon")] = device->icon();
    info[QStringLiteral("address")] = device->address();
    info[QStringLiteral("UBI")] = device->ubi();
    info[QStringLiteral("UUIDs")] = device->uuids().join(QLatin1Char(','));

    return info;
}

#include "bluedevildaemon.moc"
