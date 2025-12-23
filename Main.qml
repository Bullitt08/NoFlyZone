import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning
import NoFlyZone

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    color: "#0f1624"
    title: qsTr("No Fly Zone")

    // Ankara'nın yasak alan merkezi ve yarıçap
    readonly property var zoneCenterCoord: QtPositioning.coordinate(39.9334, 32.8597)
    readonly property double zoneRadiusMeters: 30000

    // OpenSky erişimi
    AircraftModel {
        id: aircraftModel
        zoneCenter: zoneCenterCoord
        zoneRadiusMeters: zoneRadiusMeters
        apiUrl: "https://opensky-network.org/api/states/all"
        tokenUrl: "https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token"
        bboxScale: 6.0
        clientId: "client-id"
        clientSecret: "client-secret"
        refreshIntervalMs: 15000
        onIntrusionDetected: (callsign, icao) => {
            alertText.text = callsign.length ? callsign : icao
            alertPopup.open()
            alertCloser.restart()
        }
    }

    property var selectedAircraft: null

    Plugin {
        id: osmPlugin
        name: "osm"
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: osmPlugin
        center: zoneCenterCoord
        zoomLevel: 9

        // Harita sürükleme
        DragHandler {
            id: panHandler
            target: null
            grabPermissions: PointerHandler.TakeOverForbidden
            property point lastDelta: Qt.point(0, 0)
            onActiveChanged: lastDelta = Qt.point(0, 0)
            onTranslationChanged: {
                const dx = translation.x - lastDelta.x
                const dy = translation.y - lastDelta.y
                if (dx || dy) {
                    map.pan(-dx, -dy)
                    lastDelta = Qt.point(translation.x, translation.y)
                }
            }
        }

        // Üstteki statü barı
        Label {
            id: statusLabel
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 8
            padding: 6
            text: aircraftModel.lastError.length ? "Hata: " + aircraftModel.lastError : "Aktif olarak çalışıyor"
            color: aircraftModel.lastError.length ? "#ffb199" : "#d6ffe0"
            background: Rectangle {
                color: aircraftModel.lastError.length ? "#3b1c1c" : "#1d2a32"
                radius: 6
                border.color: "#506070"
            }
        }
        Column{
            anchors{
                right: map.right
                top: map.top
                rightMargin: 3
                topMargin: 3
            }
            spacing: 3
            // Yenileme butonu
            Button {
                width:40
                height:40
                font.pixelSize: 20
                palette.buttonText: "black"
                text: qsTr("⟳")
                onClicked: aircraftModel.refreshNow()
            }


            //Zoom in butonu
            Button {
                width:40
                height:40
                font.pixelSize: 20
                palette.buttonText: "black"
                text: "+"
                onClicked: map.zoomLevel = Math.min(map.maximumZoomLevel, map.zoomLevel + 1)
            }


            //Zoom out butonu
            Button {
                width:40
                height:40
                font.pixelSize: 20
                palette.buttonText: "black"
                text: "-"
                onClicked: map.zoomLevel = Math.max(map.minimumZoomLevel, map.zoomLevel - 1)
            }

            // Harita resetleme butonu
            Button {
                width:40
                height:40
                font.pixelSize: 20
                palette.buttonText: "black"
                text: qsTr("R")
                onClicked: {
                   map.center = zoneCenterCoord
                   map.zoomLevel = 9
                }
            }
        }

        //No Fly Zone alanı
        MapCircle {
            center: zoneCenterCoord
            radius: zoneRadiusMeters
            color: "#33ff5b5e"
            border.color: "#ff5b5e"
            border.width: 2
        }

        //No Fly Zone merkezi
        MapQuickItem {
            id: zonePin
            coordinate: zoneCenterCoord
            anchorPoint: Qt.point(6, 6)
            sourceItem: Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#ff5b5e"
                border.color: "white"
            }
        }


        // Uçak markerları
        MapItemView {
            model: aircraftModel
            delegate: MapQuickItem {
                id: marker
                coordinate: QtPositioning.coordinate(latitude, longitude)
                anchorPoint: Qt.point(icon.width / 2, icon.height)

                sourceItem: Column {
                    spacing: 4
                    width: Math.max(icon.width, aircraft.implicitWidth)

                    Item {
                        id: icon
                        width: 28
                        height: 28
                        anchors.horizontalCenter: parent.horizontalCenter
                        transform: Rotation {
                            origin.x: icon.width / 2
                            origin.y: icon.height / 2
                            angle: heading
                        }

                        Text {
                            id: aircraft
                            text: qsTr("✈")
                            font.pixelSize: 30
                            color:"black"
                            rotation: 270
                        }
                    }

                    Label {
                        text: callsign.length ? callsign : icao24
                        color: "black"
                        font.pixelSize: 12
                        anchors.horizontalCenter: parent.horizontalCenter
                        elide: Text.ElideRight
                    }

                    TapHandler {
                        onTapped: {
                            map.center = marker.coordinate
                            selectedAircraft = ({
                                callsign: callsign,
                                icao24: icao24,
                                latitude: latitude,
                                longitude: longitude,
                                altitude: altitude,
                                speed: speed,
                                heading: heading,
                                inZone: inZone
                            })
                            detailPopup.open()
                        }
                    }
                }
            }
        }
    }

    // Seçilen uçak detaylarının gösterildiği popup
    Popup {
        id: detailPopup
        x: 12
        y: window.height - height - 12
        width: 280
        modal: false
        focus: true
        visible: false
        background: Rectangle {
            color: "black"
            radius: 10
            border.color: "lightblue"
            border.width: 2
        }

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            Label {
                text: selectedAircraft && selectedAircraft.callsign ? selectedAircraft.callsign : (selectedAircraft ? selectedAircraft.icao24 : "")
                color: "white"
                font.pixelSize: 18
                font.bold: true
            }
            Label { text: selectedAircraft ? "ICAO: " + selectedAircraft.icao24 : ""; color: "white"; font.pixelSize: 13 }
            Label { text: selectedAircraft ? "Konum: " + selectedAircraft.latitude.toFixed(4) + ", " + selectedAircraft.longitude.toFixed(4) : ""; color: "white"; font.pixelSize: 13 }
            Label { text: selectedAircraft ? "İrtifa: " + Math.round(selectedAircraft.altitude) + " m" : ""; color: "white"; font.pixelSize: 13 }
            Label { text: selectedAircraft ? "Hız: " + Math.round(selectedAircraft.speed * 3.6) + " km/h" : ""; color: "white"; font.pixelSize: 13 }
            Label { text: selectedAircraft ? "Yöneliyor: " + Math.round(selectedAircraft.heading) + "°" : ""; color: "white"; font.pixelSize: 13 }
            Label { text: selectedAircraft ? (selectedAircraft.inZone ? "Alan içerisinde" : "Alan dışında") : ""; color: selectedAircraft && selectedAircraft.inZone ? "red" : "white"; font.pixelSize: 13 }

            Button {
                text: qsTr("Kapat")
                onClicked: detailPopup.close()
                width: 80
            }
        }
    }
    // No Fly Zone'a giriş yapan uçaklar için uyarı
    Popup {
        id: alertPopup
        x: (window.width - width) / 2
        y: 12
        width: 360
        height: implicitHeight
        modal: false
        focus: false
        visible: false
        background: Rectangle {
            color: "#e74c3c"
            radius: 10
            border.color: "#ffffff"
            border.width: 1
        }

        Row {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10
            Label {
                text: "İhlal Tespit Edildi!"
                color: "white"
                font.bold: true
                font.pixelSize: 16
            }
            Label {
                id: alertText
                text: ""
                color: "white"
                font.pixelSize: 16
            }
        }
    }

    Timer {
        id: alertCloser
        interval: 4500
        repeat: false
        onTriggered: alertPopup.close()
    }

    Component.onCompleted: aircraftModel.start()
}
