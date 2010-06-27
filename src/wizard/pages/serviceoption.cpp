/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "serviceoption.h"
#include <QButtonGroup>
#include <kservice.h>

ServiceOption::ServiceOption(const KService* service, QButtonGroup& buttonGroup, QWidget* parent):
QWidget(parent)
{
    setupUi(this);
    m_service = service;

    radioButton->setText(service->name());
    descLbl->setText(service->comment());

    buttonGroup.addButton(radioButton);
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)));
}

void ServiceOption::setChecked(bool checked)
{
    radioButton->setChecked(checked);
}

void ServiceOption::toggled(bool checked)
{
    if (checked == true) {
        emit selected(m_service);
    }
}