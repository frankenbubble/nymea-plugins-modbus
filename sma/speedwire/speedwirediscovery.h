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

#ifndef SPEEDWIREDISCOVERY_H
#define SPEEDWIREDISCOVERY_H

#include <QTimer>
#include <QObject>
#include <QUdpSocket>

#include <network/networkdevicediscovery.h>

#include "speedwire.h"
#include "speedwireinverter.h"
#include "speedwireinterface.h"

class SpeedwireDiscovery : public QObject
{
    Q_OBJECT
public:
    typedef struct SpeedwireDiscoveryResult {
        QHostAddress address;
        NetworkDeviceInfo networkDeviceInfo;
        SpeedwireInterface::DeviceType deviceType = SpeedwireInterface::DeviceTypeUnknown;
        quint16 modelId = 0;
        quint32 serialNumber = 0;
    } SpeedwireDiscoveryResult;

    explicit SpeedwireDiscovery(NetworkDeviceDiscovery *networkDeviceDiscovery, quint32 localSerialNumber, QObject *parent = nullptr);
    ~SpeedwireDiscovery();

    bool initialize(SpeedwireInterface::DeviceType deviceType = SpeedwireInterface::DeviceTypeUnknown);
    bool initialized() const;

    bool startDiscovery();
    bool discoveryRunning() const;

    QList<SpeedwireDiscoveryResult> discoveryResult() const;

signals:
    void discoveryFinished();

private:
    NetworkDeviceDiscovery *m_networkDeviceDiscovery = nullptr;
    SpeedwireInterface::DeviceType m_deviceType = SpeedwireInterface::DeviceTypeUnknown;
    QUdpSocket *m_multicastSocket = nullptr;
    QUdpSocket *m_unicastSocket = nullptr;
    quint32 m_localSerialNumber = 0;
    bool m_initialized = false;

    // Discovery
    QTimer m_multicastSearchRequestTimer;
    NetworkDeviceInfos m_networkDeviceInfos;
    QList<SpeedwireDiscoveryResult> m_results;
    QHash<QHostAddress, SpeedwireDiscoveryResult> m_resultMeters;
    QHash<QHostAddress, SpeedwireDiscoveryResult> m_resultInverters;

    QHash<QHostAddress, SpeedwireInverter *> m_inverters;

    bool m_multicastRunning = false;
    bool m_unicastRunning = false;

    bool setupMulticastSocket();
    void startMulticastDiscovery();

    bool setupUnicastSocket();
    void startUnicastDiscovery();

    void sendUnicastDiscoveryRequest(const QHostAddress &targetHostAddress);

private slots:
    void readPendingDatagramsMulticast();
    void readPendingDatagramsUnicast();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

    void processDatagram(const QHostAddress &senderAddress, quint16 senderPort, const QByteArray &datagram);

    void sendDiscoveryRequest();

    void evaluateDiscoveryFinished();

    void finishDiscovery();

};

#endif // SPEEDWIREDISCOVERY_H
