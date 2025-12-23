# No Fly Zone Monitor 🛫

Qt 6 ve QML ile geliştirilmiş gerçek zamanlı uçak takip ve yasak bölge izleme uygulaması. Kısıtlı hava sahasının etrafındaki uçak pozisyonlarını izleyin ve uçaklar belirlenen yasak bölgeye girdiğinde anında uyarı alın.

## Özellikler

- 🗺️ **Etkileşimli Harita**: OpenStreetMap üzerinde OpenSky Network ile gerçek zamanlı uçak pozisyonlarının görselleştirilmesi
- 📡 **Canlı Takip**: OpenSky Network API'den otomatik uçak verisi çekme
- 🚨 **İhlal Tespiti**: Uçaklar yasak bölgeye girdiğinde anında uyarılar
- ✈️ **Uçak Detayları**: Detaylı bilgi gösterimi (çağrı işareti, irtifa, hız, yöneliş)
- 🎯 **Yapılandırılabilir Bölge**: Özelleştirilebilir yasak bölge merkezi ve yarıçapı
- 🔄 **Otomatik Yenileme**: Periyodik veri güncellemeleri (varsayılan: 15 saniye)
- 🔐 **OAuth2 Desteği**: Token yönetimi ile kimlik doğrulamalı API erişimi

## Teknolojiler

- **Qt 6.10.1** - Çapraz platform uygulama çatısı
- **QML** - Bildirimsel kullanıcı arayüzü dili
- **C++17** - Temel uygulama mantığı
- **CMake** - Derleme sistemi
- **Qt Location** - Harita ve konum hizmetleri
- **Qt Network** - HTTP/API iletişimi

## Gereksinimler

- Qt 6.8 veya üzeri
- Qt modülleri:
  - Qt Quick
  - Qt Network
  - Qt Positioning
  - Qt Location
- CMake 3.16 veya üzeri
- C++17 destekli C++ derleyici (Windows için MinGW 64-bit önerilir)

## Kurulum

### Depoyu klonlayın

```bash
git clone https://github.com/kullaniciadi/noflyzone.git
cd noflyzone
```

### CMake ile derleme

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Qt Creator ile derleme

1. Qt Creator'da `CMakeLists.txt` dosyasını açın
2. Projeyi Qt kit'inizle yapılandırın
3. Derleyin ve çalıştırın

## Yapılandırma

### API Kimlik Bilgileri

Uygulama OpenSky Network API'yi OAuth2 kimlik doğrulaması ile kullanır. `Main.qml` dosyasındaki kimlik bilgilerini güncelleyin:

```qml
AircraftModel {
    clientId: "client-id"
    clientSecret: "client-secret"
}
```

### Yasak Bölge Ayarları

`Main.qml` dosyasında kısıtlı alanı özelleştirin:

```qml
readonly property var zoneCenterCoord: QtPositioning.coordinate(39.9334, 32.8597)  // Ankara
readonly property double zoneRadiusMeters: 30000  // 30 km
```

### Yenileme Aralığı

Güncelleme sıklığını ayarlayın:

```qml
AircraftModel {
    refreshIntervalMs: 15000  // 15 saniye
}
```

## Kullanım

1. **Uygulamayı başlatın** - Harita yapılandırılmış yasak bölgeye odaklanacaktır
2. **Uçakları görüntüleyin** - Uçak işaretçileri çağrı işaretleriyle harita üzerinde görünür
3. **Uçağa tıklayın** - Açılır pencerede detaylı bilgileri görün
4. **Haritada gezinin** - Kaydırmak için sürükleyin, yakınlaştırmak için +/- düğmelerini kullanın
5. **Verileri yenileyin** - Anında güncelleme için yenileme düğmesine (⟳) tıklayın
6. **Görünümü sıfırlayın** - Varsayılan konuma dönmek için R düğmesine tıklayın

### Kontroller

- **⟳** - Manuel yenileme
- **+** - Yakınlaştır
- **-** - Uzaklaştır
- **R** - Haritayı varsayılan konuma sıfırla

## Proje Yapısı

```
noflyzone/
├── main.cpp              # Uygulama giriş noktası
├── Main.qml              # Ana kullanıcı arayüzü ve harita görünümü
├── aircraftmodel.h       # Uçak veri modeli başlığı
├── aircraftmodel.cpp     # Uçak veri modeli implementasyonu
├── CMakeLists.txt        # CMake derleme yapılandırması
├── .gitignore            # Git ignore kuralları
└── README.md             # Bu dosya
```

## API Referansı

Uygulama, gerçek zamanlı uçak verilerini çekmek için [OpenSky Network API](https://openskynetwork.github.io/opensky-api/) kullanır.

**Endpoint'ler:**
- States API: `https://opensky-network.org/api/states/all`
- OAuth2 Token: `https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token`

## Teşekkürler

- Ücretsiz uçak takip verisi sağladıkları için [OpenSky Network](https://opensky-network.org/)
- Harita karoları için [OpenStreetMap](https://www.openstreetmap.org/)
- Mükemmel geliştirme araçları için Qt Framework

