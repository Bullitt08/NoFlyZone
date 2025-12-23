#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>
#include <QUrl>
#include <QUrlQuery>
#include <QtMath>
#include <limits>
#include "aircraftmodel.h"

AircraftModel::AircraftModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Periyodik yenileme zamanlayıcısı.
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, &AircraftModel::performFetch);
}

int AircraftModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_records.size();
}

QVariant AircraftModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size()) {
        return {};
    }

    const auto &rec = m_records.at(index.row());
    switch (role) {
    case Icao24Role:
        return rec.icao24;
    case CallsignRole:
        return rec.callsign;
    case LatitudeRole:
        return rec.latitude;
    case LongitudeRole:
        return rec.longitude;
    case AltitudeRole:
        return rec.altitude;
    case SpeedRole:
        return rec.speed;
    case HeadingRole:
        return rec.heading;
    case InZoneRole:
        return rec.inZone;
    default:
        return {};
    }
}

QHash<int, QByteArray> AircraftModel::roleNames() const
{
    return {
        {Icao24Role, "icao24"},
        {CallsignRole, "callsign"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {AltitudeRole, "altitude"},
        {SpeedRole, "speed"},
        {HeadingRole, "heading"},
        {InZoneRole, "inZone"}
    };
}

// Zamanlayıcıyı başlat ve ilk sorguyu yap
void AircraftModel::start()
{

    if (m_timer.isActive()) {
        return;
    }
    m_timer.start(m_refreshIntervalMs);
    performFetch();
}

// Zamanlayıcıyı durdur ve devam eden isteği iptal et
void AircraftModel::stop()
{

    m_timer.stop();
    if (m_activeReply) {
        m_activeReply->abort();
    }
}

// El ile anlık yenileme
void AircraftModel::refreshNow()
{

    performFetch();
}


// Bölge merkezini güncelle
void AircraftModel::setZoneCenter(const QGeoCoordinate &center)
{

    if (center == m_zoneCenter) {
        return;
    }
    m_zoneCenter = center;
    emit zoneCenterChanged();
}

// Bölge yarıçapını güncelle
void AircraftModel::setZoneRadiusMeters(double radiusMeters)
{

    if (qFuzzyCompare(radiusMeters, m_zoneRadiusMeters)) {
        return;
    }
    m_zoneRadiusMeters = radiusMeters;
    emit zoneRadiusMetersChanged();
}

// API uç noktasını güncelle
void AircraftModel::setApiUrl(const QString &url)
{

    if (url == m_apiUrl) {
        return;
    }
    m_apiUrl = url;
    emit apiUrlChanged();
}

// Token alınacak endpoint'i güncelle
void AircraftModel::setTokenUrl(const QString &url)
{

    if (url == m_tokenUrl) {
        return;
    }
    m_tokenUrl = url;
    emit tokenUrlChanged();
}

// Veri alanı genişliği için ölçek katsayısı
void AircraftModel::setBboxScale(double scale)
{

    if (qFuzzyCompare(scale, m_bboxScale)) {
        return;
    }
    m_bboxScale = scale;
    emit bboxScaleChanged();
}

// OAuth istemci kimliği
void AircraftModel::setClientId(const QString &id)
{

    if (id == m_clientId) {
        return;
    }
    m_clientId = id;
    emit clientIdChanged();
}

// OAuth gizli anahtarı
void AircraftModel::setClientSecret(const QString &secret)
{

    if (secret == m_clientSecret) {
        return;
    }
    m_clientSecret = secret;
    emit clientSecretChanged();
}

// Yenileme aralığını güncelle
void AircraftModel::setRefreshIntervalMs(int intervalMs)
{

    if (intervalMs == m_refreshIntervalMs) {
        return;
    }
    m_refreshIntervalMs = intervalMs;
    if (m_timer.isActive()) {
        m_timer.start(m_refreshIntervalMs);
    }
    emit refreshIntervalMsChanged();
}

void AircraftModel::handleNetworkFinished()
{
    if (!m_activeReply) {
        return;
    }

    const auto reply = m_activeReply;
    m_activeReply = nullptr;

    reply->deleteLater();

    // Taşıma katmanı hataları
    if (reply->error() != QNetworkReply::NoError) {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto body = reply->readAll();
        setLastError(QStringLiteral("Ağ hatası (%1): %2 %3")
                         .arg(status)
                         .arg(reply->errorString(), QString::fromUtf8(body.left(120))));
        return;
    }

    const auto payload = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // HTTP durum hataları
    if (status >= 300) {
        setLastError(QStringLiteral("HTTP %1: %2")
                         .arg(status)
                         .arg(QString::fromUtf8(payload.left(200))));
        return;
    }
    const auto doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        setLastError(QStringLiteral("Beklenilmeyen uçak verisi yanıtı"));
        return;
    }

    const auto obj = doc.object();

    // OpenSky "states" dizisi için ayrıştırıcı
    auto processOpenSky = [&](const QJsonArray &states) -> bool {
        QList<AircraftData> newData;
        QSet<QString> newIntruders;
        newData.reserve(states.size());

        for (const auto &entryValue : states) {
            if (!entryValue.isArray()) {
                continue;
            }
            const auto arr = entryValue.toArray();
            if (arr.size() < 11) {
                continue;
            }

            AircraftData rec;
            rec.icao24 = arr.at(0).toString();
            rec.callsign = arr.at(1).toString().trimmed();
            const double longitude = arr.at(5).toDouble(std::numeric_limits<double>::quiet_NaN());
            const double latitude = arr.at(6).toDouble(std::numeric_limits<double>::quiet_NaN());
            if (std::isnan(latitude) || std::isnan(longitude)) {
                continue;
            }
            rec.longitude = longitude;
            rec.latitude = latitude;
            rec.altitude = arr.at(7).toDouble(0.0);
            rec.speed = arr.at(9).toDouble(0.0);
            rec.heading = arr.at(10).toDouble(0.0);

            const QGeoCoordinate aircraftCoord(rec.latitude, rec.longitude);
            const double distance = haversineMeters(aircraftCoord, m_zoneCenter);
            rec.inZone = distance <= m_zoneRadiusMeters;
            if (rec.inZone) {
                newIntruders.insert(rec.icao24);
                if (!m_previousIntruders.contains(rec.icao24)) {
                    emit intrusionDetected(rec.callsign.isEmpty() ? rec.icao24 : rec.callsign, rec.icao24);
                }
            }

            newData.push_back(rec);
        }

        beginResetModel();
        m_records = std::move(newData);
        endResetModel();
        m_previousIntruders = newIntruders;
        return true;
    };

    bool handled = false;
    // Hangi şemanın geldiğini seç
    if (obj.contains(QStringLiteral("states"))) {
        const auto statesValue = obj.value(QStringLiteral("states"));
        if (statesValue.isArray()) {
            handled = processOpenSky(statesValue.toArray());
        } else if (statesValue.isNull()) {
            beginResetModel();
            m_records.clear();
            endResetModel();
            m_previousIntruders.clear();
            handled = true;
            setLastError(QStringLiteral("OpenSky states=null döndürdü (alan boş ya da oran sınırlı)"));
        }
    }

    if (!handled) {
        const QString keys = obj.keys().join(',');
        const QByteArray raw = doc.toJson(QJsonDocument::Compact);
        setLastError(QStringLiteral("Uçak yükü tanınmadı. anahtarlar=[%1] gövde=%2")
                         .arg(keys)
                         .arg(QString::fromUtf8(raw.left(220))));
        return;
    }

    setLastError({});
}

