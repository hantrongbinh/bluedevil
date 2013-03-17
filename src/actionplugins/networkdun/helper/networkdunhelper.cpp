/*************************************************************************************
 *  Copyright (C) 2010 by Alex Fiestas <alex@eyeos.org>                              *
 *  Copyright (C) 2010 UFO Coders <info@ufocoders.com>                               *
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

#include "networkdunhelper.h"
#include "../../actionplugin.h"

#include <QApplication>
#include <QDebug>

#include <klocalizedstring.h>
#include <kservicetypetrader.h>
#include <ktoolinvocation.h>

#include <bluedevil/bluedevil.h>

using namespace BlueDevil;

NetworkDUNHelper::NetworkDUNHelper(const KUrl& address) {
    if (!BlueDevil::Manager::self()->usableAdapter()) {
        qDebug() << "No Adapters found";
        qApp->exit();
        return;
    }

    Device *device = Manager::self()->usableAdapter()->deviceForAddress(address.host().replace("-", ":"));

    if(device->isPaired()) {
        QString constraint("'00001103-0000-1000-8000-00805F9B34FB' in [X-BlueDevil-UUIDS]");

        KPluginFactory *factory = KPluginLoader(
            KServiceTypeTrader::self()->query("BlueDevil/ActionPlugin", constraint).first().data()->library()
        ).factory();

        ActionPlugin *plugin = factory->create<ActionPlugin>(this);
        connect(plugin, SIGNAL(finished()), qApp, SLOT(quit()), Qt::QueuedConnection);

        plugin->setDevice(device);
        plugin->startAction();
    } else {
        KToolInvocation::kdeinitExec("bluedevil-wizard", QStringList() << address.url());
    }
}