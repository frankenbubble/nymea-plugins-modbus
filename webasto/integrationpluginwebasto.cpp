/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2023, nymea GmbH
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

#include "integrationpluginwebasto.h"
#include "webastodiscovery.h"
#include "plugininfo.h"

#include <types/param.h>
#include <hardware/electricity.h>
#include <network/networkdevicediscovery.h>

#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QNetworkInterface>


IntegrationPluginWebasto::IntegrationPluginWebasto()
{
}

void IntegrationPluginWebasto::init()
{

}

void IntegrationPluginWebasto::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
        qCWarning(dcWebasto()) << "Failed to discover network devices. The network device discovery is not available.";
        info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The discovery is not available."));
        return;
    }

    if (info->thingClassId() == webastoLiveThingClassId) {
        qCInfo(dcWebasto()) << "Start discovering webasto live in the local network...";
        NetworkDeviceDiscoveryReply *discoveryReply = hardwareManager()->networkDeviceDiscovery()->discover();
        connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
        connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished, this, [=](){
            ThingDescriptors descriptors;
            qCDebug(dcWebasto()) << "Discovery finished. Found" << discoveryReply->networkDeviceInfos().count() << "devices";
            foreach (const NetworkDeviceInfo &networkDeviceInfo, discoveryReply->networkDeviceInfos()) {
                qCDebug(dcWebasto()) << networkDeviceInfo;
                if (!networkDeviceInfo.hostName().contains("webasto", Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                QString title = "Webasto Live";
                if (networkDeviceInfo.hostName().isEmpty()) {
                    title += networkDeviceInfo.address().toString();
                } else {
                    title += networkDeviceInfo.address().toString() + " (" + networkDeviceInfo.hostName() + ")";
                }

                QString description;
                if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                    description = networkDeviceInfo.macAddress();
                } else {
                    description = networkDeviceInfo.macAddress() + " (" + networkDeviceInfo.macAddressManufacturer() + ")";
                }

                ThingDescriptor descriptor(webastoLiveThingClassId, title, description);

                // Check if we already have set up this device
                Things existingThings = myThings().filterByParam(webastoLiveThingIpAddressParamTypeId, networkDeviceInfo.address().toString());
                if (existingThings.count() == 1) {
                    qCDebug(dcWebasto()) << "This thing already exists in the system." << existingThings.first() << networkDeviceInfo;
                    descriptor.setThingId(existingThings.first()->id());
                }

                ParamList params;
                params << Param(webastoLiveThingIpAddressParamTypeId, networkDeviceInfo.address().toString());
                params << Param(webastoLiveThingMacAddressParamTypeId, networkDeviceInfo.macAddress());
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
            info->finish(Thing::ThingErrorNoError);
        });

        return;
    }


    if (info->thingClassId() == webastoNextThingClassId) {

        qCInfo(dcWebasto()) << "Start discovering Webasto NEXT in the local network...";

        // Create a discovery with the info as parent for auto deleting the object once the discovery info is done
        WebastoDiscovery *discovery = new WebastoDiscovery(hardwareManager()->networkDeviceDiscovery(), info);
        connect(discovery, &WebastoDiscovery::discoveryFinished, info, [=](){
            foreach (const WebastoDiscovery::Result &result, discovery->results()) {

                QString title = "Webasto Next";
                if (!result.networkDeviceInfo.hostName().isEmpty()){
                    title.append(" (" + result.networkDeviceInfo.hostName() + ")");
                }

                QString description = result.networkDeviceInfo.address().toString();
                if (result.networkDeviceInfo.macAddressManufacturer().isEmpty()) {
                    description += " " + result.networkDeviceInfo.macAddress();
                } else {
                    description += " " + result.networkDeviceInfo.macAddress() + " (" + result.networkDeviceInfo.macAddressManufacturer() + ")";
                }

                ThingDescriptor descriptor(webastoNextThingClassId, title, description);

                // Check if we already have set up this device
                Things existingThings = myThings().filterByParam(webastoNextThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
                if (existingThings.count() == 1) {
                    qCDebug(dcWebasto()) << "This thing already exists in the system." << existingThings.first() << result.networkDeviceInfo;
                    descriptor.setThingId(existingThings.first()->id());
                }

                ParamList params;
                params << Param(webastoNextThingMacAddressParamTypeId, result.networkDeviceInfo.macAddress());
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }

            info->finish(Thing::ThingErrorNoError);
        });

        discovery->startDiscovery();
        return;
    }

    Q_ASSERT_X(false, "discoverThings", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
}

void IntegrationPluginWebasto::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcWebasto()) << "Setup thing" << thing->name();

    if (thing->thingClassId() == webastoLiveThingClassId) {

        if (m_webastoLiveConnections.contains(thing)) {
            // Clean up after reconfiguration
            m_webastoLiveConnections.take(thing)->deleteLater();
        }

        QHostAddress address = QHostAddress(thing->paramValue(webastoLiveThingIpAddressParamTypeId).toString());
        Webasto *webasto = new Webasto(address, 502, thing);
        m_webastoLiveConnections.insert(thing, webasto);
        connect(webasto, &Webasto::destroyed, this, [thing, this] {m_webastoLiveConnections.remove(thing);});
        connect(webasto, &Webasto::connectionStateChanged, this, &IntegrationPluginWebasto::onConnectionChanged);
        connect(webasto, &Webasto::receivedRegister, this, &IntegrationPluginWebasto::onReceivedRegister);
        connect(webasto, &Webasto::writeRequestError, this, &IntegrationPluginWebasto::onWriteRequestError);
        connect(webasto, &Webasto::writeRequestExecuted, this, &IntegrationPluginWebasto::onWriteRequestExecuted);
        if (!webasto->connectDevice()) {
            qCWarning(dcWebasto()) << "Could not connect to device";
            info->finish(Thing::ThingErrorSetupFailed);
        }
        connect(webasto, &Webasto::connectionStateChanged, info, [info] (bool connected) {
            if (connected)
                info->finish(Thing::ThingErrorNoError);
        });

        return;
    }

    if (thing->thingClassId() == webastoNextThingClassId) {

        // Handle reconfigure
        if (m_webastoNextConnections.contains(thing)) {
            qCDebug(dcWebasto()) << "Reconfiguring existing thing" << thing->name();
            m_webastoNextConnections.take(thing)->deleteLater();

            if (m_monitors.contains(thing)) {
                hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
            }
        }

        MacAddress macAddress = MacAddress(thing->paramValue(webastoNextThingMacAddressParamTypeId).toString());
        if (!macAddress.isValid()) {
            qCWarning(dcWebasto()) << "The configured mac address is not valid" << thing->params();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The MAC address is not known. Please reconfigure the thing."));
            return;
        }

        // Create the monitor
        NetworkDeviceMonitor *monitor = hardwareManager()->networkDeviceDiscovery()->registerMonitor(macAddress);
        m_monitors.insert(thing, monitor);

        QHostAddress address = monitor->networkDeviceInfo().address();
        if (address.isNull()) {
            qCWarning(dcWebasto()) << "Cannot set up thing. The host address is not known yet. Maybe it will be available in the next run...";
            hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("The host address is not known yet. Trying later again."));
            return;
        }

        // Clean up in case the setup gets aborted
        connect(info, &ThingSetupInfo::aborted, monitor, [=](){
            if (m_monitors.contains(thing)) {
                qCDebug(dcWebasto()) << "Unregister monitor because setup has been aborted.";
                hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));
            }
        });

        // If this is the first setup, the monitor must become reachable before we finish the setup
        if (info->isInitialSetup()) {
            // Wait for the monitor to be ready
            if (monitor->reachable()) {
                // Thing already reachable...let's continue with the setup
                setupWebastoNextConnection(info);
            } else {
                qCDebug(dcWebasto()) << "Waiting for the network monitor to get reachable before continue to set up the connection" << thing->name() << address.toString() << "...";
                connect(monitor, &NetworkDeviceMonitor::reachableChanged, info, [=](bool reachable){
                    if (reachable) {
                        qCDebug(dcWebasto()) << "The monitor for thing setup" << thing->name() << "is now reachable. Continue setup...";
                        setupWebastoNextConnection(info);
                    }
                });
            }
        } else {
            // Not the first setup, just add and let the monitor do the check reachable work
            setupWebastoNextConnection(info);
        }

        return;
    }

    Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());

}

