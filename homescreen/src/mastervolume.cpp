/*
 * Copyright (C) 2017 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mastervolume.h"
#include <QTimer>
#include <QtDebug>

MasterVolume::MasterVolume(QObject* parent) :
	QObject(parent),
	m_volume(50)
{
	VehicleSignalsConfig vsConfig("homescreen");
	m_vs = new VehicleSignals(vsConfig);

	if (m_vs) {
		QObject::connect(m_vs, &VehicleSignals::connected, this, &MasterVolume::onConnected);
		QObject::connect(m_vs, &VehicleSignals::authorized, this, &MasterVolume::onAuthorized);
		QObject::connect(m_vs, &VehicleSignals::disconnected, this, &MasterVolume::onDisconnected);

		m_vs->connect();
	}
}

qint32 MasterVolume::getVolume() const
{
	return m_volume;
}

void MasterVolume::setVolume(qint32 volume)
{
	if (m_volume == volume)
		return;

	m_volume = volume;

	if (!(m_vs && m_connected))
		return;

	m_vs->set("Vehicle.Cabin.Infotainment.Media.Volume", QString::number(volume));
}

void MasterVolume::onConnected()
{
	if (!m_vs)
		return;

	m_vs->authorize();
}

void MasterVolume::onAuthorized()
{
	if (!m_vs)
		return;

	m_connected = true;

	QObject::connect(m_vs, &VehicleSignals::getSuccessResponse, this, &MasterVolume::onGetSuccessResponse);
	QObject::connect(m_vs, &VehicleSignals::signalNotification, this, &MasterVolume::onSignalNotification);

	m_vs->subscribe("Vehicle.Cabin.Infotainment.Media.Volume");
	m_vs->get("Vehicle.Cabin.Infotainment.Media.Volume");
}

void MasterVolume::onDisconnected()
{
	QObject::disconnect(m_vs, &VehicleSignals::signalNotification, this, &MasterVolume::onGetSuccessResponse);
	QObject::disconnect(m_vs, &VehicleSignals::signalNotification, this, &MasterVolume::onSignalNotification);

	m_connected = false;
}

void MasterVolume::updateVolume(QString value)
{
	bool ok;
	qint32 volume = value.toInt(&ok);
	if (ok) {
		volume = qBound(0, volume, 100);
		if (m_volume != volume)	{
			m_volume = volume;
			emit VolumeChanged();
		}
	}
}

void MasterVolume::onGetSuccessResponse(QString path, QString value, QString timestamp)
{
	if (path == "Vehicle.Cabin.Infotainment.Media.Volume") {
		updateVolume(value);
		emit VolumeChanged();
	}
}

void MasterVolume::onSignalNotification(QString path, QString value, QString timestamp)
{
	if (path == "Vehicle.Cabin.Infotainment.Media.Volume")
		updateVolume(value);
}
