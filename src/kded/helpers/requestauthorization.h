/***************************************************************************
 *   Copyright (C) 2010 Alejandro Fiestas Olivares <alex@eyeos.org>        *
 *   Copyright (C) 2010 Eduardo Robles Elvira <edulix@gmail.com>           *
 *   Copyright (C) 2010 UFO Coders <info@ufocoders.com>                    *
 *   Copyright (C) 2014-2015 David Rosca <nowrep@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef REQUESTAUTHORIZATION_H
#define REQUESTAUTHORIZATION_H

#include <QObject>

#include <BluezQt/Device>

class RequestAuthorization : public QObject
{
    Q_OBJECT

public:
    enum Result {
        Deny,
        Accept,
        AcceptAndTrust
    };

    explicit RequestAuthorization(BluezQt::DevicePtr device, QObject *parent = nullptr);

Q_SIGNALS:
    void done(Result result);

private Q_SLOTS:
    void authorizeAndTrust();
    void authorize();
    void deny();

private:
    BluezQt::DevicePtr m_device;

};

#endif // REQUESTAUTHORIZATION_H