void AircraftModel::requestToken()
{
    // Paralel token isteğini engelle; biri çalışıyorsa fetch'i sıraya al.
    if (m_tokenReply) {
        m_fetchPending = true;
        return;
    }
    if (m_clientId.isEmpty() || m_clientSecret.isEmpty() || m_tokenUrl.isEmpty()) {
        return;
    }

    // Client credentials akışı
    QNetworkRequest req{QUrl(m_tokenUrl)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setTransferTimeout(10000);

    const QByteArray body = QByteArray("grant_type=client_credentials&client_id=")
                                + QUrl::toPercentEncoding(m_clientId)
                                + "&client_secret="
                                + QUrl::toPercentEncoding(m_clientSecret);

    m_tokenReply = m_network.post(req, body);
    connect(m_tokenReply, &QNetworkReply::finished, this, &AircraftModel::handleTokenFinished);
}

void AircraftModel::performFetch()
{
    // Aynı anda tek ağ isteği olsun
    if (m_activeReply) {
        return;
    }

    QUrl url{m_apiUrl};

    const bool wantBearer = !m_clientId.isEmpty() && !m_clientSecret.isEmpty() && !m_tokenUrl.isEmpty();
    if (wantBearer) {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const bool tokenMissing = m_accessToken.isEmpty();
        const bool tokenExpired = now >= (m_tokenExpiryMsSinceEpoch - 30000); // refresh 30s early
        if (tokenMissing || tokenExpired) {
            m_fetchPending = true;
            requestToken();
            return;
        }
    }

    // OpenSky için bbox verilmemişse bölge etrafında (ölçekli) bbox ekle.
    if (url.host().contains(QStringLiteral("opensky-network.org"))) {
        QUrlQuery query{url};
        const bool hasBBox = query.hasQueryItem(QStringLiteral("lamin"))
                           && query.hasQueryItem(QStringLiteral("lamax"))
                           && query.hasQueryItem(QStringLiteral("lomin"))
                           && query.hasQueryItem(QStringLiteral("lomax"));
        if (!hasBBox && m_zoneRadiusMeters > 0.0 && m_bboxScale > 0.0) {
            const double effectiveRadius = m_zoneRadiusMeters * m_bboxScale;
            const double latDelta = effectiveRadius / 111320.0;
            const double lonMeterPerDeg = qMax(1e-6, qAbs(qCos(qDegreesToRadians(m_zoneCenter.latitude()))) * 111320.0);
            const double lonDelta = effectiveRadius / lonMeterPerDeg;

            query.addQueryItem(QStringLiteral("lamin"), QString::number(m_zoneCenter.latitude() - latDelta, 'f', 6));
            query.addQueryItem(QStringLiteral("lamax"), QString::number(m_zoneCenter.latitude() + latDelta, 'f', 6));
            query.addQueryItem(QStringLiteral("lomin"), QString::number(m_zoneCenter.longitude() - lonDelta, 'f', 6));
            query.addQueryItem(QStringLiteral("lomax"), QString::number(m_zoneCenter.longitude() + lonDelta, 'f', 6));
            url.setQuery(query);
        }
    }

    QNetworkRequest req{url};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NoFlyZone/1.0"));
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false); // some providers on free tier reject HTTP/2
    req.setTransferTimeout(10000);

    if (wantBearer) {
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
    } else if (!m_clientId.isEmpty() || !m_clientSecret.isEmpty()) {
        // Fallback to Basic auth when token endpoint is not provided.
        const QByteArray token = QStringLiteral("%1:%2").arg(m_clientId, m_clientSecret).toUtf8().toBase64();
        req.setRawHeader("Authorization", "Basic " + token);
    }

    m_activeReply = m_network.get(req);
    connect(m_activeReply, &QNetworkReply::finished, this, &AircraftModel::handleNetworkFinished);
}