void IntegrationPluginWebasto::postSetupThing(Thing *thing)
{
    qCDebug(dcWebasto()) << "Post setup thing" << thing->name();
    if (!m_pluginTimer) {
        qCDebug(dcWebasto()) << "Setting up refresh timer for Webasto connections";
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this] {

            foreach(Webasto *connection, m_webastoLiveConnections) {
                if (connection->connected()) {
                    update(connection);
                }
            }

            foreach(WebastoNextModbusTcpConnection *webastoNext, m_webastoNextConnections) {
                if (webastoNext->reachable()) {
                    webastoNext->update();
                }
            }
        });

        m_pluginTimer->start();
    }

    if (thing->thingClassId() == webastoLiveThingClassId) {
        Webasto *connection = m_webastoLiveConnections.value(thing);
        update(connection);
        return;
    }

    if (thing->thingClassId() == webastoNextThingClassId) {
        WebastoNextModbusTcpConnection *connection = m_webastoNextConnections.value(thing);
        if (connection->reachable()) {
            thing->setStateValue(webastoNextConnectedStateTypeId, true);
            connection->update();
        } else {
            // We start the connection mechanism only if the monitor says the thing is reachable
            if (m_monitors.value(thing)->reachable()) {
                connection->connectDevice();
            }
        }
        return;
    }
}

