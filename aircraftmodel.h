#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QUrlQuery>

// Tek bir uçağın çekilen verileri
struct AircraftData
{
    QString icao24;
    QString callsign;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;      // meters
    double speed = 0.0;         // m/s
    double heading = 0.0;       // degrees
    bool inZone = false;
};

// OpenSky uçuş verilerini QML için listeleyen model
class AircraftModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate zoneCenter READ zoneCenter WRITE setZoneCenter NOTIFY zoneCenterChanged)
    Q_PROPERTY(double zoneRadiusMeters READ zoneRadiusMeters WRITE setZoneRadiusMeters NOTIFY zoneRadiusMetersChanged)
    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString tokenUrl READ tokenUrl WRITE setTokenUrl NOTIFY tokenUrlChanged)
    Q_PROPERTY(double bboxScale READ bboxScale WRITE setBboxScale NOTIFY bboxScaleChanged)
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    Q_PROPERTY(QString clientSecret READ clientSecret WRITE setClientSecret NOTIFY clientSecretChanged)
    Q_PROPERTY(int refreshIntervalMs READ refreshIntervalMs WRITE setRefreshIntervalMs NOTIFY refreshIntervalMsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Roles {
        Icao24Role = Qt::UserRole + 1,
        CallsignRole,
        LatitudeRole,
        LongitudeRole,
        AltitudeRole,
        SpeedRole,
        HeadingRole,
        InZoneRole
    };
    Q_ENUM(Roles)

    explicit AircraftModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refreshNow();

    QGeoCoordinate zoneCenter() const { return m_zoneCenter; }
    void setZoneCenter(const QGeoCoordinate &center);

    double zoneRadiusMeters() const { return m_zoneRadiusMeters; }
    void setZoneRadiusMeters(double radiusMeters);

    QString apiUrl() const { return m_apiUrl; }
    void setApiUrl(const QString &url);

    QString tokenUrl() const { return m_tokenUrl; }
    void setTokenUrl(const QString &url);

    double bboxScale() const { return m_bboxScale; }
    void setBboxScale(double scale);

    QString clientId() const { return m_clientId; }
    void setClientId(const QString &id);

    QString clientSecret() const { return m_clientSecret; }
    void setClientSecret(const QString &secret);

    int refreshIntervalMs() const { return m_refreshIntervalMs; }
    void setRefreshIntervalMs(int intervalMs);

    QString lastError() const { return m_lastError; }

signals:
    void intrusionDetected(const QString &callsign, const QString &icao24);
    void zoneCenterChanged();
    void zoneRadiusMetersChanged();
    void apiUrlChanged();
    void tokenUrlChanged();
    void bboxScaleChanged();
    void clientIdChanged();
    void clientSecretChanged();
    void refreshIntervalMsChanged();
    void lastErrorChanged();

private slots:
    void handleNetworkFinished();
    void handleTokenFinished();

private:
    void requestToken();
    void performFetch();
    double haversineMeters(const QGeoCoordinate &a, const QGeoCoordinate &b) const;
    void setLastError(const QString &error);

    QNetworkAccessManager m_network{this};
    QPointer<QNetworkReply> m_activeReply;
    QTimer m_timer;
    QList<AircraftData> m_records;
    QSet<QString> m_previousIntruders;
    QGeoCoordinate m_zoneCenter {39.9334, 32.8597};
    double m_zoneRadiusMeters = 30000.0;
    QString m_apiUrl = QStringLiteral("https://opensky-network.org/api/states/all");
    QString m_tokenUrl = QStringLiteral("https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token");
    double m_bboxScale = 1.0; 
    QString m_accessToken;
    qint64 m_tokenExpiryMsSinceEpoch = 0;
    bool m_fetchPending = false;
    QPointer<QNetworkReply> m_tokenReply;
    QString m_clientId;
    QString m_clientSecret;
    int m_refreshIntervalMs = 15000;
    QString m_lastError;
};
