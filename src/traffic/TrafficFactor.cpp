/***************************************************************************
 *   Copyright (C) 2020-2021 by Stefan Kebekus                             *
 *   stefan.kebekus@gmail.com                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "Settings.h"
#include "positioning/PositionProvider.h"
#include "traffic/TrafficFactor.h"


Traffic::TrafficFactor::TrafficFactor(QObject *parent) : TrafficFactor_Abstract(parent)
{  

    // Bindings for property description
    connect(this, &Traffic::TrafficFactor::positionInfoChanged, this, &Traffic::TrafficFactor::dispatchUpdateDescription);

    // Bindings for property icon
    connect(this, &Traffic::TrafficFactor_Abstract::colorChanged, this, &Traffic::TrafficFactor::updateIcon);
    connect(this, &Traffic::TrafficFactor::positionInfoChanged, this, &Traffic::TrafficFactor::updateIcon);

    // Bindings for property valid
    connect(this, &Traffic::TrafficFactor::positionInfoChanged, this, &Traffic::TrafficFactor::dispatchUpdateValid);

}


void Traffic::TrafficFactor::setPositionInfo(const QGeoPositionInfo& newPositionInfo)
{
    if (m_positionInfo == newPositionInfo) {
        return;
    }
    m_positionInfo = newPositionInfo;

    auto ownCoordinate = Positioning::PositionProvider::lastValidCoordinate();
    auto trafficCoordinate = m_positionInfo.coordinate();

    /* Notifier signals */
#warning too many emissions here
    emit climbRateChanged();
    emit coordinateChanged();
    emit groundSpeedChanged();
    emit hDistChanged();
    emit positionInfoChanged();
    emit ttChanged();
    emit vDistChanged();

}


void Traffic::TrafficFactor::updateDescription()
{
    QStringList results;

    if (!callSign().isEmpty()) {
        results << callSign();
    }

    switch(type()) {
    case TrafficFactor_Abstract::Aircraft:
        results << tr("Aircraft");
        break;
    case TrafficFactor_Abstract::Airship:
        results << tr("Airship");
        break;
    case TrafficFactor_Abstract::Balloon:
        results << tr("Balloon");
        break;
    case TrafficFactor_Abstract::Copter:
        results << tr("Copter");
        break;
    case TrafficFactor_Abstract::Drone:
        results << tr("Drone");
        break;
    case TrafficFactor_Abstract::Glider:
        results << tr("Glider");
        break;
    case TrafficFactor_Abstract::HangGlider:
        results << tr("Hang glider");
        break;
    case TrafficFactor_Abstract::Jet:
        results << tr("Jet");
        break;
    case TrafficFactor_Abstract::Paraglider:
        results << tr("Paraglider");
        break;
    case TrafficFactor_Abstract::Skydiver:
        results << tr("Skydiver");
        break;
    case TrafficFactor_Abstract::StaticObstacle:
        results << tr("Static Obstacle");
        break;
    case TrafficFactor_Abstract::TowPlane:
        results << tr("Tow Plane");
        break;
    default:
        results << tr("Traffic");
        break;
    }

    if (!positionInfo().coordinate().isValid()) {
        results << tr("Position unknown");
    }

    if (vDist().isFinite()) {
        auto result = vDist().toString(Settings::useMetricUnitsStatic(), true, true);
        auto climbRateMPS = climbRate().toMPS();
        if ( qIsFinite(climbRateMPS) ) {
            if (climbRateMPS < -1.0) {
                result += " ↘";
            }
            if ((climbRateMPS >= -1.0) && (climbRateMPS <= +1.0)) {
                result += " →";
            }
            if (climbRateMPS > 1.0) {
                result += " ↗";
            }
        }
        results << result;
    }

    // Set property value
    auto newDescription = results.join(u"<br>");
    if (m_description == newDescription) {
        return;
    }
    m_description = newDescription;
    emit descriptionChanged();

}


void Traffic::TrafficFactor::updateIcon()
{
    // BaseType
    QString baseType = QStringLiteral("noDirection");
    if (m_positionInfo.hasAttribute(QGeoPositionInfo::GroundSpeed) && m_positionInfo.hasAttribute(QGeoPositionInfo::Direction)) {
        auto GS = AviationUnits::Speed::fromMPS( m_positionInfo.attribute(QGeoPositionInfo::GroundSpeed) );
        if (GS.isFinite() && (GS.toKN() > 4)) {
            baseType = QStringLiteral("withDirection");
        }
    }

    auto newIcon = "/icons/traffic-"+baseType+"-"+color()+".svg";
    if (m_icon == newIcon) {
        return;
    }
    m_icon = newIcon;
    emit iconChanged();
}


void Traffic::TrafficFactor::updateValid()
{

    if (!positionInfo().isValid()) {
        if (m_valid) {
            m_valid = false;
            emit validChanged();
        }
        return;
    }

    TrafficFactor_Abstract::updateValid();

}