void IntegrationPluginWebasto::thingRemoved(Thing *thing)
{
    qCDebug(dcWebasto()) << "Delete thing" << thing->name();

    if (thing->thingClassId() == webastoNextThingClassId) {
        WebastoNextModbusTcpConnection *connection = m_webastoNextConnections.take(thing);
        connection->disconnectDevice();
        connection->deleteLater();
    }

    // Unregister related hardware resources
    if (m_monitors.contains(thing))
        hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(m_monitors.take(thing));

    if (m_pluginTimer && myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginWebasto::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == webastoLiveThingClassId) {

        Webasto *connection = m_webastoLiveConnections.value(thing);
        if (!connection) {
            qCWarning(dcWebasto()) << "Can't find connection to thing";
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        if (action.actionTypeId() == webastoLivePowerActionTypeId) {
            bool enabled = action.paramValue(webastoLivePowerActionPowerParamTypeId).toBool();
            thing->setStateValue(webastoLivePowerActionTypeId, enabled);
            int ampere = 0;
            if (enabled) {
                ampere = thing->stateValue(webastoLiveMaxChargingCurrentStateTypeId).toUInt();
            }
            QUuid requestId = connection->setChargeCurrent(ampere);
            if (requestId.isNull()) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                m_asyncActions.insert(requestId, info);
            }
        } else if (action.actionTypeId() == webastoLiveMaxChargingCurrentActionTypeId) {
            int ampere = action.paramValue(webastoLiveMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            thing->setStateValue(webastoLiveMaxChargingCurrentStateTypeId, ampere);
            QUuid requestId = connection->setChargeCurrent(ampere);
            if (requestId.isNull()) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                m_asyncActions.insert(requestId, info);
            }
        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }

        return;
    }

    if (thing->thingClassId() == webastoNextThingClassId) {

        WebastoNextModbusTcpConnection *connection = m_webastoNextConnections.value(thing);
        if (!connection) {
            qCWarning(dcWebasto()) << "Can't find modbus connection for" << thing;
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        if (!connection->reachable()) {
            qCWarning(dcWebasto()) << "Cannot execute action because the connection of" << thing << "is not reachable.";
            info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("The charging station is not reachable."));
            return;
        }

        if (action.actionTypeId() == webastoNextPowerActionTypeId) {
            bool power = action.paramValue(webastoNextPowerActionPowerParamTypeId).toBool();

            // If this action was executed by the user, we start a new session, otherwise we assume it was a some charging logic
            // and we keep the current session.

            if (power && action.triggeredBy() == Action::TriggeredByUser) {
                // First send 0 ChargingActionNoAction before sending 1 start session
                qCDebug(dcWebasto()) << "Enable charging action triggered by user. Restarting the session.";
                QModbusReply *reply = connection->setChargingAction(WebastoNextModbusTcpConnection::ChargingActionNoAction);
                connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
                connect(reply, &QModbusReply::finished, info, [this, info, reply, power](){
                    if (reply->error() == QModbusDevice::NoError) {
                        info->thing()->setStateValue(webastoNextPowerStateTypeId, power);
                        qCDebug(dcWebasto()) << "Restart charging session request finished successfully.";
                        info->finish(Thing::ThingErrorNoError);
                    } else {
                        qCWarning(dcWebasto()) << "Restart charging session request finished with error:" << reply->errorString();
                        info->finish(Thing::ThingErrorHardwareFailure);
                    }

                    // Note: even if "NoAction" failed, we try to send the start charging action and report the error there just in case
                    executeWebastoNextPowerAction(info, power);
                });
            } else {
                executeWebastoNextPowerAction(info, power);
            }
        } else if (action.actionTypeId() == webastoNextMaxChargingCurrentActionTypeId) {
            quint16 chargingCurrent = action.paramValue(webastoNextMaxChargingCurrentActionMaxChargingCurrentParamTypeId).toUInt();
            qCDebug(dcWebasto()) << "Set max charging current of" << thing << "to" << chargingCurrent << "ampere";
            QModbusReply *reply = connection->setChargeCurrent(chargingCurrent);
            connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
            connect(reply, &QModbusReply::finished, info, [info, reply, chargingCurrent](){
                if (reply->error() == QModbusDevice::NoError) {
                    qCDebug(dcWebasto()) << "Set max charging current finished successfully.";
                    info->thing()->setStateValue(webastoNextMaxChargingCurrentStateTypeId, chargingCurrent);
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    qCWarning(dcWebasto()) << "Set max charging current request finished with error:" << reply->errorString();
                    info->finish(Thing::ThingErrorHardwareFailure);
                }
            });

        } else {
            Q_ASSERT_X(false, "executeAction", QString("Unhandled actionTypeId: %1").arg(action.actionTypeId().toString()).toUtf8());
        }

        return;
    }

    Q_ASSERT_X(false, "executeAction", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
}


void IntegrationPluginWebasto::setupWebastoNextConnection(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QHostAddress address = m_monitors.value(thing)->networkDeviceInfo().address();
    uint port = thing->paramValue(webastoNextThingPortParamTypeId).toUInt();
    quint16 slaveId = thing->paramValue(webastoNextThingSlaveIdParamTypeId).toUInt();

    qCDebug(dcWebasto()) << "Setting up webasto next connection on" << QString("%1:%2").arg(address.toString()).arg(port) << "slave ID:" << slaveId;
    WebastoNextModbusTcpConnection *webastoNextConnection = new WebastoNextModbusTcpConnection(address, port, slaveId, this);
    webastoNextConnection->modbusTcpMaster()->setTimeout(500);
    webastoNextConnection->modbusTcpMaster()->setNumberOfRetries(3);
    m_webastoNextConnections.insert(thing, webastoNextConnection);
    connect(info, &ThingSetupInfo::aborted, webastoNextConnection, [=](){
        webastoNextConnection->deleteLater();
        m_webastoNextConnections.remove(thing);
    });

    // Reconnect on monitor reachable changed
    NetworkDeviceMonitor *monitor = m_monitors.value(thing);
    connect(monitor, &NetworkDeviceMonitor::reachableChanged, thing, [=](bool reachable){

        if (reachable) {
            qCDebug(dcWebasto()) << "Network device is now reachable for" << thing << monitor->networkDeviceInfo();
        } else {
            qCDebug(dcWebasto()) << "Network device not reachable any more" << thing;
        }

        if (!thing->setupComplete())
            return;

        if (reachable) {
            webastoNextConnection->modbusTcpMaster()->setHostAddress(monitor->networkDeviceInfo().address());
            webastoNextConnection->reconnectDevice();
        } else {
            // Note: We disable autoreconnect explicitly and we will
            // connect the device once the monitor says it is reachable again
            webastoNextConnection->disconnectDevice();
        }
    });

    connect(webastoNextConnection, &WebastoNextModbusTcpConnection::reachableChanged, thing, [thing, webastoNextConnection, monitor](bool reachable){
        qCDebug(dcWebasto()) << "Reachable changed to" << reachable << "for" << thing;
        thing->setStateValue(webastoNextConnectedStateTypeId, reachable);
        if (reachable) {
            // Connected true will be set after successfull init
            webastoNextConnection->update();
        } else {
            thing->setStateValue(webastoNextCurrentPowerStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPowerPhaseAStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPowerPhaseBStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPowerPhaseCStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPhaseAStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPhaseBStateTypeId, 0);
            thing->setStateValue(webastoNextCurrentPhaseCStateTypeId, 0);

            if (monitor->reachable()) {
                webastoNextConnection->reconnectDevice();
            }
        }
    });

    connect(webastoNextConnection, &WebastoNextModbusTcpConnection::updateFinished, thing, [thing, webastoNextConnection](){

        // Note: we get the update finished also if all calles failed...
        if (!webastoNextConnection->reachable()) {
            thing->setStateValue(webastoNextConnectedStateTypeId, false);
            return;
        }

        thing->setStateValue(webastoNextConnectedStateTypeId, true);

        qCDebug(dcWebasto()) << "Update finished" << webastoNextConnection;
        // States
        switch (webastoNextConnection->chargeState()) {
        case WebastoNextModbusTcpConnection::ChargeStateIdle:
            thing->setStateValue(webastoNextChargingStateTypeId, false);
            break;
        case WebastoNextModbusTcpConnection::ChargeStateCharging:
            thing->setStateValue(webastoNextChargingStateTypeId, true);
            break;
        }

        switch (webastoNextConnection->chargerState()) {
        case WebastoNextModbusTcpConnection::ChargerStateNoVehicle:
            thing->setStateValue(webastoNextChargingStateTypeId, false);
            thing->setStateValue(webastoNextPluggedInStateTypeId, false);
            break;
        case WebastoNextModbusTcpConnection::ChargerStateVehicleAttachedNoPermission:
            thing->setStateValue(webastoNextPluggedInStateTypeId, true);
            break;
        case WebastoNextModbusTcpConnection::ChargerStateCharging:
            thing->setStateValue(webastoNextChargingStateTypeId, true);
            thing->setStateValue(webastoNextPluggedInStateTypeId, true);
            break;
        case WebastoNextModbusTcpConnection::ChargerStateChargingPaused:
            thing->setStateValue(webastoNextPluggedInStateTypeId, true);
            break;
        default:
            break;
        }

        // Meter values
        thing->setStateValue(webastoNextCurrentPowerPhaseAStateTypeId, webastoNextConnection->activePowerL1());
        thing->setStateValue(webastoNextCurrentPowerPhaseBStateTypeId, webastoNextConnection->activePowerL2());
        thing->setStateValue(webastoNextCurrentPowerPhaseCStateTypeId, webastoNextConnection->activePowerL3());

        double currentPhaseA = webastoNextConnection->currentL1() / 1000.0;
        double currentPhaseB = webastoNextConnection->currentL2() / 1000.0;
        double currentPhaseC = webastoNextConnection->currentL3() / 1000.0;
        thing->setStateValue(webastoNextCurrentPhaseAStateTypeId, currentPhaseA);
        thing->setStateValue(webastoNextCurrentPhaseBStateTypeId, currentPhaseB);
        thing->setStateValue(webastoNextCurrentPhaseCStateTypeId, currentPhaseC);

        // Note: we do not use the active phase power, because we have sometimes a few watts on inactive phases
        Electricity::Phases phases = Electricity::PhaseNone;
        phases.setFlag(Electricity::PhaseA, currentPhaseA > 0);
        phases.setFlag(Electricity::PhaseB, currentPhaseB > 0);
        phases.setFlag(Electricity::PhaseC, currentPhaseC > 0);
        if (phases != Electricity::PhaseNone) {
            thing->setStateValue(webastoNextUsedPhasesStateTypeId, Electricity::convertPhasesToString(phases));
            thing->setStateValue(webastoNextPhaseCountStateTypeId, Electricity::getPhaseCount(phases));
        }


        thing->setStateValue(webastoNextCurrentPowerStateTypeId, webastoNextConnection->totalActivePower());

        thing->setStateValue(webastoNextTotalEnergyConsumedStateTypeId, webastoNextConnection->energyConsumed() / 1000.0);
        thing->setStateValue(webastoNextSessionEnergyStateTypeId, webastoNextConnection->sessionEnergy() / 1000.0);

        // Min / Max charging current^
        thing->setStateValue(webastoNextMinCurrentTotalStateTypeId, webastoNextConnection->minChargingCurrent());
        thing->setStateValue(webastoNextMaxCurrentTotalStateTypeId, webastoNextConnection->maxChargingCurrent());
        thing->setStateMinValue(webastoNextMaxChargingCurrentStateTypeId, webastoNextConnection->minChargingCurrent());
        thing->setStateMaxValue(webastoNextMaxChargingCurrentStateTypeId, webastoNextConnection->maxChargingCurrent());

        thing->setStateValue(webastoNextMaxCurrentChargerStateTypeId, webastoNextConnection->maxChargingCurrentStation());
        thing->setStateValue(webastoNextMaxCurrentCableStateTypeId, webastoNextConnection->maxChargingCurrentCable());
        thing->setStateValue(webastoNextMaxCurrentElectricVehicleStateTypeId, webastoNextConnection->maxChargingCurrentEv());

        if (webastoNextConnection->evseErrorCode() == 0) {
            thing->setStateValue(webastoNextErrorStateTypeId, "");
        } else {
            uint errorCode = webastoNextConnection->evseErrorCode() - 1;
            switch (errorCode) {
            case 1:
                // Note: also PB61 has the same mapping and the same reason for the error.
                // We inform only about the PB02 since it does not make any difference regarding the action
                thing->setStateValue(webastoNextErrorStateTypeId, "PB02 - PowerSwitch Failure");
                break;
            case 2:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB07 - InternalError (Aux Voltage)");
                break;
            case 3:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB09 - EV Communication Error");
                break;
            case 4:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB17 - OverVoltage");
                break;
            case 5:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB18 - UnderVoltage");
                break;
            case 6:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB23 - OverCurrent Failure");
                break;
            case 7:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB24 - OtherError");
                break;
            case 8:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB27 - GroundFailure");
                break;
            case 9:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB28 - InternalError (Selftest)");
                break;
            case 10:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB29 - High Temperature");
                break;
            case 11:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB52 - Proximity Pilot Error");
                break;
            case 12:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB53 - Shutter Error");
                break;
            case 13:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB57 - Error Three Phase Check");
                break;
            case 14:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB59 - PWR internal error");
                break;
            case 15:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB60 - EV Communication Error - Negative control pilot voltage");
                break;
            case 16:
                thing->setStateValue(webastoNextErrorStateTypeId, "PB62- DC residual current (Vehicle)");
                break;
            default:
                thing->setStateValue(webastoNextErrorStateTypeId, QString("Unknwon error code %1").arg(errorCode));
                break;
            }
        }

        // Handle life bit (keep alive mechanism if there is a HEMS activated)
        if (webastoNextConnection->lifeBit() == 0) {
            // Let's reset the life bit so the wallbox knows we are still here,
            // otherwise the wallbox goes into the failsave mode and limits the charging to the configured
            QModbusReply *reply = webastoNextConnection->setLifeBit(1);
            connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
            connect(reply, &QModbusReply::finished, webastoNextConnection, [reply, webastoNextConnection](){
                if (reply->error() == QModbusDevice::NoError) {
                    qCDebug(dcWebasto()) << "Resetted life bit watchdog on" << webastoNextConnection << "finished successfully";
                } else {
                    qCWarning(dcWebasto()) << "Resetted life bit watchdog on" << webastoNextConnection << "finished with error:" << reply->errorString();
                }
            });
        }
    });

    connect(thing, &Thing::settingChanged, webastoNextConnection, [webastoNextConnection](const ParamTypeId &paramTypeId, const QVariant &value){
        if (paramTypeId == webastoNextSettingsCommunicationTimeoutParamTypeId) {
            QModbusReply *reply = webastoNextConnection->setComTimeout(value.toUInt());
            connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
            connect(reply, &QModbusReply::finished, webastoNextConnection, [reply, webastoNextConnection, value](){
                if (reply->error() == QModbusDevice::NoError) {
                    qCDebug(dcWebasto()) << "Setting communication timout to" << value.toUInt() << "on" << webastoNextConnection << "finished successfully.";
                } else {
                    qCWarning(dcWebasto()) << "Setting communication timout to" << value.toUInt() << "on" << webastoNextConnection << "finished with error:" << reply->errorString();
                    if (webastoNextConnection->reachable()) {
                        webastoNextConnection->updateComTimeout();
                    }
                }
            });
        } else if (paramTypeId == webastoNextSettingsSafeCurrentParamTypeId) {
            QModbusReply *reply = webastoNextConnection->setSafeCurrent(value.toUInt());
            connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
            connect(reply, &QModbusReply::finished, webastoNextConnection, [reply, webastoNextConnection, value](){
                if (reply->error() == QModbusDevice::NoError) {
                    qCDebug(dcWebasto()) << "Setting save current to" << value.toUInt() << "on" << webastoNextConnection << "finished successfully.";
                } else {
                    qCWarning(dcWebasto()) << "Setting save current to" << value.toUInt() << "on" << webastoNextConnection << "finished with error:" << reply->errorString();
                    if (webastoNextConnection->reachable()) {
                        webastoNextConnection->updateSafeCurrent();
                    }
                }
            });
        } else {
            qCWarning(dcWebasto()) << "Unhandled setting changed for" << webastoNextConnection;
        }
    });

    connect(webastoNextConnection, &WebastoNextModbusTcpConnection::comTimeoutChanged, thing, [thing](quint16 comTimeout){
        thing->setSettingValue(webastoNextSettingsCommunicationTimeoutParamTypeId, comTimeout);
    });

    connect(webastoNextConnection, &WebastoNextModbusTcpConnection::safeCurrentChanged, thing, [thing](quint16 safeCurrent){
        thing->setSettingValue(webastoNextSettingsSafeCurrentParamTypeId, safeCurrent);
    });

    qCInfo(dcWebasto()) << "Setup finished successfully for Webasto NEXT" << thing << monitor;
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginWebasto::update(Webasto *webasto)
{
    webasto->getRegister(Webasto::TqChargePointState);
    webasto->getRegister(Webasto::TqCableState);
    webasto->getRegister(Webasto::TqEVSEError);

    webasto->getRegister(Webasto::TqCurrentL1);
    webasto->getRegister(Webasto::TqCurrentL2);
    webasto->getRegister(Webasto::TqCurrentL3);

    webasto->getRegister(Webasto::TqActivePower, 2);
    webasto->getRegister(Webasto::TqEnergyMeter, 2);

    webasto->getRegister(Webasto::TqMaxCurrent);

    webasto->getRegister(Webasto::TqChargedEnergy);
    webasto->getRegister(Webasto::TqChargingTime, 2);

    webasto->getRegister(Webasto::TqUserId, 10);
}

void IntegrationPluginWebasto::evaluatePhaseCount(Thing *thing)
{
    uint amperePhase1 = thing->stateValue(webastoLiveCurrentPhase1StateTypeId).toUInt();
    uint amperePhase2 = thing->stateValue(webastoLiveCurrentPhase2StateTypeId).toUInt();
    uint amperePhase3 = thing->stateValue(webastoLiveCurrentPhase3StateTypeId).toUInt();
    // Check how many phases are actually charging, and update the phase count only if something happens on the phases (current or power)
    if (!(amperePhase1 == 0 && amperePhase2 == 0 && amperePhase3 == 0)) {
        uint phaseCount = 0;
        if (amperePhase1 != 0)
            phaseCount += 1;

        if (amperePhase2 != 0)
            phaseCount += 1;

        if (amperePhase3 != 0)
            phaseCount += 1;

        thing->setStateValue(webastoLivePhaseCountStateTypeId, phaseCount);
    }
}

void IntegrationPluginWebasto::executeWebastoNextPowerAction(ThingActionInfo *info, bool power)
{
    qCDebug(dcWebasto()) << (power ? "Enabling": "Disabling") << "charging on" << info->thing();

    WebastoNextModbusTcpConnection *connection = m_webastoNextConnections.value(info->thing());
    QModbusReply *reply = nullptr;
    if (power) {
        reply = connection->setChargingAction(WebastoNextModbusTcpConnection::ChargingActionStartSession);
    } else {
        reply = connection->setChargingAction(WebastoNextModbusTcpConnection::ChargingActionCancelSession);
    }

    connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
    connect(reply, &QModbusReply::finished, info, [info, reply, power](){
        if (reply->error() == QModbusDevice::NoError) {
            info->thing()->setStateValue(webastoNextPowerStateTypeId, power);
            qCDebug(dcWebasto()) << "Enabling/disabling charging request finished successfully.";
            info->finish(Thing::ThingErrorNoError);
        } else {
            qCWarning(dcWebasto()) << "Enabling/disabling charging request finished with error:" << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    });
}

void IntegrationPluginWebasto::onConnectionChanged(bool connected)
{
    Webasto *connection = static_cast<Webasto *>(sender());
    Thing *thing = m_webastoLiveConnections.key(connection);
    if (!thing) {
        qCWarning(dcWebasto()) << "On connection changed, thing not found for connection";
        return;
    }
    thing->setStateValue(webastoLiveConnectedStateTypeId, connected);
}

void IntegrationPluginWebasto::onWriteRequestExecuted(const QUuid &requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    }
}

void IntegrationPluginWebasto::onWriteRequestError(const QUuid &requestId, const QString &error)
{
    Q_UNUSED(requestId);
    qCWarning(dcWebasto()) << "Write request error" << error;
}

void IntegrationPluginWebasto::onReceivedRegister(Webasto::TqModbusRegister modbusRegister, const QVector<quint16> &data)
{
    Webasto *connection = static_cast<Webasto *>(sender());
    Thing *thing = m_webastoLiveConnections.key(connection);
    if (!thing) {
        qCWarning(dcWebasto()) << "On basic information received, thing not found for connection";
        return;
    }
    if (thing->thingClassId() == webastoLiveThingClassId) {
        switch (modbusRegister) {
        case Webasto::TqChargePointState:
            qCDebug(dcWebasto()) << "   - Charge point state:" << Webasto::ChargePointState(data[0]);
            switch (Webasto::ChargePointState(data[0])) {
            case Webasto::ChargePointStateNoVehicleAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "No vehicle attached");
                break;
            case Webasto::ChargePointStateVehicleAttachedNoPermission:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Vehicle attached, no permission");
                break;
            case Webasto::ChargePointStateChargingAuthorized:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charging authorized");
                break;
            case Webasto::ChargePointStateCharging:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charging");
                break;
            case Webasto::ChargePointStateChargingPaused:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charging paused");
                break;
            case Webasto::ChargePointStateChargeSuccessfulCarStillAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charge successful (car still attached)");
                break;
            case Webasto::ChargePointStateChargingStoppedByUserCarStillAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charging stopped by user (car still attached)");
                break;
            case Webasto::ChargePointStateChargingErrorCarStillAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId,  "Charging error (car still attached)");
                break;
            case Webasto::ChargePointStateChargingStationReservedNorCarAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "Charging station reserved (No car attached)");
                break;
            case Webasto::ChargePointStateUserNotAuthorizedCarAttached:
                thing->setStateValue(webastoLiveChargePointStateStateTypeId, "User not authorized (car attached)");
                break;
            }

            thing->setStateValue(webastoLiveChargingStateTypeId, Webasto::ChargePointState(data[0]) == Webasto::ChargePointStateCharging);
            break;
        case Webasto::TqChargeState:
            qCDebug(dcWebasto()) << "   - Charge state:" << data[0];
            break;
        case Webasto::TqEVSEState:
            qCDebug(dcWebasto()) << "   - EVSE state:" << data[0];
            break;
        case Webasto::TqCableState:
            qCDebug(dcWebasto()) << "   - Cable state:" << Webasto::CableState(data[0]);
            switch (Webasto::CableState(data[0])) {
            case Webasto::CableStateNoCableAttached:
                thing->setStateValue(webastoLiveCableStateStateTypeId, "No cable attached");
                thing->setStateValue(webastoLivePluggedInStateTypeId, false);
                break;
            case Webasto::CableStateCableAttachedNoCarAttached:
                thing->setStateValue(webastoLiveCableStateStateTypeId, "Cable attached but no car attached)");
                thing->setStateValue(webastoLivePluggedInStateTypeId, false);
                break;
            case Webasto::CableStateCableAttachedCarAttached:
                thing->setStateValue(webastoLiveCableStateStateTypeId, "Cable attached and car attached");
                thing->setStateValue(webastoLivePluggedInStateTypeId, true);
                break;
            case Webasto::CableStateCableAttachedCarAttachedLockActive:
                thing->setStateValue(webastoLiveCableStateStateTypeId, "Cable attached, car attached and lock active");
                thing->setStateValue(webastoLivePluggedInStateTypeId, true);
                break;
            }
            break;
        case Webasto::TqEVSEError:
            qCDebug(dcWebasto()) << "   - EVSE error:" << data[0];
            thing->setStateValue(webastoLiveErrorStateTypeId, data[0]);
            break;
        case Webasto::TqCurrentL1:
            qCDebug(dcWebasto()) << "   - Current L1:" << data[0];
            thing->setStateValue(webastoLiveCurrentPhase1StateTypeId, data[0]);
            evaluatePhaseCount(thing);
            break;
        case Webasto::TqCurrentL2:
            qCDebug(dcWebasto()) << "   - Current L2:" << data[0];
            thing->setStateValue(webastoLiveCurrentPhase2StateTypeId, data[0]);
            evaluatePhaseCount(thing);
            break;
        case Webasto::TqCurrentL3:
            qCDebug(dcWebasto()) << "   - Current L3:" << data[0];
            thing->setStateValue(webastoLiveCurrentPhase3StateTypeId, data[0]);
            evaluatePhaseCount(thing);
            break;
        case Webasto::TqActivePower: {
            if (data.count() < 2)
                return;
            int power = (static_cast<quint32>(data[0])<<16 | data[1]);
            qCDebug(dcWebasto()) << "   - Active power:" << power;
            thing->setStateValue(webastoLiveCurrentPowerStateTypeId, power);
        } break;
        case Webasto::TqEnergyMeter: {
            if (data.count() < 2)
                return;
            int energy = (static_cast<quint32>(data[0])<<16 | data[1]);
            qCDebug(dcWebasto()) << "   - Energy meter:" << energy << "Wh";
            thing->setStateValue(webastoLiveTotalEnergyConsumedStateTypeId, energy);
        } break;
        case Webasto::TqMaxCurrent:
            qCDebug(dcWebasto()) << "   - Max. Current" << data[0];
            thing->setStateValue(webastoLiveMaxPossibleChargingCurrentStateTypeId, data[0]);
            break;
        case Webasto::TqMinimumCurrentLimit:
            qCDebug(dcWebasto()) << "   - Min. Current" << data[0];
            break;
        case Webasto::TqMaxCurrentFromEVSE:
            qCDebug(dcWebasto()) << "   - Max. Current EVSE" << data[0];
            break;
        case Webasto::TqMaxCurrentFromCable:
            qCDebug(dcWebasto()) << "   - Max. Current Cable" << data[0];
            break;
        case Webasto::TqMaxCurrentFromEV:
            qCDebug(dcWebasto()) << "   - Max. Current EV" << data[0];
            break;
        case Webasto::TqUserPriority:
            qCDebug(dcWebasto()) << "   - User priority" << data[0];
            break;
        case Webasto::TqEVBatteryState:
            qCDebug(dcWebasto()) << "   - Battery state" << data[0];
            break;
        case Webasto::TqEVBatteryCapacity: {
            if (data.count() < 2)
                return;
            uint batteryCapacity = (static_cast<quint32>(data[0])<<16 | data[1]);
            qCDebug(dcWebasto()) << "   - Battery capacity" << batteryCapacity << "Wh";
        } break;
        case Webasto::TqScheduleType:
            qCDebug(dcWebasto()) << "   - Schedule type" << data[0];
            break;
        case Webasto::TqRequiredEnergy: {
            if (data.count() < 2)
                return;
            uint requiredEnergy = (static_cast<quint32>(data[0])<<16 | data[1]);
            qCDebug(dcWebasto()) << "   - Required energy" << requiredEnergy;
        } break;
        case Webasto::TqRequiredBatteryState:
            qCDebug(dcWebasto()) << "   - Required battery state" << data[0];
            break;
        case Webasto::TqScheduledTime:
            qCDebug(dcWebasto()) << "   - Scheduled time" << data[0];
            break;
        case Webasto::TqScheduledDate:
            qCDebug(dcWebasto()) << "   - Scheduled date" << data[0];
            break;
        case Webasto::TqChargedEnergy:
            qCDebug(dcWebasto()) << "   - Charged energy" << data[0];
            thing->setStateValue(webastoLiveSessionEnergyStateTypeId, data[0]/1000.00); // Charged energy in kWh
            break;
        case Webasto::TqStartTime:
            qCDebug(dcWebasto()) << "   - Start time" << (static_cast<quint32>(data[0])<<16 | data[1]);
            break;
        case Webasto::TqChargingTime: {
            if (data.count() < 2)
                return;
            uint seconds = (static_cast<quint32>(data[0])<<16 | data[1]);
            qCDebug(dcWebasto()) << "   - Charging time" << seconds << "s";
            thing->setStateValue(webastoLiveSessionTimeStateTypeId, seconds/60.00); // Charging time in minutes
        } break;
        case Webasto::TqEndTime: {
            if (data.count() < 2)
                return;
            uint hour =    ((static_cast<quint32>(data[0])<<16 | data[1])&0xff0000)>>16;
            uint minutes = ((static_cast<quint32>(data[0])<<16 | data[1])&0x00ff00)>>8;
            uint seconds=  (static_cast<quint32>(data[0])<<16 | data[1])&0x0000ff;
            qCDebug(dcWebasto()) << "   - End time" << hour << "h" << minutes << "m" << seconds << "s";
        } break;
        case Webasto::TqUserId: {
            if (data.count() < 10)
                return;
            QByteArray userID;
            Q_FOREACH(quint16 i, data) {
                userID.append(i>>16);
                userID.append(i&0xff);
            }
            qCDebug(dcWebasto()) << "   - User ID:" << userID;
        } break;
        case Webasto::TqSmartVehicleDetected:
            qCDebug(dcWebasto()) << "   - Smart vehicle detected:" << data[0];
            break;
        case Webasto::TqSafeCurrent:
            qCDebug(dcWebasto()) << "   - Safe current:" << data[0];
            break;
        case Webasto::TqComTimeout:
            qCDebug(dcWebasto()) << "   - Com timeout:" << data[0];
            break;
        default:
            break;
        }
    }
}