double AircraftModel::haversineMeters(const QGeoCoordinate &a, const QGeoCoordinate &b) const
{
    static constexpr double earthRadius = 6371000.0; // meters
    const double lat1 = qDegreesToRadians(a.latitude());
    const double lon1 = qDegreesToRadians(a.longitude());
    const double lat2 = qDegreesToRadians(b.latitude());
    const double lon2 = qDegreesToRadians(b.longitude());

    const double dLat = lat2 - lat1;
    const double dLon = lon2 - lon1;

    const double h = qPow(qSin(dLat / 2.0), 2) + qCos(lat1) * qCos(lat2) * qPow(qSin(dLon / 2.0), 2);
    const double c = 2.0 * qAtan2(qSqrt(h), qSqrt(1.0 - h));
    return earthRadius * c;
}

void AircraftModel::setLastError(const QString &error)
{
    if (error == m_lastError) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged();
}

void AircraftModel::handleTokenFinished()
{
    // Token isteğinin yanıtını ele al
    if (!m_tokenReply) {
        return;
    }

    const auto reply = m_tokenReply;
    m_tokenReply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto body = reply->readAll();
        setLastError(QStringLiteral("Token hatası (%1): %2 %3")
                         .arg(status)
                         .arg(reply->errorString())
                         .arg(QString::fromUtf8(body.left(200))));
        m_fetchPending = false;
        return;
    }

    const auto body = reply->readAll();
    const auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        setLastError(QStringLiteral("Token cevabı JSON değil"));
        m_fetchPending = false;
        return;
    }

    const auto obj = doc.object();
    const QString token = obj.value(QStringLiteral("access_token")).toString();
    const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(600);
    if (token.isEmpty()) {
        setLastError(QStringLiteral("Token access_token eksik"));
        m_fetchPending = false;
        return;
    }

    m_accessToken = token;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_tokenExpiryMsSinceEpoch = now + qMax(60, expiresIn - 60) * 1000LL;
    setLastError({});

    if (m_fetchPending) {
        m_fetchPending = false;
        performFetch();
    }
}
