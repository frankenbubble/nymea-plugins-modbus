/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINMENNEKES_H
#define INTEGRATIONPLUGINMENNEKES_H

#include <plugintimer.h>
#include <integrations/integrationplugin.h>
#include <network/networkdevicemonitor.h>

#include "extern-plugininfo.h"

#include "amtronecumodbustcpconnection.h"
#include "amtronhcc3modbustcpconnection.h"
#include "amtroncompact20modbusrtuconnection.h"

class IntegrationPluginMennekes: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginmennekes.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginMennekes();

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private slots:
    void updateECUPhaseCount(Thing *thing);
    void updateCompact20PhaseCount(Thing *thing);

private:
    void setupAmtronECUConnection(ThingSetupInfo *info);
    void setupAmtronHCC3Connection(ThingSetupInfo *info);
    void setupAmtronCompact20Connection(ThingSetupInfo *info);

    bool ensureAmtronECUVersion(AmtronECUModbusTcpConnection *connection, const QString &version);

    PluginTimer *m_pluginTimer = nullptr;
    QHash<Thing *, AmtronECUModbusTcpConnection *> m_amtronECUConnections;
    QHash<Thing *, AmtronHCC3ModbusTcpConnection *> m_amtronHCC3Connections;
    QHash<Thing *, AmtronCompact20ModbusRtuConnection *> m_amtronCompact20Connections;
    QHash<Thing *, NetworkDeviceMonitor *> m_monitors;

};

#endif // INTEGRATIONPLUGINMENNEKES_H


